#[compute]

#version 450

#VERSION_DEFINES

// GI worked out below half resolution brought up to half, where the scene
// reads it from: each texel is mixed from the four low resolution ones around
// it, as far as they lie on the same surface - same plane, same facing - so
// light does not bleed over edges.

#define WG_SIZE 8

layout(local_size_x = WG_SIZE, local_size_y = WG_SIZE, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform texture2D depth_buffer;
layout(set = 0, binding = 1) uniform texture2D normal_roughness_buffer;
layout(set = 0, binding = 2) uniform texture2D ambient_buffer;
layout(set = 0, binding = 3) uniform texture2D reflection_buffer;
layout(set = 0, binding = 4) uniform texture2D blend_buffer;
layout(r32ui, set = 0, binding = 5) uniform restrict writeonly uimage2D dst_ambient_buffer;
layout(r32ui, set = 0, binding = 6) uniform restrict writeonly uimage2D dst_reflection_buffer;
layout(rg8, set = 0, binding = 7) uniform restrict writeonly image2D dst_blend_buffer;
layout(set = 0, binding = 8) uniform sampler linear_sampler;

layout(constant_id = 0) const bool sc_use_full_projection_matrix = false;

layout(set = 0, binding = 9, std140) uniform SceneData {
	mat4x4 inv_projection[2];
	mat4x4 cam_transform;
	vec4 eye_offset[2];

	ivec2 screen_size;
	float pad1;
	float pad2;
}
scene_data;

layout(push_constant, std430) uniform Params {
	bool orthogonal;
	float z_near;
	float z_far;
	uint view_index;

	vec4 proj_info;

	ivec2 src_size;
	uint src_shift;
	uint pad;
}
params;

// How fast a neighbour stops counting as it leaves this pixel's plane, in
// distance from the plane over distance from the camera.
#define PLANE_SHARPNESS 50.0

vec3 reconstruct_position(ivec2 screen_pos) {
	if (sc_use_full_projection_matrix) {
		vec4 pos;
		pos.xy = (2.0 * vec2(screen_pos) / vec2(scene_data.screen_size)) - 1.0;
		pos.z = texelFetch(sampler2D(depth_buffer, linear_sampler), screen_pos, 0).r * 2.0 - 1.0;
		pos.w = 1.0;

		pos = scene_data.inv_projection[params.view_index] * pos;

		return pos.xyz / pos.w;
	} else {
		vec3 pos;
		pos.z = texelFetch(sampler2D(depth_buffer, linear_sampler), screen_pos, 0).r;

		pos.z = pos.z * 2.0 - 1.0;
		if (params.orthogonal) {
			pos.z = ((pos.z + (params.z_far + params.z_near) / (params.z_far - params.z_near)) * (params.z_far - params.z_near)) / 2.0;
		} else {
			pos.z = 2.0 * params.z_near * params.z_far / (params.z_far + params.z_near - pos.z * (params.z_far - params.z_near));
		}
		pos.z = -pos.z;

		pos.xy = vec2(screen_pos) * params.proj_info.xy + params.proj_info.zw;
		if (!params.orthogonal) {
			pos.xy *= pos.z;
		}

		return pos;
	}
}

vec4 fetch_normal_and_roughness(ivec2 pos) {
	vec4 normal_roughness = texelFetch(sampler2D(normal_roughness_buffer, linear_sampler), pos, 0);
	if (normal_roughness.xyz != vec3(0)) {
		normal_roughness.xyz = normalize(normal_roughness.xyz * 2.0 - 1.0);
		bool dynamic_object = normal_roughness.a > 0.5;
		if (dynamic_object) {
			normal_roughness.a = 1.0 - normal_roughness.a;
		}
		normal_roughness.a /= (127.0 / 255.0);
	}
	return normal_roughness;
}

uint rgbe_encode(vec3 rgb) {
	const float rgbe_max = uintBitsToFloat(0x477F8000);
	const float rgbe_min = uintBitsToFloat(0x37800000);

	rgb = clamp(rgb, 0, rgbe_max);

	float max_channel = max(max(rgbe_min, rgb.r), max(rgb.g, rgb.b));

	float bias = uintBitsToFloat((floatBitsToUint(max_channel) + 0x07804000) & 0x7F800000);

	uvec3 urgb = floatBitsToUint(rgb + bias);
	uint e = (floatBitsToUint(bias) << 4) + 0x10000000;
	return e | (urgb.b << 18) | (urgb.g << 9) | (urgb.r & 0x1FF);
}

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if (any(greaterThanEqual(pos, imageSize(dst_blend_buffer)))) {
		return;
	}

	// As at half resolution: the texel stands for the first pixel of its 2x2.
	ivec2 pixel = min(pos << 1, scene_data.screen_size - 1);

	vec4 normal_roughness = fetch_normal_and_roughness(pixel);
	if (normal_roughness.xyz == vec3(0)) {
		// Nothing drawn here, no GI either.
		imageStore(dst_ambient_buffer, pos, uvec4(0));
		imageStore(dst_reflection_buffer, pos, uvec4(0));
		imageStore(dst_blend_buffer, pos, vec4(0));
		return;
	}
	vec3 vertex = reconstruct_position(pixel);
	vec3 normal = normal_roughness.xyz;
	float roughness = normal_roughness.a;
	float plane_scale = PLANE_SHARPNESS / max(abs(vertex.z), 0.05);

	// Low resolution texel i was worked out for the pixel at i << src_shift.
	vec2 src_pos = vec2(pixel) / float(1 << params.src_shift);
	ivec2 base = ivec2(src_pos);
	vec2 f = src_pos - vec2(base);

	// Light mixed weighted by how much of it there is (the blend), so that
	// GI fading out at the edge of a VoxelGI does not darken what it mixes in.
	vec3 ambient = vec3(0.0);
	vec3 reflection = vec3(0.0);
	float ambient_alpha = 0.0;
	float reflection_alpha = 0.0;
	float ambient_weight = 0.0;
	float reflection_weight = 0.0;

	// Fallback for a pixel none of the four are much like - a thin pole on a
	// far wall: the one most like it.
	float best_weight = -1.0;
	ivec2 best_src = base;

	for (int i = 0; i < 4; i++) {
		ivec2 offset = ivec2(i & 1, i >> 1);
		ivec2 src = min(base + offset, params.src_size - 1);
		ivec2 src_pixel = min(src << params.src_shift, scene_data.screen_size - 1);

		vec4 src_normal_roughness = fetch_normal_and_roughness(src_pixel);
		if (src_normal_roughness.xyz == vec3(0)) {
			continue; // Sky: holds no GI.
		}

		vec3 src_vertex = reconstruct_position(src_pixel);
		float facing = max(0.0, dot(normal, src_normal_roughness.xyz));
		facing *= facing;
		float weight = facing * facing * exp(-abs(dot(src_vertex - vertex, normal)) * plane_scale);
		if (weight > best_weight) {
			best_weight = weight;
			best_src = src;
		}

		weight *= (offset.x == 1 ? f.x : 1.0 - f.x) * (offset.y == 1 ? f.y : 1.0 - f.y);
		// Reflections also change with roughness.
		float src_reflection_weight = weight * max(0.0, 1.0 - abs(src_normal_roughness.a - roughness) * 2.0);

		vec2 blend = texelFetch(sampler2D(blend_buffer, linear_sampler), src, 0).rg;
		ambient += texelFetch(sampler2D(ambient_buffer, linear_sampler), src, 0).rgb * blend.r * weight;
		ambient_alpha += blend.r * weight;
		ambient_weight += weight;
		reflection += texelFetch(sampler2D(reflection_buffer, linear_sampler), src, 0).rgb * blend.g * src_reflection_weight;
		reflection_alpha += blend.g * src_reflection_weight;
		reflection_weight += src_reflection_weight;
	}

	if (best_weight < 0.0) {
		// All four were sky.
		imageStore(dst_ambient_buffer, pos, uvec4(0));
		imageStore(dst_reflection_buffer, pos, uvec4(0));
		imageStore(dst_blend_buffer, pos, vec4(0));
		return;
	}

	const float min_weight = 0.0001;
	vec2 best_blend = texelFetch(sampler2D(blend_buffer, linear_sampler), best_src, 0).rg;
	vec4 ambient_light;
	if (ambient_weight > min_weight) {
		ambient_light.a = ambient_alpha / ambient_weight;
		ambient_light.rgb = ambient_alpha > 0.0 ? ambient / ambient_alpha : vec3(0.0);
	} else {
		ambient_light = vec4(texelFetch(sampler2D(ambient_buffer, linear_sampler), best_src, 0).rgb, best_blend.r);
	}
	vec4 reflection_light;
	if (reflection_weight > min_weight) {
		reflection_light.a = reflection_alpha / reflection_weight;
		reflection_light.rgb = reflection_alpha > 0.0 ? reflection / reflection_alpha : vec3(0.0);
	} else {
		reflection_light = vec4(texelFetch(sampler2D(reflection_buffer, linear_sampler), best_src, 0).rgb, best_blend.g);
	}

	imageStore(dst_ambient_buffer, pos, uvec4(rgbe_encode(ambient_light.rgb)));
	imageStore(dst_reflection_buffer, pos, uvec4(rgbe_encode(reflection_light.rgb)));
	imageStore(dst_blend_buffer, pos, vec4(ambient_light.a, reflection_light.a, 0, 0));
}

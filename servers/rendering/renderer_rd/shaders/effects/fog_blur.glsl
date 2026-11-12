#[compute]
#version 450

#VERSION_DEFINES

#define MAX_VIEWS 2
#include "../scene_data_inc.glsl"

layout(set = 0, binding = 0) uniform sampler2D screen_sampler;
layout(set = 0, binding = 1) uniform sampler2D depth_sampler;
layout(set = 0, binding = 4) uniform sampler3D volumetric_fog_sampler;
layout(set = 0, binding = 2, std140) uniform SceneDataBlock {
	SceneData data;
	SceneData prev_data;
}
scene;
layout(rgba16f, set = 0, binding = 3) uniform writeonly image2D output_image;

layout(push_constant, std430) uniform Params {
	int max_lod;
	int view_index;
	float intensity;
	float max_blur_lod;
	int gaussian_filter_enabled;
	int depth_fog;
	int volumetric_enabled;
	float volumetric_fog_inv_length;
	float volumetric_fog_detail_spread;
}
params;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

vec2 uv_to_ndc(vec2 p_uv) {
	return p_uv * 2.0 - 1.0;
}

mat4 view_to_world_matrix(mat3x4 p_matrix) {
	return transpose(mat4(p_matrix[0], p_matrix[1], p_matrix[2], vec4(0.0, 0.0, 0.0, 1.0)));
}

vec3 reconstruct_view_position(ivec2 p_pixel, ivec2 p_screen_size, float p_depth) {
	vec2 uv = (vec2(p_pixel) + vec2(0.5)) / vec2(p_screen_size);
	vec4 view_position = scene.data.inv_projection_matrix_view[params.view_index] * vec4(uv_to_ndc(uv), p_depth, 1.0);
	if (abs(view_position.w) < 0.000001) {
		return vec3(0.0, 0.0, -scene.data.z_far);
	}
	return view_position.xyz / view_position.w;
}

float get_fog_amount(vec3 p_view_position, vec2 p_uv) {
	float fog_amount = 0.0;
	float view_distance = length(p_view_position);

	if ((scene.data.flags & SCENE_DATA_FLAGS_USE_FOG) != 0u) {
		if (params.depth_fog != 0) {
			float fog_z = smoothstep(scene.data.fog_depth_begin, scene.data.fog_depth_end, view_distance);
			fog_amount = pow(fog_z, scene.data.fog_depth_curve) * scene.data.fog_density;
		} else {
			fog_amount = 1.0 - exp(-view_distance * scene.data.fog_density);
		}

		if (abs(scene.data.fog_height_density) >= 0.0001) {
			mat4 inv_view_matrix = view_to_world_matrix(scene.data.inv_view_matrix);
			vec3 world_position = (inv_view_matrix * vec4(p_view_position, 1.0)).xyz;
			float y_dist = world_position.y - scene.data.fog_height;
			float height_fog_amount = 1.0 - exp(min(0.0, y_dist * scene.data.fog_height_density));
			fog_amount = max(fog_amount, height_fog_amount);
		}
	}

	if (params.volumetric_enabled != 0) {
		float volumetric_depth = max(0.0, -p_view_position.z) * params.volumetric_fog_inv_length;
		if (volumetric_depth < 1.0) {
			volumetric_depth = pow(volumetric_depth, params.volumetric_fog_detail_spread);
		}
		float volumetric_amount = 1.0 - texture(volumetric_fog_sampler, vec3(p_uv, volumetric_depth)).a;
		fog_amount = 1.0 - (1.0 - fog_amount) * (1.0 - volumetric_amount);
	}

	return clamp(fog_amount, 0.0, 1.0);
}

vec3 sample_gaussian_blur(vec2 p_uv, float p_radius) {
	if (p_radius < 0.5) {
		return textureLod(screen_sampler, p_uv, 0.0).rgb;
	}

	const int sample_count = 24;
	const float golden_angle = 2.39996323;
	vec2 pixel_size = 1.0 / vec2(textureSize(screen_sampler, 0));
	vec3 color = textureLod(screen_sampler, p_uv, 0.0).rgb;
	float total_weight = 1.0;

	for (int i = 0; i < sample_count; i++) {
		float normalized_radius = sqrt((float(i) + 0.5) / float(sample_count));
		float angle = float(i) * golden_angle;
		float sample_radius = normalized_radius * p_radius;
		vec2 offset = vec2(cos(angle), sin(angle)) * sample_radius * pixel_size;
		float weight = exp(-2.0 * normalized_radius * normalized_radius);

		color += textureLod(screen_sampler, p_uv + offset, 0.0).rgb * weight;
		total_weight += weight;
	}

	return color / total_weight;
}

void main() {
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	ivec2 screen_size = imageSize(output_image);
	if (any(greaterThanEqual(pixel, screen_size))) {
		return;
	}

	vec2 uv = (vec2(pixel) + vec2(0.5)) / vec2(screen_size);
	vec4 original = textureLod(screen_sampler, uv, 0.0);
	float raw_depth = texelFetch(depth_sampler, pixel, 0).r;
	vec3 view_position = reconstruct_view_position(pixel, screen_size, raw_depth);
	float fog_amount = get_fog_amount(view_position, uv);
	float optical_depth = -log(max(1.0 - fog_amount, 0.0001));
	float lod = min(optical_depth * params.intensity * float(params.max_lod), min(params.max_blur_lod, float(params.max_lod)));
	float radius = exp2(lod) - 1.0;

	vec3 blurred = params.gaussian_filter_enabled != 0 ? sample_gaussian_blur(uv, radius) : textureLod(screen_sampler, uv, lod).rgb;
	imageStore(output_image, pixel, vec4(blurred, original.a));
}

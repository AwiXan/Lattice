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

vec3 sample_gaussian_blur(vec2 p_uv, float p_lod) {
	float radius = 0.5 * (exp2(p_lod) - 1.0);
	vec2 offset = radius / vec2(textureSize(screen_sampler, 0));

	vec3 a = textureLod(screen_sampler, p_uv + offset * vec2(-1.0, -1.0), p_lod).rgb;
	vec3 b = textureLod(screen_sampler, p_uv + offset * vec2(0.0, -1.0), p_lod).rgb;
	vec3 c = textureLod(screen_sampler, p_uv + offset * vec2(1.0, -1.0), p_lod).rgb;
	vec3 d = textureLod(screen_sampler, p_uv + offset * vec2(-0.5, -0.5), p_lod).rgb;
	vec3 e = textureLod(screen_sampler, p_uv + offset * vec2(0.5, -0.5), p_lod).rgb;
	vec3 f = textureLod(screen_sampler, p_uv + offset * vec2(-1.0, 0.0), p_lod).rgb;
	vec3 g = textureLod(screen_sampler, p_uv, p_lod).rgb;
	vec3 h = textureLod(screen_sampler, p_uv + offset * vec2(1.0, 0.0), p_lod).rgb;
	vec3 i = textureLod(screen_sampler, p_uv + offset * vec2(-0.5, 0.5), p_lod).rgb;
	vec3 j = textureLod(screen_sampler, p_uv + offset * vec2(0.5, 0.5), p_lod).rgb;
	vec3 k = textureLod(screen_sampler, p_uv + offset * vec2(-1.0, 1.0), p_lod).rgb;
	vec3 l = textureLod(screen_sampler, p_uv + offset * vec2(0.0, 1.0), p_lod).rgb;
	vec3 m = textureLod(screen_sampler, p_uv + offset * vec2(1.0, 1.0), p_lod).rgb;

	return (d + e + i + j) * 0.125 +
			(a + c + k + m) * 0.03125 +
			(b + f + h + l) * 0.0625 +
			g * 0.125;
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

	vec3 blurred = params.gaussian_filter_enabled != 0 ? sample_gaussian_blur(uv, lod) : textureLod(screen_sampler, uv, lod).rgb;
	imageStore(output_image, pixel, vec4(blurred, original.a));
}

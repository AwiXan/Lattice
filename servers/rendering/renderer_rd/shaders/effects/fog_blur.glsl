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

vec3 reconstruct_view_position_uv(vec2 p_uv, float p_depth) {
	vec4 view_position = scene.data.inv_projection_matrix_view[params.view_index] * vec4(uv_to_ndc(p_uv), p_depth, 1.0);
	if (abs(view_position.w) < 0.000001) {
		return vec3(0.0, 0.0, -scene.data.z_far);
	}
	return view_position.xyz / view_position.w;
}

vec3 reconstruct_view_position(ivec2 p_pixel, ivec2 p_screen_size, float p_depth) {
	vec2 uv = (vec2(p_pixel) + vec2(0.5)) / vec2(p_screen_size);
	return reconstruct_view_position_uv(uv, p_depth);
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

float compute_fog_lod(float p_fog_amount, float p_max_lod_cap) {
	return min(p_fog_amount * params.intensity * params.max_blur_lod, p_max_lod_cap);
}

vec3 sample_depth_aware_vogel_blur(vec2 p_uv, float p_lod, float p_raw_depth, float p_view_distance, vec3 p_center_color, float p_max_lod_cap, int p_sample_count) {
	float radius = 0.5 * (exp2(p_lod) - 1.0);
	if (radius < 0.25) {
		return p_center_color;
	}

	const float GOLDEN_ANGLE = 2.39996323;
	vec2 pixel_size = 1.0 / vec2(textureSize(screen_sampler, 0));
	float inv_samples = 1.0 / float(p_sample_count);
	// Match mip footprint to inter-tap spacing so thin edges blur smoothly without discrete copies.
	float base_mip = clamp(log2(max(1.0, radius * sqrt(inv_samples))), 0.0, min(max(0.0, p_lod - 1.0), 3.0));

	// Only bridge sub-1.5px radii with the unblurred center pixel; larger radii use 0% mip-0 center
	// so sharp geometry never ghosts on top of the blur, while occluded edges normalize cleanly.
	float center_weight = smoothstep(1.5, 0.25, radius);
	vec3 color = p_center_color * center_weight;
	float total_weight = center_weight;

	for (int i = 0; i < p_sample_count; i++) {
		float r_norm = sqrt((float(i) + 0.5) * inv_samples);
		float r_px = r_norm * radius;
		float theta = float(i) * GOLDEN_ANGLE;
		vec2 sample_uv = clamp(p_uv + vec2(cos(theta), sin(theta)) * r_px * pixel_size, vec2(0.0), vec2(1.0));

		float sample_raw_depth = textureLod(depth_sampler, sample_uv, 0.0).r;
		float occ = 1.0;
		float tap_mip = base_mip;

		// Reverse-Z: larger raw depth is closer to the camera (in front of p_raw_depth).
		if (sample_raw_depth > p_raw_depth) {
			vec3 s_view = reconstruct_view_position_uv(sample_uv, sample_raw_depth);
			float s_dist = length(s_view);
			float depth_diff = (p_view_distance - s_dist) / max(p_view_distance, 0.001);
			if (depth_diff > 0.02) {
				float s_fog = get_fog_amount(s_view, sample_uv);
				float s_lod = compute_fog_lod(s_fog, p_max_lod_cap);
				float s_radius = max(0.0, 0.5 * (exp2(s_lod) - 1.0));
				occ = smoothstep(r_px - 0.5, r_px + 0.5, s_radius);
				tap_mip = min(tap_mip, s_lod);
			}
		}

		if (occ <= 0.001) {
			continue;
		}

		float weight = exp(-2.0 * r_norm * r_norm) * occ;
		color += textureLod(screen_sampler, sample_uv, tap_mip).rgb * weight;
		total_weight += weight;
	}

	if (total_weight <= 0.001) {
		return p_center_color;
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
	float view_distance = length(view_position);
	float fog_amount = get_fog_amount(view_position, uv);
	float max_lod_cap = min(params.max_blur_lod, float(params.max_lod));
	float lod = compute_fog_lod(fog_amount, max_lod_cap);

	vec3 blurred;
	if (lod <= 0.05) {
		blurred = original.rgb;
	} else {
		int sample_count = params.gaussian_filter_enabled != 0 ? 16 : 12;
		blurred = sample_depth_aware_vogel_blur(uv, lod, raw_depth, view_distance, original.rgb, max_lod_cap, sample_count);
	}

	imageStore(output_image, pixel, vec4(blurred, original.a));
}

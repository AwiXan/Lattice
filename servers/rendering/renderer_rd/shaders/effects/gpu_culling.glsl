#[compute]

#version 450

#VERSION_DEFINES

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct InstanceAABB {
	vec4 min_bounds; // xyz: min_pos, w: padding
	vec4 max_bounds; // xyz: max_pos, w: padding
};

layout(set = 0, binding = 0, std140) uniform CullParams {
	mat4 view_proj;
	mat4 prev_view_proj;
	vec4 cam_pos;
	vec4 frustum_planes[6];
	vec2 viewport_size;
	float z_near;
	float z_far;
	uint total_instances;
	uint hiz_max_mip;
	uint enable_occlusion;
	uint pad;
}
params;

layout(set = 0, binding = 1, std430) readonly buffer InstanceBuffer {
	InstanceAABB instances[];
};

layout(set = 0, binding = 2, std430) writeonly buffer VisibilityBuffer {
	uint visibility[]; // 1 = visible, 0 = culled
};

layout(set = 0, binding = 3, std430) buffer StatsBuffer {
	uint visible_count;
	uint culled_frustum;
	uint culled_occlusion;
};

layout(set = 1, binding = 0) uniform sampler2D hiz_pyramid;

void main() {
	uint idx = gl_GlobalInvocationID.x;
	if (idx >= params.total_instances) {
		return;
	}

	vec3 box_min = instances[idx].min_bounds.xyz;
	vec3 box_max = instances[idx].max_bounds.xyz;

	// 1. Frustum Culling (P-Vertex testing against 6 planes)
	bool outside = false;
	for (int i = 0; i < 6; i++) {
		vec3 p_corner = vec3(
				params.frustum_planes[i].x >= 0.0 ? box_max.x : box_min.x,
				params.frustum_planes[i].y >= 0.0 ? box_max.y : box_min.y,
				params.frustum_planes[i].z >= 0.0 ? box_max.z : box_min.z);
		if (dot(params.frustum_planes[i].xyz, p_corner) + params.frustum_planes[i].w < 0.0) {
			outside = true;
			break;
		}
	}

	if (outside) {
		visibility[idx] = 0;
		atomicAdd(culled_frustum, 1);
		return;
	}

	// 2. Hi-Z Occlusion Culling
	if (params.enable_occlusion == 0) {
		visibility[idx] = 1;
		atomicAdd(visible_count, 1);
		return;
	}

	vec3 corners[8] = vec3[8](
			vec3(box_min.x, box_min.y, box_min.z),
			vec3(box_max.x, box_min.y, box_min.z),
			vec3(box_min.x, box_max.y, box_min.z),
			vec3(box_max.x, box_max.y, box_min.z),
			vec3(box_min.x, box_min.y, box_max.z),
			vec3(box_max.x, box_min.y, box_max.z),
			vec3(box_min.x, box_max.y, box_max.z),
			vec3(box_max.x, box_max.y, box_max.z));

	vec2 min_uv = vec2(1.0);
	vec2 max_uv = vec2(0.0);
	float max_z = 0.0; // In reversed-Z, nearest point has highest Z value

	bool crosses_near_plane = false;

	for (int i = 0; i < 8; i++) {
		vec4 clip = params.prev_view_proj * vec4(corners[i], 1.0);
		if (clip.w <= 0.0001) {
			crosses_near_plane = true;
			break;
		}
		vec3 ndc = clip.xyz / clip.w;
		vec2 uv = ndc.xy * 0.5 + 0.5;
		if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
			crosses_near_plane = true;
			break;
		}
		min_uv = min(min_uv, uv);
		max_uv = max(max_uv, uv);
		max_z = max(max_z, ndc.z);
	}

	if (crosses_near_plane) {
		visibility[idx] = 1;
		atomicAdd(visible_count, 1);
		return;
	}

	min_uv = clamp(min_uv, vec2(0.0), vec2(1.0));
	max_uv = clamp(max_uv, vec2(0.0), vec2(1.0));

	vec2 size_pixels = (max_uv - min_uv) * params.viewport_size;
	float max_dim = max(size_pixels.x, size_pixels.y);
	float mip = clamp(floor(log2(max(max_dim, 1.0))), 0.0, float(params.hiz_max_mip));

	vec2 center = (min_uv + max_uv) * 0.5;
	float h0 = textureLod(hiz_pyramid, min_uv, mip).r;
	float h1 = textureLod(hiz_pyramid, vec2(max_uv.x, min_uv.y), mip).r;
	float h2 = textureLod(hiz_pyramid, vec2(min_uv.x, max_uv.y), mip).r;
	float h3 = textureLod(hiz_pyramid, max_uv, mip).r;
	float h4 = textureLod(hiz_pyramid, center, mip).r;

	float occluder_depth = min(min(min(h0, h1), min(h2, h3)), h4);

	if (max_z < occluder_depth - 0.0005) {
		visibility[idx] = 0;
		atomicAdd(culled_occlusion, 1);
		return;
	}

	visibility[idx] = 1;
	atomicAdd(visible_count, 1);
}

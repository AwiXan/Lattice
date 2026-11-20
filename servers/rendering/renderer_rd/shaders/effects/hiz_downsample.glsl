#[compute]

#version 450

#VERSION_DEFINES

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(push_constant, std430) uniform Params {
	ivec2 dest_size;
	ivec2 src_size;
}
params;

#ifdef FIRST_PASS
layout(set = 0, binding = 0) uniform sampler2D source_depth;
#else
layout(set = 0, binding = 0) uniform sampler2D source_hiz;
#endif

layout(r32f, set = 1, binding = 0) uniform restrict writeonly image2D dest_hiz;

void main() {
	ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
	if (pos.x >= params.dest_size.x || pos.y >= params.dest_size.y) {
		return;
	}

#ifdef FIRST_PASS
	// Map destination pixel to 2x2 source quad
	ivec2 src_pos = pos * 2;

	// Clamp to source dimensions
	int x0 = min(src_pos.x, params.src_size.x - 1);
	int x1 = min(src_pos.x + 1, params.src_size.x - 1);
	int y0 = min(src_pos.y, params.src_size.y - 1);
	int y1 = min(src_pos.y + 1, params.src_size.y - 1);

	float d00 = texelFetch(source_depth, ivec2(x0, y0), 0).r;
	float d10 = texelFetch(source_depth, ivec2(x1, y0), 0).r;
	float d01 = texelFetch(source_depth, ivec2(x0, y1), 0).r;
	float d11 = texelFetch(source_depth, ivec2(x1, y1), 0).r;

	// In Godot reversed-Z (1.0 = near, 0.0 = far),
	// conservative occlusion requires the minimum depth (farthest surface).
	float min_d = min(min(d00, d10), min(d01, d11));

	imageStore(dest_hiz, pos, vec4(min_d, 0.0, 0.0, 0.0));
#else
	// Subsequent mip reduction: sample previous mip level
	ivec2 src_pos = pos * 2;

	int x0 = min(src_pos.x, params.src_size.x - 1);
	int x1 = min(src_pos.x + 1, params.src_size.x - 1);
	int y0 = min(src_pos.y, params.src_size.y - 1);
	int y1 = min(src_pos.y + 1, params.src_size.y - 1);

	float d00 = texelFetch(source_hiz, ivec2(x0, y0), 0).r;
	float d10 = texelFetch(source_hiz, ivec2(x1, y0), 0).r;
	float d01 = texelFetch(source_hiz, ivec2(x0, y1), 0).r;
	float d11 = texelFetch(source_hiz, ivec2(x1, y1), 0).r;

	float min_d = min(min(d00, d10), min(d01, d11));

	imageStore(dest_hiz, pos, vec4(min_d, 0.0, 0.0, 0.0));
#endif
}

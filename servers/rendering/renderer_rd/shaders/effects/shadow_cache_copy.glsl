#[vertex]

#version 450

#VERSION_DEFINES

layout(push_constant, std430) uniform Params {
	ivec2 dst_origin;
	ivec2 src_offset;
	ivec2 src_size;
	float clear_depth;
	float pad;
}
params;

void main() {
	vec2 base_arr[4] = vec2[](vec2(0.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(1.0, 0.0));
	gl_Position = vec4(base_arr[gl_VertexIndex] * 2.0 - 1.0, 0.0, 1.0);
}

#[fragment]

#version 450

#VERSION_DEFINES

// Copies depth, texel for texel, from one shadow map to a region of another:
// a directional shadow cascade's cache into the atlas, or back. Texels of the
// region with nothing under them in the source (the cache moved) get
// clear_depth.

layout(set = 0, binding = 0) uniform sampler2D source_depth;

layout(push_constant, std430) uniform Params {
	ivec2 dst_origin;
	ivec2 src_offset;
	ivec2 src_size;
	float clear_depth;
	float pad;
}
params;

void main() {
	ivec2 src = ivec2(gl_FragCoord.xy) - params.dst_origin + params.src_offset;
	float depth = params.clear_depth;
	if (all(greaterThanEqual(src, ivec2(0))) && all(lessThan(src, params.src_size))) {
		depth = texelFetch(source_depth, src, 0).r;
	}
	gl_FragDepth = depth;
}

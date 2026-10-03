#[vertex]

#version 450

#VERSION_DEFINES

void main() {
	vec2 base_arr[3] = vec2[](vec2(-1.0, -1.0), vec2(-1.0, 3.0), vec2(3.0, -1.0));
	gl_Position = vec4(base_arr[gl_VertexIndex], 0.0, 1.0);
}

#[fragment]

#version 450

#VERSION_DEFINES

#ifdef USE_MULTIVIEW
#extension GL_EXT_multiview : enable
#define ViewIndex gl_ViewIndex
#endif // USE_MULTIVIEW

// What the order-independent transparent surfaces left: their colors, summed
// with their weights, and how much is seen through all of them.
#ifdef USE_MULTIVIEW
layout(set = 0, binding = 0) uniform sampler2DArray source_accumulation;
layout(set = 0, binding = 1) uniform sampler2DArray source_revealage;
#else // USE_MULTIVIEW
layout(set = 0, binding = 0) uniform sampler2D source_accumulation;
layout(set = 0, binding = 1) uniform sampler2D source_revealage;
#endif // USE_MULTIVIEW

layout(location = 0) out vec4 frag_color;

void main() {
#ifdef USE_MULTIVIEW
	ivec3 pos = ivec3(ivec2(gl_FragCoord.xy), ViewIndex);
#else // USE_MULTIVIEW
	ivec2 pos = ivec2(gl_FragCoord.xy);
#endif // USE_MULTIVIEW

	float revealage = texelFetch(source_revealage, pos, 0).r;
	if (revealage >= 1.0) {
		// Nothing of them here.
		discard;
	}

	vec4 accumulation = texelFetch(source_accumulation, pos, 0);
	if (isinf(max(accumulation.r, max(accumulation.g, accumulation.b)))) {
		// Too bright for the buffer: the average is lost, so keep it white.
		accumulation.rgb = vec3(accumulation.a);
	}

	// Their weighted average over what is behind, mixed in by the pipeline.
	frag_color = vec4(accumulation.rgb / max(accumulation.a, 1e-5), 1.0 - revealage);
}

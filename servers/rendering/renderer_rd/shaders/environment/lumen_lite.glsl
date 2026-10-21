#[compute]

#version 450

#VERSION_DEFINES

// Lumen Lite: screen-space probes that gather HDDAGI's world (see doc/awidot/lumen_lite.md).
//
// MODE_TRACE: one work group per probe, one thread per texel of its 8x8 octahedral
// radiance map. Places the probe in its tile, traces one ray per texel (screen
// space first, then HDDAGI's voxels, then HDDAGI's probes as a radiance cache)
// and blends it with last frame's radiance of the same surface.
// MODE_FILTER: same layout. Averages each texel with the neighbouring probes and
// projects the result on L1 spherical harmonics.
// MODE_GATHER: one thread per GI pixel. Interpolates the probes' irradiance at
// the pixel's normal and writes it to the GI ambient buffer.
// MODE_DEBUG: one thread per pixel, draws the debug views.

#define PROBE_OCT_SIZE 8
#define GROUP_SIZE 8

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;

#define M_PI 3.14159265359

#define FLAG_SCREEN_TRACES 1
#define FLAG_SPATIAL_FILTER 2
#define FLAG_HALF_RAYS 4
#define FLAG_HISTORY_VALID 8

#define HIT_NONE 0.0
#define HIT_SCREEN 1.0
#define HIT_WORLD 2.0
#define HIT_CACHE 3.0

#define DEBUG_PROBES 0
#define DEBUG_TRACE_HITS 1
#define DEBUG_RADIANCE_CACHE 2

layout(set = 0, binding = 0, std140) uniform LumenLiteData {
	mat4 projection; // View to clip, depth corrected.
	mat4 inv_projection;
	mat4 cam_basis; // View to camera-relative world (no translation).
	mat4 inv_cam_basis;
	mat4 prev_reprojection; // Camera-relative world of this frame to last frame's clip.
	vec4 cam_origin;

	ivec2 screen_size;
	ivec2 probe_count;

	uint tile_size;
	uint frame;
	uint screen_steps;
	uint flags;

	float screen_trace_distance;
	float history_alpha; // Weight of the new sample against the history.
	float pixel_angle; // Size of a pixel one unit away from the camera.
	uint pad;
}
params;

#define HDDAGI_MAX_CASCADES 8
#define REGION_SIZE 8
#define PROBE_CELLS 8

layout(rg32ui, set = 0, binding = 1) uniform restrict readonly uimage3D voxel_cascades;
layout(r8ui, set = 0, binding = 2) uniform restrict readonly uimage3D voxel_region_cascades;
layout(set = 0, binding = 3) uniform texture3D light_cascades;
layout(set = 0, binding = 4) uniform texture2DArray lightprobe_specular;
layout(set = 0, binding = 5) uniform texture3D occlusion[2];
layout(set = 0, binding = 6) uniform sampler linear_sampler;

struct ProbeCascadeData {
	vec3 position;
	float to_probe;

	ivec3 region_world_offset;
	float to_cell; // 1/bounds * grid_size

	vec3 pad;
	float exposure_normalization;

	uvec4 pad2;
};

layout(set = 0, binding = 7, std140) uniform HDDAGI {
	ivec3 grid_size;
	uint max_cascades;

	float normal_bias;
	float energy;
	float y_mult;
	float reflection_bias;

	ivec3 probe_axis_size;
	float esm_strength;

	float last_cascade_fade;
	uint pad3_0;
	uint pad3_1;
	uint pad3_2;

	ProbeCascadeData cascades[HDDAGI_MAX_CASCADES];
}
hddagi;

layout(set = 0, binding = 8) uniform texture2D depth_buffer;
layout(set = 0, binding = 9) uniform texture2D normal_roughness_buffer;
layout(set = 0, binding = 10) uniform texture2D last_lit; // Last frame's lit opaque scene, half size.

layout(rgba32f, set = 0, binding = 11) uniform restrict image2D probe_position; // xyz: world position, w: depth (0 without a probe).
layout(rgba16f, set = 0, binding = 12) uniform restrict image2D probe_normal; // xyz: world normal, w: probe pixel in its tile (x + y * 16).
layout(rgba32f, set = 0, binding = 13) uniform restrict readonly image2D prev_probe_position;
layout(rgba16f, set = 0, binding = 14) uniform restrict readonly image2D prev_probe_normal;
layout(rgba16f, set = 0, binding = 15) uniform restrict image2D radiance_trace; // a: how the ray ended (HIT_*).
layout(rgba16f, set = 0, binding = 16) uniform restrict readonly image2D radiance_history;
layout(rgba16f, set = 0, binding = 17) uniform restrict image2D radiance_filtered;
layout(rgba16f, set = 0, binding = 18) uniform restrict image2D probe_sh; // 3 texels per probe: R, G, B (L0, L1 y, z, x).
layout(r32ui, set = 0, binding = 19) uniform restrict writeonly uimage2D ambient_buffer;
layout(rg8, set = 0, binding = 20) uniform restrict image2D blend_buffer;
layout(rgba16f, set = 0, binding = 21) uniform restrict writeonly image2D debug_image;

layout(push_constant, std430) uniform Push {
	ivec2 gi_size;
	uint res_shift;
	uint debug_mode;
}
push;

/* Helpers */

uint hash(uint x) {
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
	return x;
}

float hash_float(uint x) {
	return float(hash(x) & 0xFFFFFF) / float(0x1000000);
}

bool bayer_dither(float value) {
	uvec2 dt = gl_GlobalInvocationID.xy & 0x1;
	uint index = dt.x + dt.y * 2;
	const float table[4] = float[](0.0, 0.5, 0.75, 0.25);
	return value > table[index];
}

vec2 octahedron_wrap(vec2 v) {
	vec2 signVal;
	signVal.x = v.x >= 0.0 ? 1.0 : -1.0;
	signVal.y = v.y >= 0.0 ? 1.0 : -1.0;
	return (1.0 - abs(v.yx)) * signVal;
}

// Direction to [0, 1]².
vec2 octahedron_encode(vec3 n) {
	n /= (abs(n.x) + abs(n.y) + abs(n.z));
	n.xy = n.z >= 0.0 ? n.xy : octahedron_wrap(n.xy);
	n.xy = n.xy * 0.5 + 0.5;
	return n.xy;
}

vec3 octahedron_decode(vec2 f) {
	f = f * 2.0 - 1.0;
	vec3 n = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
	float t = max(-n.z, 0.0);
	n.x += n.x >= 0.0 ? -t : t;
	n.y += n.y >= 0.0 ? -t : t;
	return normalize(n);
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

vec2 pixel_to_ndc(vec2 p_pixel) {
	return (p_pixel + 0.5) / vec2(params.screen_size) * 2.0 - 1.0;
}

// A pixel with geometry gives its view-space position and normal.
bool fetch_surface(ivec2 p_pixel, out vec3 r_view_pos, out vec3 r_view_normal) {
	float depth = texelFetch(sampler2D(depth_buffer, linear_sampler), p_pixel, 0).r;
	vec3 raw_normal = texelFetch(sampler2D(normal_roughness_buffer, linear_sampler), p_pixel, 0).xyz;
	if (depth <= 0.0 || dot(raw_normal, raw_normal) < 0.0625) {
		return false; // Sky (reverse Z), or nothing written.
	}
	vec4 pos = params.inv_projection * vec4(pixel_to_ndc(vec2(p_pixel)), depth, 1.0);
	r_view_pos = pos.xyz / pos.w;
	r_view_normal = normalize(raw_normal * 2.0 - 1.0);
	return true;
}

vec3 view_to_world_dir(vec3 p_view) {
	return mat3(params.cam_basis) * p_view;
}

/* HDDAGI world: voxels and probes */

#include "hddagi_trace_inc.glsl"

ivec3 modi(ivec3 value, ivec3 p_y) {
	return mix(value % p_y, p_y - ((abs(value) - ivec3(1)) % p_y) - 1, lessThan(sign(value), ivec3(0)));
}

ivec2 probe_to_tex(ivec3 local_probe, int p_cascade) {
	ivec3 cell = modi(hddagi.cascades[p_cascade].region_world_offset + local_probe, hddagi.probe_axis_size);
	return cell.xy + ivec2(0, cell.z * int(hddagi.probe_axis_size.y));
}

// Camera-relative world to HDDAGI's space (its cascades may be squashed in Y).
vec3 to_hddagi_space(vec3 p_pos) {
	return p_pos * vec3(1.0, hddagi.y_mult, 1.0);
}

int find_cascade(vec3 p_pos) {
	for (int i = 0; i < int(hddagi.max_cascades); i++) {
		vec3 cascade_pos = (p_pos - hddagi.cascades[i].position) * hddagi.cascades[i].to_cell;
		if (all(greaterThanEqual(cascade_pos, vec3(0.0))) && all(lessThan(cascade_pos, vec3(hddagi.grid_size)))) {
			return i;
		}
	}
	return -1;
}

// The radiance HDDAGI's probes saw in a direction, around a point: what distant
// hits, misses and the sky fall back to.
vec3 radiance_cache_sample(vec3 p_pos, vec3 p_dir) {
	int cascade = find_cascade(p_pos);
	if (cascade < 0) {
		cascade = int(hddagi.max_cascades) - 1;
	}

	vec3 posf = (p_pos - hddagi.cascades[cascade].position) * hddagi.cascades[cascade].to_cell;
	posf = clamp(posf, vec3(0.0), vec3(hddagi.grid_size) - vec3(0.001));

	ivec3 posi = ivec3(posf);
	ivec3 base_probe = posi / PROBE_CELLS;

	ivec3 occ_pos = posi;
	vec3 pos_fract = posf - vec3(posi);
	occ_pos = (occ_pos + hddagi.cascades[cascade].region_world_offset * PROBE_CELLS) & (hddagi.grid_size - 1);
	occ_pos.y += (hddagi.grid_size.y + 2) * cascade;
	occ_pos += ivec3(1);
	ivec3 occ_total_size = hddagi.grid_size + ivec3(2);
	occ_total_size.y *= int(hddagi.max_cascades);
	vec3 occ_posf = (vec3(occ_pos) + pos_fract) / vec3(occ_total_size);

	vec4 occ_0 = texture(sampler3D(occlusion[0], linear_sampler), occ_posf);
	vec4 occ_1 = texture(sampler3D(occlusion[1], linear_sampler), occ_posf);
	float occ_weights[8] = float[](occ_0.x, occ_0.y, occ_0.z, occ_0.w, occ_1.x, occ_1.y, occ_1.z, occ_1.w);

	vec2 light_probe_tex_to_uv = 1.0 / vec2((LIGHTPROBE_OCT_SIZE + 2) * hddagi.probe_axis_size.x, (LIGHTPROBE_OCT_SIZE + 2) * hddagi.probe_axis_size.y * hddagi.probe_axis_size.z);
	vec2 light_uv = octahedron_encode(p_dir) * float(LIGHTPROBE_OCT_SIZE);

	vec3 accum = vec3(0.0);
	float weight_accum = 0.0;

	for (int i = 0; i < 8; i++) {
		ivec3 probe = base_probe + ((ivec3(i) >> ivec3(0, 1, 2)) & ivec3(1, 1, 1));
		vec3 probe_to_pos = posf - vec3(probe * PROBE_CELLS);

		ivec3 probe_occ = (hddagi.cascades[cascade].region_world_offset + probe) & ivec3(1);
		uint weight_index = uint(probe_occ.x) | (uint(probe_occ.y) << 1) | (uint(probe_occ.z) << 2);

		vec3 trilinear = vec3(1.0) - abs(probe_to_pos / float(PROBE_CELLS));
		float weight = trilinear.x * trilinear.y * trilinear.z * max(0.05, occ_weights[weight_index]);

		ivec2 tex_pos = probe_to_tex(probe, cascade);
		vec2 tex_uv = (vec2(tex_pos * (LIGHTPROBE_OCT_SIZE + 2) + ivec2(1)) + light_uv) * light_probe_tex_to_uv;
		accum += texture(sampler2DArray(lightprobe_specular, linear_sampler), vec3(tex_uv, float(cascade))).rgb * weight;
		weight_accum += weight;
	}

	return weight_accum > 0.0 ? accum / weight_accum : vec3(0.0);
}

// A ray through HDDAGI's voxels; returns HIT_WORLD with the light of the hit, or
// HIT_CACHE with the radiance cache's.
// p_skip: how far along the ray is already known to be free (the screen trace saw
// nothing there); starting past it keeps rays from corners out of the next wall.
float world_trace(vec3 p_pos, vec3 p_normal, vec3 p_dir, float p_skip, out vec3 r_radiance) {
	vec3 pos = to_hddagi_space(p_pos + p_dir * p_skip);
	vec3 normal = normalize(to_hddagi_space(p_normal));
	vec3 dir = normalize(to_hddagi_space(p_dir));

	int cascade = find_cascade(pos);
	if (cascade < 0) {
		r_radiance = radiance_cache_sample(pos, dir);
		return HIT_CACHE;
	}

	// Out of the cell of the surface the probe is on, as HDDAGI's reflections do; less
	// so once the ray is already away from it, so that in a corner it doesn't start in
	// the other wall.
	vec3 start_cell = (pos - hddagi.cascades[cascade].position) * hddagi.cascades[cascade].to_cell;
	vec3 abs_normal = abs(normal);
	float skip_cells = p_skip * hddagi.cascades[cascade].to_cell;
	float bias = mix(hddagi.reflection_bias, 0.5, clamp(skip_cells - 1.0, 0.0, 1.0));
	start_cell += normal / max(abs_normal.x, max(abs_normal.y, abs_normal.z)) * bias;
	vec3 ray_pos = start_cell / hddagi.cascades[cascade].to_cell + hddagi.cascades[cascade].position;

	ivec3 hit_cell;
	ivec3 hit_face;
	int hit_cascade;
	if (trace_ray_hdda(ray_pos, dir, cascade, hit_cell, hit_face, hit_cascade)) {
		if (hit_cascade == cascade && ivec3(start_cell) == hit_cell) {
			// Started inside a solid cell (a corner): step out along the ray and try again.
			vec3 retry_pos = ray_pos + dir * (1.5 / hddagi.cascades[cascade].to_cell);
			start_cell = (retry_pos - hddagi.cascades[cascade].position) * hddagi.cascades[cascade].to_cell;
			if (!trace_ray_hdda(retry_pos, dir, cascade, hit_cell, hit_face, hit_cascade)) {
				r_radiance = radiance_cache_sample(retry_pos, dir);
				return HIT_CACHE;
			}
			ray_pos = retry_pos;
		}
		if (hit_cascade != cascade || ivec3(start_cell) != hit_cell) {
			hit_cell += hit_face;
			ivec3 read_cell = (hit_cell + (hddagi.cascades[hit_cascade].region_world_offset * REGION_SIZE)) & (ivec3(hddagi.grid_size) - 1);
			r_radiance = texelFetch(sampler3D(light_cascades, linear_sampler), read_cell + ivec3(0, (hddagi.grid_size.y * hit_cascade), 0), 0).rgb;
			return HIT_WORLD;
		}
	}

	r_radiance = radiance_cache_sample(ray_pos, dir);
	return HIT_CACHE;
}

/* Screen probes */

vec3 sh_irradiance(ivec2 p_probe, vec3 p_normal) {
	// L1 spherical harmonics, convolved with the clamped cosine: A0 = pi, A1 = 2pi/3.
	vec4 basis = vec4(0.282095 * M_PI, vec3(0.488603 * 2.0 * M_PI / 3.0) * p_normal.yzx);
	vec3 irradiance;
	irradiance.r = dot(imageLoad(probe_sh, ivec2(p_probe.x * 3 + 0, p_probe.y)), basis);
	irradiance.g = dot(imageLoad(probe_sh, ivec2(p_probe.x * 3 + 1, p_probe.y)), basis);
	irradiance.b = dot(imageLoad(probe_sh, ivec2(p_probe.x * 3 + 2, p_probe.y)), basis);
	return max(irradiance, vec3(0.0));
}

#ifdef MODE_TRACE

shared vec3 probe_view_pos;
shared vec3 probe_view_normal;
shared bool probe_valid;
shared ivec2 history_probe;
shared bool history_valid;

// Marches the depth buffer; on a hit, reads last frame's lit scene there.
// r_free: how far the ray went without passing behind anything.
bool screen_trace(vec3 p_origin, vec3 p_dir, float p_jitter, out vec3 r_radiance, out float r_free) {
	float max_distance = params.screen_trace_distance;
	uint steps = params.screen_steps;
	bool free = true;
	r_free = 0.0;
	for (uint i = 0; i < steps; i++) {
		float s = (float(i) + p_jitter) / float(steps);
		float t = max_distance * s * s;
		vec3 p = p_origin + p_dir * t;

		vec4 clip = params.projection * vec4(p, 1.0);
		if (clip.w <= 0.0) {
			return false;
		}
		vec2 ndc = clip.xy / clip.w;
		vec2 pixelf = (ndc * 0.5 + 0.5) * vec2(params.screen_size) - 0.5;
		if (any(lessThan(pixelf, vec2(0.0))) || any(greaterThanEqual(pixelf, vec2(params.screen_size)))) {
			return false;
		}
		ivec2 pixel = ivec2(pixelf + 0.5);

		float depth = texelFetch(sampler2D(depth_buffer, linear_sampler), pixel, 0).r;
		if (depth <= 0.0) {
			continue; // Sky.
		}
		vec4 scene = params.inv_projection * vec4(pixel_to_ndc(vec2(pixel)), depth, 1.0);
		scene.xyz /= scene.w;

		float thickness = max(0.1, t * 0.25);
		float behind = scene.z - p.z; // View Z is negative: the ray is behind the surface when positive.
		if (behind > 0.0 && behind < thickness) {
			vec4 prev_clip = params.prev_reprojection * vec4(view_to_world_dir(scene.xyz), 1.0);
			if (prev_clip.w <= 0.0) {
				return false;
			}
			vec2 prev_uv = (prev_clip.xy / prev_clip.w) * 0.5 + 0.5;
			if (any(lessThan(prev_uv, vec2(0.0))) || any(greaterThan(prev_uv, vec2(1.0)))) {
				return false;
			}
			r_radiance = textureLod(sampler2D(last_lit, linear_sampler), prev_uv, 0.0).rgb;
			return true;
		}
		if (behind > 0.0) {
			free = false; // Behind something thick: what lies further is unknown.
		} else if (free) {
			r_free = t;
		}
	}
	return false;
}

void main() {
	ivec2 probe = ivec2(gl_WorkGroupID.xy);
	ivec2 texel = ivec2(gl_LocalInvocationID.xy);
	ivec2 radiance_pos = probe * PROBE_OCT_SIZE + texel;
	uint tile_size = params.tile_size;

	if (gl_LocalInvocationIndex == 0) {
		// Where in its tile the probe goes this frame; tries a few pixels to stay off the sky.
		probe_valid = false;
		history_valid = false;
		ivec2 tile_origin = probe * int(tile_size);
		uint seed = hash(uint(probe.x) * 1973u + uint(probe.y) * 9277u + params.frame * 26699u);
		for (uint i = 0; i < 4 && !probe_valid; i++) {
			uint h = hash(seed + i * 7919u);
			ivec2 local = ivec2(h % tile_size, (h >> 8) % tile_size);
			if (i == 3) {
				local = ivec2(tile_size / 2);
			}
			ivec2 pixel = min(tile_origin + local, params.screen_size - 1);
			vec3 view_pos, view_normal;
			if (fetch_surface(pixel, view_pos, view_normal)) {
				probe_valid = true;
				probe_view_pos = view_pos;
				probe_view_normal = view_normal;

				vec3 world_pos = view_to_world_dir(view_pos);
				vec3 world_normal = normalize(view_to_world_dir(view_normal));
				float depth = -view_pos.z;
				imageStore(probe_position, probe, vec4(world_pos + params.cam_origin.xyz, depth));
				imageStore(probe_normal, probe, vec4(world_normal, float((pixel.x - tile_origin.x) + (pixel.y - tile_origin.y) * 16)));

				// The same surface in last frame's probe of where this one was.
				if (bool(params.flags & FLAG_HISTORY_VALID)) {
					vec4 prev_clip = params.prev_reprojection * vec4(world_pos, 1.0);
					if (prev_clip.w > 0.0) {
						vec2 prev_pixel = ((prev_clip.xy / prev_clip.w) * 0.5 + 0.5) * vec2(params.screen_size);
						ivec2 prev_probe = ivec2(floor(prev_pixel / float(tile_size)));
						if (all(greaterThanEqual(prev_probe, ivec2(0))) && all(lessThan(prev_probe, params.probe_count))) {
							vec4 prev_pos = imageLoad(prev_probe_position, prev_probe);
							vec3 prev_normal = imageLoad(prev_probe_normal, prev_probe).xyz;
							vec3 offset = prev_pos.xyz - (world_pos + params.cam_origin.xyz);
							// A tile seen at a grazing angle covers that much more of its surface.
							float facing = max(abs(dot(world_normal, normalize(world_pos))), 0.15);
							float tile_world_size = float(tile_size) * depth * params.pixel_angle / facing;
							bool same_plane = abs(dot(offset, world_normal)) < 0.05 * depth / facing + 0.02;
							bool close = length(offset) < tile_world_size * 1.5 + 0.05;
							if (prev_pos.w > 0.0 && same_plane && close && dot(prev_normal, world_normal) > 0.8) {
								history_valid = true;
								history_probe = prev_probe;
							}
						}
					}
				}
			}
		}
		if (!probe_valid) {
			imageStore(probe_position, probe, vec4(0.0));
			imageStore(probe_normal, probe, vec4(0.0));
		}
	}

	memoryBarrierShared();
	barrier();

	if (!probe_valid) {
		imageStore(radiance_trace, radiance_pos, vec4(0.0));
		return;
	}

	vec3 world_normal = normalize(view_to_world_dir(probe_view_normal));
	vec3 world_pos = view_to_world_dir(probe_view_pos);

	uint ray_seed = hash(uint(radiance_pos.x) * 7369u + uint(radiance_pos.y) * 1237u + params.frame * 4783u);
	vec2 jitter = vec2(hash_float(ray_seed), hash_float(ray_seed + 1u));
	vec3 dir = octahedron_decode((vec2(texel) + jitter) / float(PROBE_OCT_SIZE));

	vec4 history = vec4(0.0);
	if (history_valid) {
		history = imageLoad(radiance_history, history_probe * PROBE_OCT_SIZE + texel);
	}

	if (dot(dir, world_normal) <= 0.0) {
		// Below the surface: nothing to trace, and no light to keep.
		imageStore(radiance_trace, radiance_pos, vec4(0.0));
		return;
	}

	if (bool(params.flags & FLAG_HALF_RAYS) && ((texel.x + texel.y + int(params.frame)) & 1) == 1 && history.a > 0.0) {
		// Checkerboard: half of the texels keep their history this frame.
		imageStore(radiance_trace, radiance_pos, history);
		return;
	}

	vec3 radiance = vec3(0.0);
	float hit = HIT_NONE;
	float free_distance = 0.0;

	if (bool(params.flags & FLAG_SCREEN_TRACES)) {
		float depth = -probe_view_pos.z;
		vec3 origin = probe_view_pos + probe_view_normal * (0.01 + depth * 0.005);
		vec3 view_dir = mat3(params.inv_cam_basis) * dir;
		if (screen_trace(origin, view_dir, hash_float(ray_seed + 2u), radiance, free_distance)) {
			hit = HIT_SCREEN;
		}
	}

	if (hit == HIT_NONE) {
		hit = world_trace(world_pos, world_normal, dir, free_distance, radiance);
	}

	// Keep a single bright texel from flashing the whole probe.
	float luminance = dot(radiance, vec3(0.2126, 0.7152, 0.0722));
	radiance *= min(1.0, 16.0 / max(luminance, 0.0001));

	if (history.a > 0.0) {
		radiance = mix(history.rgb, radiance, params.history_alpha);
	}

	imageStore(radiance_trace, radiance_pos, vec4(radiance, hit));
}

#endif // MODE_TRACE

#ifdef MODE_FILTER

shared vec4 sh_r[64];
shared vec4 sh_g[64];
shared vec4 sh_b[64];

void main() {
	ivec2 probe = ivec2(gl_WorkGroupID.xy);
	ivec2 texel = ivec2(gl_LocalInvocationID.xy);
	ivec2 radiance_pos = probe * PROBE_OCT_SIZE + texel;
	uint index = gl_LocalInvocationIndex;

	vec4 center_pos = imageLoad(probe_position, probe);
	vec4 radiance = imageLoad(radiance_trace, radiance_pos);

	if (center_pos.w > 0.0 && radiance.a > 0.0 && bool(params.flags & FLAG_SPATIAL_FILTER)) {
		vec3 center_normal = imageLoad(probe_normal, probe).xyz;
		vec3 accum = radiance.rgb;
		float weight_accum = 1.0;
		// The 3x3 probes around, the diagonal ones at half the weight of the sides, so that
		// an outlier spreads as a soft blob and not as a cross.
		for (int i = 0; i < 9; i++) {
			ivec2 offset = ivec2(i % 3, i / 3) - ivec2(1);
			if (i == 4) {
				continue;
			}
			float neighbour_weight = (offset.x != 0 && offset.y != 0) ? 0.25 : 0.5;
			ivec2 n = probe + offset;
			if (any(lessThan(n, ivec2(0))) || any(greaterThanEqual(n, params.probe_count))) {
				continue;
			}
			vec4 n_pos = imageLoad(probe_position, n);
			if (n_pos.w <= 0.0) {
				continue;
			}
			vec3 n_normal = imageLoad(probe_normal, n).xyz;
			float plane_distance = abs(dot(n_pos.xyz - center_pos.xyz, center_normal));
			if (plane_distance > 0.05 * center_pos.w + 0.05 || dot(n_normal, center_normal) < 0.7) {
				continue;
			}
			vec4 n_radiance = imageLoad(radiance_trace, n * PROBE_OCT_SIZE + texel);
			if (n_radiance.a > 0.0) {
				accum += n_radiance.rgb * neighbour_weight;
				weight_accum += neighbour_weight;
			}
		}
		radiance.rgb = accum / weight_accum;
	}

	imageStore(radiance_filtered, radiance_pos, radiance);

	// Project on L1 spherical harmonics; every texel covers about 4pi/64 sr.
	vec4 r = vec4(0.0), g = vec4(0.0), b = vec4(0.0);
	if (radiance.a > 0.0) {
		vec3 dir = octahedron_decode((vec2(texel) + 0.5) / float(PROBE_OCT_SIZE));
		vec4 basis = vec4(0.282095, 0.488603 * dir.yzx) * (4.0 * M_PI / 64.0);
		r = basis * radiance.r;
		g = basis * radiance.g;
		b = basis * radiance.b;
	}
	sh_r[index] = r;
	sh_g[index] = g;
	sh_b[index] = b;

	memoryBarrierShared();
	barrier();

	for (uint s = 32; s > 0; s >>= 1) {
		if (index < s) {
			sh_r[index] += sh_r[index + s];
			sh_g[index] += sh_g[index + s];
			sh_b[index] += sh_b[index + s];
		}
		memoryBarrierShared();
		barrier();
	}

	if (index == 0) {
		imageStore(probe_sh, ivec2(probe.x * 3 + 0, probe.y), sh_r[0]);
		imageStore(probe_sh, ivec2(probe.x * 3 + 1, probe.y), sh_g[0]);
		imageStore(probe_sh, ivec2(probe.x * 3 + 2, probe.y), sh_b[0]);
	}
}

#endif // MODE_FILTER

#ifdef MODE_GATHER

void main() {
	ivec2 gi_pos = ivec2(gl_GlobalInvocationID.xy);
	if (any(greaterThanEqual(gi_pos, push.gi_size))) {
		return;
	}
	ivec2 pixel = min(gi_pos << push.res_shift, params.screen_size - 1);

	vec3 view_pos, view_normal;
	if (!fetch_surface(pixel, view_pos, view_normal)) {
		return;
	}
	vec3 world_pos = view_to_world_dir(view_pos) + params.cam_origin.xyz;
	vec3 normal = normalize(view_to_world_dir(view_normal));
	float depth = -view_pos.z;

	vec2 tile_f = (vec2(pixel) + 0.5) / float(params.tile_size) - 0.5;
	ivec2 base = ivec2(floor(tile_f));
	vec2 f = tile_f - vec2(base);

	vec3 irradiance = vec3(0.0);
	float weight_accum = 0.0;

	for (int i = 0; i < 4; i++) {
		ivec2 offset = ivec2(i & 1, i >> 1);
		ivec2 probe = clamp(base + offset, ivec2(0), params.probe_count - 1);
		vec4 p = imageLoad(probe_position, probe);
		if (p.w <= 0.0) {
			continue;
		}
		vec3 p_normal = imageLoad(probe_normal, probe).xyz;
		vec2 bilinear = mix(1.0 - f, f, vec2(offset));
		float weight = bilinear.x * bilinear.y;
		float plane_distance = abs(dot(p.xyz - world_pos, normal));
		weight *= exp(-plane_distance / (0.02 * depth + 0.02));
		weight *= pow(max(dot(normal, p_normal), 0.0), 4.0);
		if (weight > 0.0) {
			irradiance += sh_irradiance(probe, normal) * weight;
			weight_accum += weight;
		}
	}

	if (weight_accum < 0.001) {
		// Nothing fits around: any probe of the 3x3 tiles that faces the same way.
		irradiance = vec3(0.0);
		weight_accum = 0.0;
		ivec2 center = ivec2(vec2(pixel) / float(params.tile_size));
		for (int y = -1; y <= 1; y++) {
			for (int x = -1; x <= 1; x++) {
				ivec2 probe = clamp(center + ivec2(x, y), ivec2(0), params.probe_count - 1);
				vec4 p = imageLoad(probe_position, probe);
				if (p.w <= 0.0) {
					continue;
				}
				vec3 p_normal = imageLoad(probe_normal, probe).xyz;
				float plane_distance = abs(dot(p.xyz - world_pos, normal));
				float weight = pow(max(dot(normal, p_normal), 0.0), 4.0) / (1.0 + plane_distance * 4.0);
				if (weight > 0.0) {
					irradiance += sh_irradiance(probe, normal) * weight;
					weight_accum += weight;
				}
			}
		}
		if (weight_accum < 0.0001) {
			return; // HDDAGI's own gather stays.
		}
	}

	vec3 ambient = irradiance / weight_accum / M_PI * hddagi.energy;
	imageStore(ambient_buffer, gi_pos, uvec4(rgbe_encode(ambient)));
	vec4 blend = imageLoad(blend_buffer, gi_pos);
	imageStore(blend_buffer, gi_pos, vec4(1.0, blend.g, 0.0, 0.0));
}

#endif // MODE_GATHER

#ifdef MODE_DEBUG

void main() {
	ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
	if (any(greaterThanEqual(pixel, params.screen_size))) {
		return;
	}

	vec3 color = vec3(0.0);

	if (push.debug_mode == DEBUG_RADIANCE_CACHE) {
		vec3 view_pos, view_normal;
		if (fetch_surface(pixel, view_pos, view_normal)) {
			vec3 pos = to_hddagi_space(view_to_world_dir(view_pos));
			vec3 normal = normalize(to_hddagi_space(view_to_world_dir(view_normal)));
			int cascade = find_cascade(pos);
			float cell_size = cascade >= 0 ? 1.0 / hddagi.cascades[cascade].to_cell : 1.0;
			color = radiance_cache_sample(pos + normal * cell_size, normal) * hddagi.energy;
		}
	} else {
		uint tile_size = params.tile_size;
		ivec2 probe = pixel / int(tile_size);
		ivec2 local = pixel - probe * int(tile_size);
		if (all(lessThan(probe, params.probe_count)) && imageLoad(probe_position, probe).w > 0.0) {
			ivec2 texel = local * PROBE_OCT_SIZE / int(tile_size);
			ivec2 radiance_pos = probe * PROBE_OCT_SIZE + texel;
			if (push.debug_mode == DEBUG_PROBES) {
				color = imageLoad(radiance_filtered, radiance_pos).rgb;
				int probe_pixel = int(imageLoad(probe_normal, probe).w + 0.5);
				if (all(equal(local, ivec2(probe_pixel % 16, probe_pixel / 16)))) {
					color = vec3(1.0, 0.0, 1.0);
				}
			} else {
				float hit = imageLoad(radiance_trace, radiance_pos).a;
				const vec3 hit_colors[4] = vec3[](vec3(0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.25, 1.0), vec3(1.0, 0.0, 0.0));
				color = hit_colors[clamp(int(hit + 0.5), 0, 3)];
			}
			if (any(equal(local, ivec2(0)))) {
				color *= 0.5; // Tile borders.
			}
		}
	}

	imageStore(debug_image, pixel, vec4(color, 1.0));
}

#endif // MODE_DEBUG

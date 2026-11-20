/**************************************************************************/
/*  gpu_culling.h                                                         */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "core/math/projection.h"
#include "core/math/transform_3d.h"
#include "core/templates/local_vector.h"
#include "servers/rendering/renderer_rd/shaders/effects/gpu_culling.glsl.gen.h"
#include "servers/rendering/renderer_rd/shaders/effects/hiz_downsample.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"

#define RB_SCOPE_GPU_CULL SNAME("gpu_cull")
#define RB_TEX_HIZ_BUFFER SNAME("hiz_buffer")

namespace RendererRD {

class GPUCulling {
public:
	struct Stats {
		uint32_t total_instances = 0;
		uint32_t visible_count = 0;
		uint32_t culled_frustum = 0;
		uint32_t culled_occlusion = 0;
	};

	struct InstanceAABB {
		float min_pos[4]; // xyz: min, w: pad
		float max_pos[4]; // xyz: max, w: pad
	};

	struct alignas(16) CullParams {
		float view_proj[16];
		float prev_view_proj[16];
		float cam_pos[4];
		float frustum_planes[6][4];
		float viewport_size[2];
		float z_near;
		float z_far;
		uint32_t total_instances;
		uint32_t hiz_max_mip;
		uint32_t enable_occlusion;
		uint32_t pad;
	};

private:
	static GPUCulling *singleton;

	enum HizMode {
		HIZ_MODE_SUBSEQUENT = 0,
		HIZ_MODE_FIRST_PASS = 1,
		HIZ_MODE_MAX
	};

	struct HizPushConstant {
		int32_t dest_size[2];
		int32_t src_size[2];
	};

	HizDownsampleShaderRD hiz_shader;
	RID hiz_shader_version;
	RID hiz_pipelines[HIZ_MODE_MAX];

	GpuCullingShaderRD cull_shader;
	RID cull_shader_version;
	RID cull_pipeline;

	RID cull_params_ubo;
	RID instance_ssbo;
	RID visibility_ssbo;
	RID stats_ssbo;
	uint32_t current_buffer_capacity = 0;

	Stats last_stats;
	bool enabled = true;

	void _ensure_buffers(uint32_t p_instance_count);

public:
	static GPUCulling *get_singleton() { return singleton; }

	bool is_enabled() const { return enabled; }
	void set_enabled(bool p_enabled) { enabled = p_enabled; }

	Stats get_last_stats() const { return last_stats; }

	// Builds or updates the Hi-Z depth pyramid for the given viewport/scene buffers
	void build_hiz_pyramid(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_depth, const Size2i &p_screen_size, uint32_t p_view = 0);

	// Runs the GPU culling compute shader against the Hi-Z buffer.
	// Fills r_visibility with true for visible, false for culled instances.
	void cull_instances(Ref<RenderSceneBuffersRD> p_render_buffers, const LocalVector<AABB> &p_aabbs, const Projection &p_proj, const Projection &p_prev_proj, const Transform3D &p_cam_transform, const Vector<Plane> &p_frustum_planes, Vector<bool> &r_visibility, bool p_use_occlusion = true);

	GPUCulling();
	~GPUCulling();
};

} // namespace RendererRD

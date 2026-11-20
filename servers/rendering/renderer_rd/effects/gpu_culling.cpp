/**************************************************************************/
/*  gpu_culling.cpp                                                       */
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

#include "gpu_culling.h"

#include "core/config/project_settings.h"
#include "servers/rendering/renderer_rd/storage_rd/material_storage.h"
#include "servers/rendering/renderer_rd/storage_rd/texture_storage.h"
#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"

using namespace RendererRD;

GPUCulling *GPUCulling::singleton = nullptr;

GPUCulling::GPUCulling() {
	singleton = this;
	enabled = GLOBAL_GET("rendering/occlusion_culling/use_gpu_culling");

	// 1. Initialize Hi-Z downsample compute shader
	Vector<String> hiz_modes;
	hiz_modes.push_back("");
	hiz_modes.push_back("\n#define FIRST_PASS\n");
	hiz_shader.initialize(hiz_modes);
	hiz_shader_version = hiz_shader.version_create();
	hiz_pipelines[HIZ_MODE_SUBSEQUENT] = RD::get_singleton()->compute_pipeline_create(hiz_shader.version_get_shader(hiz_shader_version, HIZ_MODE_SUBSEQUENT));
	hiz_pipelines[HIZ_MODE_FIRST_PASS] = RD::get_singleton()->compute_pipeline_create(hiz_shader.version_get_shader(hiz_shader_version, HIZ_MODE_FIRST_PASS));

	// 2. Initialize GPU culling compute shader
	Vector<String> cull_modes;
	cull_modes.push_back("");
	cull_shader.initialize(cull_modes);
	cull_shader_version = cull_shader.version_create();
	cull_pipeline = RD::get_singleton()->compute_pipeline_create(cull_shader.version_get_shader(cull_shader_version, 0));

	cull_params_ubo = RD::get_singleton()->uniform_buffer_create(sizeof(CullParams));
	stats_ssbo = RD::get_singleton()->storage_buffer_create(sizeof(uint32_t) * 3);
}

GPUCulling::~GPUCulling() {
	if (cull_params_ubo.is_valid()) {
		RD::get_singleton()->free_rid(cull_params_ubo);
	}
	if (stats_ssbo.is_valid()) {
		RD::get_singleton()->free_rid(stats_ssbo);
	}
	if (instance_ssbo.is_valid()) {
		RD::get_singleton()->free_rid(instance_ssbo);
	}
	if (visibility_ssbo.is_valid()) {
		RD::get_singleton()->free_rid(visibility_ssbo);
	}

	hiz_shader.version_free(hiz_shader_version);
	cull_shader.version_free(cull_shader_version);

	singleton = nullptr;
}

void GPUCulling::_ensure_buffers(uint32_t p_instance_count) {
	if (p_instance_count <= current_buffer_capacity && instance_ssbo.is_valid() && visibility_ssbo.is_valid()) {
		return;
	}

	if (instance_ssbo.is_valid()) {
		RD::get_singleton()->free_rid(instance_ssbo);
	}
	if (visibility_ssbo.is_valid()) {
		RD::get_singleton()->free_rid(visibility_ssbo);
	}

	current_buffer_capacity = MAX(1024u, (uint32_t)Math::next_power_of_2(p_instance_count));
	instance_ssbo = RD::get_singleton()->storage_buffer_create(current_buffer_capacity * sizeof(InstanceAABB));
	visibility_ssbo = RD::get_singleton()->storage_buffer_create(current_buffer_capacity * sizeof(uint32_t));
}

void GPUCulling::build_hiz_pyramid(Ref<RenderSceneBuffersRD> p_render_buffers, RID p_source_depth, const Size2i &p_screen_size, uint32_t p_view) {
	if (p_render_buffers.is_null() || !p_source_depth.is_valid()) {
		return;
	}

	UniformSetCacheRD *uniform_set_cache = UniformSetCacheRD::get_singleton();
	MaterialStorage *material_storage = MaterialStorage::get_singleton();
	if (!uniform_set_cache || !material_storage) {
		return;
	}

	Size2i hiz_base_size = Size2i(MAX(1, p_screen_size.x / 2), MAX(1, p_screen_size.y / 2));
	int mip_count = MAX(1, Image::get_image_required_mipmaps(hiz_base_size.x, hiz_base_size.y, Image::FORMAT_RF));

	if (!p_render_buffers->has_texture(RB_SCOPE_GPU_CULL, RB_TEX_HIZ_BUFFER)) {
		p_render_buffers->create_texture(
				RB_SCOPE_GPU_CULL,
				RB_TEX_HIZ_BUFFER,
				RD::DATA_FORMAT_R32_SFLOAT,
				RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT,
				RD::TEXTURE_SAMPLES_1,
				hiz_base_size,
				p_render_buffers->get_view_count(),
				mip_count);
	}

	RID nearest_sampler = material_storage->sampler_rd_get_default(RSE::CANVAS_ITEM_TEXTURE_FILTER_NEAREST_WITH_MIPMAPS, RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);

	RD::get_singleton()->draw_command_begin_label("Build Hi-Z Pyramid (GPU)");

	HizPushConstant push_constant;

	// Level 0: downsample from source depth to hiz mip 0
	{
		push_constant.dest_size[0] = hiz_base_size.x;
		push_constant.dest_size[1] = hiz_base_size.y;
		push_constant.src_size[0] = p_screen_size.x;
		push_constant.src_size[1] = p_screen_size.y;

		RID dest_mip0 = p_render_buffers->get_texture_slice(RB_SCOPE_GPU_CULL, RB_TEX_HIZ_BUFFER, p_view, 0);
		RID first_shader = hiz_shader.version_get_shader(hiz_shader_version, HIZ_MODE_FIRST_PASS);

		RD::Uniform u_src(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 0, Vector<RID>{ nearest_sampler, p_source_depth });
		RD::Uniform u_dst(RD::UNIFORM_TYPE_IMAGE, 0, dest_mip0);

		RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, hiz_pipelines[HIZ_MODE_FIRST_PASS]);
		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set_cache->get_cache(first_shader, 0, u_src), 0);
		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set_cache->get_cache(first_shader, 1, u_dst), 1);
		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HizPushConstant));
		RD::get_singleton()->compute_list_dispatch_threads(compute_list, hiz_base_size.x, hiz_base_size.y, 1);
		RD::get_singleton()->compute_list_end();
	}

	// Subsequent levels: mip m-1 -> mip m
	RID sub_shader = hiz_shader.version_get_shader(hiz_shader_version, HIZ_MODE_SUBSEQUENT);
	for (int m = 1; m < mip_count; m++) {
		Size2i src_size = Size2i(MAX(1, hiz_base_size.x >> (m - 1)), MAX(1, hiz_base_size.y >> (m - 1)));
		Size2i dst_size = Size2i(MAX(1, hiz_base_size.x >> m), MAX(1, hiz_base_size.y >> m));

		push_constant.dest_size[0] = dst_size.x;
		push_constant.dest_size[1] = dst_size.y;
		push_constant.src_size[0] = src_size.x;
		push_constant.src_size[1] = src_size.y;

		RID src_mip = p_render_buffers->get_texture_slice(RB_SCOPE_GPU_CULL, RB_TEX_HIZ_BUFFER, p_view, m - 1);
		RID dst_mip = p_render_buffers->get_texture_slice(RB_SCOPE_GPU_CULL, RB_TEX_HIZ_BUFFER, p_view, m);

		RD::Uniform u_src(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 0, Vector<RID>{ nearest_sampler, src_mip });
		RD::Uniform u_dst(RD::UNIFORM_TYPE_IMAGE, 0, dst_mip);

		RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
		RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, hiz_pipelines[HIZ_MODE_SUBSEQUENT]);
		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set_cache->get_cache(sub_shader, 0, u_src), 0);
		RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set_cache->get_cache(sub_shader, 1, u_dst), 1);
		RD::get_singleton()->compute_list_set_push_constant(compute_list, &push_constant, sizeof(HizPushConstant));
		RD::get_singleton()->compute_list_dispatch_threads(compute_list, dst_size.x, dst_size.y, 1);
		RD::get_singleton()->compute_list_end();
	}

	RD::get_singleton()->draw_command_end_label();
}

void GPUCulling::cull_instances(Ref<RenderSceneBuffersRD> p_render_buffers, const LocalVector<AABB> &p_aabbs, const Projection &p_proj, const Projection &p_prev_proj, const Transform3D &p_cam_transform, const Vector<Plane> &p_frustum_planes, Vector<bool> &r_visibility, bool p_use_occlusion) {
	uint32_t count = p_aabbs.size();
	r_visibility.resize(count);
	if (count == 0) {
		last_stats = Stats();
		return;
	}

	UniformSetCacheRD *uniform_set_cache = UniformSetCacheRD::get_singleton();
	MaterialStorage *material_storage = MaterialStorage::get_singleton();
	if (!uniform_set_cache || !material_storage) {
		for (uint32_t i = 0; i < count; i++) {
			r_visibility.write[i] = true;
		}
		return;
	}

	_ensure_buffers(count);

	// Pack AABBs into instance_ssbo:
	Vector<InstanceAABB> packed_aabbs;
	packed_aabbs.resize(count);
	InstanceAABB *ptr = packed_aabbs.ptrw();
	for (uint32_t i = 0; i < count; i++) {
		const AABB &b = p_aabbs[i];
		Vector3 bmin = b.position;
		Vector3 bmax = b.position + b.size;
		ptr[i].min_pos[0] = bmin.x;
		ptr[i].min_pos[1] = bmin.y;
		ptr[i].min_pos[2] = bmin.z;
		ptr[i].min_pos[3] = 0.0f;
		ptr[i].max_pos[0] = bmax.x;
		ptr[i].max_pos[1] = bmax.y;
		ptr[i].max_pos[2] = bmax.z;
		ptr[i].max_pos[3] = 0.0f;
	}
	RD::get_singleton()->buffer_update(instance_ssbo, 0, count * sizeof(InstanceAABB), packed_aabbs.ptr());

	// Reset stats counter buffer:
	uint32_t stats_init[3] = { 0, 0, 0 };
	RD::get_singleton()->buffer_update(stats_ssbo, 0, sizeof(stats_init), stats_init);

	// Setup CullParams:
	CullParams params;
	for (int r = 0; r < 4; r++) {
		for (int c = 0; c < 4; c++) {
			params.view_proj[r * 4 + c] = p_proj.columns[c][r];
			params.prev_view_proj[r * 4 + c] = p_prev_proj.columns[c][r];
		}
	}
	params.cam_pos[0] = p_cam_transform.origin.x;
	params.cam_pos[1] = p_cam_transform.origin.y;
	params.cam_pos[2] = p_cam_transform.origin.z;
	params.cam_pos[3] = 1.0f;

	for (int i = 0; i < 6; i++) {
		if (i < p_frustum_planes.size()) {
			params.frustum_planes[i][0] = p_frustum_planes[i].normal.x;
			params.frustum_planes[i][1] = p_frustum_planes[i].normal.y;
			params.frustum_planes[i][2] = p_frustum_planes[i].normal.z;
			params.frustum_planes[i][3] = p_frustum_planes[i].d;
		} else {
			params.frustum_planes[i][0] = 0.0f;
			params.frustum_planes[i][1] = 0.0f;
			params.frustum_planes[i][2] = 0.0f;
			params.frustum_planes[i][3] = 0.0f;
		}
	}

	Size2i internal_size = p_render_buffers.is_valid() ? p_render_buffers->get_internal_size() : Size2i(1920, 1080);
	params.viewport_size[0] = internal_size.x;
	params.viewport_size[1] = internal_size.y;
	params.z_near = p_proj.get_z_near();
	params.z_far = p_proj.get_z_far();
	params.total_instances = count;

	bool has_hiz = p_use_occlusion && p_render_buffers.is_valid() && p_render_buffers->has_texture(RB_SCOPE_GPU_CULL, RB_TEX_HIZ_BUFFER);
	params.enable_occlusion = has_hiz ? 1 : 0;
	params.hiz_max_mip = has_hiz ? MAX(1, Image::get_image_required_mipmaps(internal_size.x / 2, internal_size.y / 2, Image::FORMAT_RF)) - 1 : 0;
	params.pad = 0;

	RD::get_singleton()->buffer_update(cull_params_ubo, 0, sizeof(CullParams), &params);

	RID linear_sampler = material_storage->sampler_rd_get_default(RSE::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS, RSE::CANVAS_ITEM_TEXTURE_REPEAT_DISABLED);
	RID hiz_tex = has_hiz ? p_render_buffers->get_texture(RB_SCOPE_GPU_CULL, RB_TEX_HIZ_BUFFER) : RID();
	if (hiz_tex.is_null()) {
		hiz_tex = TextureStorage::get_singleton()->texture_rd_get_default(TextureStorage::DEFAULT_RD_TEXTURE_WHITE);
	}

	RID cull_sh = cull_shader.version_get_shader(cull_shader_version, 0);

	RD::Uniform u_params(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 0, cull_params_ubo);
	RD::Uniform u_instances(RD::UNIFORM_TYPE_STORAGE_BUFFER, 1, instance_ssbo);
	RD::Uniform u_visibility(RD::UNIFORM_TYPE_STORAGE_BUFFER, 2, visibility_ssbo);
	RD::Uniform u_stats(RD::UNIFORM_TYPE_STORAGE_BUFFER, 3, stats_ssbo);
	RD::Uniform u_hiz(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 0, Vector<RID>{ linear_sampler, hiz_tex });

	RD::get_singleton()->draw_command_begin_label("GPU Culling Dispatch");

	RD::ComputeListID compute_list = RD::get_singleton()->compute_list_begin();
	RD::get_singleton()->compute_list_bind_compute_pipeline(compute_list, cull_pipeline);
	RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set_cache->get_cache(cull_sh, 0, u_params, u_instances, u_visibility, u_stats), 0);
	RD::get_singleton()->compute_list_bind_uniform_set(compute_list, uniform_set_cache->get_cache(cull_sh, 1, u_hiz), 1);
	RD::get_singleton()->compute_list_dispatch_threads(compute_list, count, 1, 1);
	RD::get_singleton()->compute_list_end();

	RD::get_singleton()->draw_command_end_label();

	// Read back visibility and stats
	Vector<uint8_t> vis_bytes = RD::get_singleton()->buffer_get_data(visibility_ssbo, 0, count * sizeof(uint32_t));
	if (vis_bytes.size() >= (int)(count * sizeof(uint32_t))) {
		const uint32_t *v_ptr = reinterpret_cast<const uint32_t *>(vis_bytes.ptr());
		for (uint32_t i = 0; i < count; i++) {
			r_visibility.write[i] = (v_ptr[i] != 0);
		}
	} else {
		for (uint32_t i = 0; i < count; i++) {
			r_visibility.write[i] = true;
		}
	}

	Vector<uint8_t> stats_bytes = RD::get_singleton()->buffer_get_data(stats_ssbo, 0, sizeof(stats_init));
	if (stats_bytes.size() >= (int)sizeof(stats_init)) {
		const uint32_t *s_ptr = reinterpret_cast<const uint32_t *>(stats_bytes.ptr());
		last_stats.total_instances = count;
		last_stats.visible_count = s_ptr[0];
		last_stats.culled_frustum = s_ptr[1];
		last_stats.culled_occlusion = s_ptr[2];
	}
}

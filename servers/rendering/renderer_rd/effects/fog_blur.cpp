/**************************************************************************/
/*  fog_blur.cpp                                                          */
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
/* without limitation the rights to use, copy, modify, merge, publish,     */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF      */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "servers/rendering/renderer_rd/effects/fog_blur.h"

#include "servers/rendering/renderer_rd/uniform_set_cache_rd.h"

RendererRD::FogBlur::FogBlur() {
	RD::SamplerState linear_sampler_state;
	linear_sampler_state.mag_filter = RD::SAMPLER_FILTER_LINEAR;
	linear_sampler_state.min_filter = RD::SAMPLER_FILTER_LINEAR;
	linear_sampler_state.mip_filter = RD::SAMPLER_FILTER_LINEAR;
	linear_sampler_state.repeat_u = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	linear_sampler_state.repeat_v = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	linear_sampler_state.repeat_w = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	fog_blur.linear_sampler = RD::get_singleton()->sampler_create(linear_sampler_state);

	RD::SamplerState nearest_sampler_state;
	nearest_sampler_state.mag_filter = RD::SAMPLER_FILTER_NEAREST;
	nearest_sampler_state.min_filter = RD::SAMPLER_FILTER_NEAREST;
	nearest_sampler_state.mip_filter = RD::SAMPLER_FILTER_NEAREST;
	nearest_sampler_state.repeat_u = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	nearest_sampler_state.repeat_v = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	nearest_sampler_state.repeat_w = RD::SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
	fog_blur.nearest_sampler = RD::get_singleton()->sampler_create(nearest_sampler_state);

	fog_blur.shader.initialize({ "\n" });
	fog_blur.shader_version = fog_blur.shader.version_create();
	fog_blur.pipeline.create_compute_pipeline(fog_blur.shader.version_get_shader(fog_blur.shader_version, 0));
}

RendererRD::FogBlur::~FogBlur() {
	fog_blur.pipeline.free();
	fog_blur.shader.version_free(fog_blur.shader_version);
	RD::get_singleton()->free_rid(fog_blur.linear_sampler);
	RD::get_singleton()->free_rid(fog_blur.nearest_sampler);
}

void RendererRD::FogBlur::fog_blur_compute(Ref<RenderSceneBuffersRD> p_render_buffers, RenderSceneDataRD *p_scene_data, StringName p_source_texture, float p_intensity, float p_max_blur_lod, bool p_depth_fog, bool p_volumetric_fog_enabled, RID p_volumetric_fog_texture, float p_volumetric_fog_inv_length, float p_volumetric_fog_detail_spread, CopyEffects *p_copy_effects) {
	ERR_FAIL_COND(p_render_buffers.is_null());
	ERR_FAIL_NULL(p_scene_data);
	ERR_FAIL_NULL(p_copy_effects);
	ERR_FAIL_COND(p_intensity <= 0.0f);
	ERR_FAIL_COND(p_max_blur_lod <= 0.0f);

	RD *rd = RD::get_singleton();
	const RD::DataFormat output_format = RD::DATA_FORMAT_R16G16B16A16_SFLOAT;
	const uint32_t output_usage = RD::TEXTURE_USAGE_SAMPLING_BIT | RD::TEXTURE_USAGE_STORAGE_BIT;
	if (!rd->texture_is_format_supported_for_usage(output_format, output_usage)) {
		WARN_PRINT_ONCE("Fog blur requires storage support for R16G16B16A16_SFLOAT textures.");
		return;
	}

	Size2i size = p_render_buffers->get_internal_size();
	if (!p_render_buffers->has_texture(RB_SCOPE_FOG_BLUR, RB_TEX_FOG_BLUR_OUTPUT)) {
		p_render_buffers->create_texture(RB_SCOPE_FOG_BLUR, RB_TEX_FOG_BLUR_OUTPUT, output_format, output_usage, RD::TEXTURE_SAMPLES_1, size);
	}

	const RD::TextureFormat source_format = p_render_buffers->get_texture_format(RB_SCOPE_BUFFERS, p_source_texture);
	ERR_FAIL_COND(source_format.mipmaps == 0);
	const int32_t max_lod = int32_t(source_format.mipmaps) - 1;
	const RID scene_data_uniform = p_scene_data->get_uniform_buffer();

	rd->draw_command_begin_label("Fog Blur");
	for (uint32_t view = 0; view < p_render_buffers->get_view_count(); view++) {
		RID source_texture = p_render_buffers->get_texture_slice(RB_SCOPE_BUFFERS, p_source_texture, view, 0, 1, source_format.mipmaps);
		RID depth_texture = p_render_buffers->get_depth_texture(view);
		RID output_texture = p_render_buffers->get_texture_slice(RB_SCOPE_FOG_BLUR, RB_TEX_FOG_BLUR_OUTPUT, view, 0);
		RID base_texture = p_render_buffers->get_internal_texture(view);

		RD::Uniform source_uniform(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 0, Vector<RID>{ fog_blur.linear_sampler, source_texture });
		RD::Uniform depth_uniform(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 1, Vector<RID>{ fog_blur.nearest_sampler, depth_texture });
		RD::Uniform scene_data_uniform_set(RD::UNIFORM_TYPE_UNIFORM_BUFFER, 2, scene_data_uniform);
		RD::Uniform output_uniform(RD::UNIFORM_TYPE_IMAGE, 3, output_texture);
		RD::Uniform volumetric_fog_uniform(RD::UNIFORM_TYPE_SAMPLER_WITH_TEXTURE, 4, Vector<RID>{ fog_blur.linear_sampler, p_volumetric_fog_texture });

		RID shader = fog_blur.shader.version_get_shader(fog_blur.shader_version, 0);
		RD::ComputeListID compute_list = rd->compute_list_begin();
		rd->compute_list_bind_compute_pipeline(compute_list, fog_blur.pipeline.get_rid());
		rd->compute_list_bind_uniform_set(compute_list, UniformSetCacheRD::get_singleton()->get_cache(shader, 0, source_uniform, depth_uniform, scene_data_uniform_set, output_uniform, volumetric_fog_uniform), 0);

		FogBlurPushConstant push_constant;
		push_constant.max_lod = max_lod;
		push_constant.view_index = view;
		push_constant.intensity = p_intensity;
		push_constant.max_blur_lod = p_max_blur_lod;
		push_constant.depth_fog = p_depth_fog ? 1 : 0;
		push_constant.volumetric_enabled = p_volumetric_fog_enabled ? 1 : 0;
		push_constant.volumetric_fog_inv_length = p_volumetric_fog_inv_length;
		push_constant.volumetric_fog_detail_spread = p_volumetric_fog_detail_spread;
		rd->compute_list_set_push_constant(compute_list, &push_constant, sizeof(FogBlurPushConstant));
		rd->compute_list_dispatch_threads(compute_list, size.x, size.y, 1);
		rd->compute_list_end();

		p_copy_effects->copy_to_rect(output_texture, base_texture, Rect2i(Point2i(), size));
	}
	rd->draw_command_end_label();
}

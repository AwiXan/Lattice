/**************************************************************************/
/*  fog_blur.h                                                            */
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
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#pragma once

#include "servers/rendering/renderer_rd/effects/copy_effects.h"
#include "servers/rendering/renderer_rd/pipeline_deferred_rd.h"
#include "servers/rendering/renderer_rd/shaders/effects/fog_blur.glsl.gen.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_buffers_rd.h"
#include "servers/rendering/renderer_rd/storage_rd/render_scene_data_rd.h"

#define RB_SCOPE_FOG_BLUR SNAME("fog_blur")
#define RB_TEX_FOG_BLUR_OUTPUT SNAME("blur_output")

namespace RendererRD {

class FogBlur {
	struct FogBlurPushConstant {
		int32_t max_lod;
		int32_t view_index;
		float intensity;
		float max_blur_lod;
		int32_t depth_fog;
		int32_t volumetric_enabled;
		float volumetric_fog_inv_length;
		float volumetric_fog_detail_spread;
	};

	struct {
		FogBlurShaderRD shader;
		RID shader_version;
		PipelineDeferredRD pipeline;
		RID linear_sampler;
		RID nearest_sampler;
	} fog_blur;

public:
	FogBlur();
	~FogBlur();

	void fog_blur_compute(Ref<RenderSceneBuffersRD> p_render_buffers, RenderSceneDataRD *p_scene_data, StringName p_source_texture, float p_intensity, float p_max_blur_lod, bool p_depth_fog, bool p_volumetric_fog_enabled, RID p_volumetric_fog_texture, float p_volumetric_fog_inv_length, float p_volumetric_fog_detail_spread, CopyEffects *p_copy_effects);
};

} //namespace RendererRD

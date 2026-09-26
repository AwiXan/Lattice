/**************************************************************************/
/*  rendering_shader_stats.h                                              */
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

#include "core/templates/safe_refcount.h"

// What the renderer has compiled and is compiling, by kind of shader, for
// RenderingServer.get_shader_compilation_info(): a loading screen can wait
// for it, a game can show it. Shader programs (a material's variants, built
// from source or loaded from the shader cache) and the pipelines made of them
// for the ways they are drawn - both happen on worker threads.
struct RenderingShaderStats {
	enum Kind {
		KIND_SPATIAL,
		KIND_CANVAS_ITEM,
		KIND_PARTICLES,
		KIND_SKY,
		KIND_FOG,
		// The renderer's own effects.
		KIND_ENGINE,
		KIND_MAX,
	};

	struct Counters {
		SafeNumeric<uint32_t> shaders_queued;
		SafeNumeric<uint32_t> shaders_done;
		SafeNumeric<uint32_t> shaders_from_cache;
		SafeNumeric<uint32_t> pipelines_queued;
		SafeNumeric<uint32_t> pipelines_done;
	};

	static Counters counters[KIND_MAX];

	static const char *get_kind_name(int p_kind) {
		static const char *names[KIND_MAX] = { "spatial", "canvas_item", "particles", "sky", "fog", "engine" };
		return (p_kind >= 0 && p_kind < KIND_MAX) ? names[p_kind] : "engine";
	}
	static Counters &of(int p_kind) { return counters[(p_kind >= 0 && p_kind < KIND_MAX) ? p_kind : KIND_ENGINE]; }
};

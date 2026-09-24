/**************************************************************************/
/*  node_3d_editor_chrome.cpp                                             */
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

#include "node_3d_editor_chrome.h"

#include "core/io/image.h"
#include "scene/resources/image_texture.h"

Ref<Texture2D> Node3DEditorChrome::make_shading_icon(Shading p_shading, int p_size, const Color &p_ink, const Color &p_accent) {
	const int size = MAX(p_size, 8);
	Ref<Image> image = Image::create_empty(size, size, false, Image::FORMAT_RGBA8);

	const real_t center = size * 0.5;
	const real_t radius = size * 0.5 - 1.0;
	// Lit from the upper left and a little in front, as icons usually are.
	const Vector3 light = Vector3(-0.5, -0.6, 0.62).normalized();
	const Vector3 half = (light + Vector3(0, 0, 1)).normalized();

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			const real_t dx = x + 0.5 - center;
			const real_t dy = y + 0.5 - center;
			const real_t distance = Math::sqrt(dx * dx + dy * dy);
			// How much of this pixel the disc covers, for smooth edges.
			const real_t disc = CLAMP(radius + 0.5 - distance, 0.0, 1.0);
			if (disc <= 0.0) {
				image->set_pixel(x, y, Color(0, 0, 0, 0));
				continue;
			}

			Color color;
			real_t alpha = disc;
			switch (p_shading) {
				case SHADING_WIREFRAME: {
					// The outline, a meridian and the equator.
					const real_t outline = CLAMP(1.4 - Math::abs(distance - (radius - 0.7)), 0.0, 1.0);
					const real_t ellipse_x = dx / (radius * 0.42);
					const real_t ellipse_y = dy / radius;
					const real_t meridian = CLAMP(1.0 - Math::abs(Math::sqrt(ellipse_x * ellipse_x + ellipse_y * ellipse_y) - 1.0) * radius * 0.42 * 1.4, 0.0, 1.0);
					const real_t equator = CLAMP(1.0 - Math::abs(dy) * 1.4, 0.0, 1.0);
					color = p_ink;
					alpha = disc * MAX(outline, MAX(meridian, equator) * 0.8);
				} break;
				case SHADING_UNSHADED: {
					color = p_ink;
					alpha = disc * 0.85;
				} break;
				case SHADING_LIGHTING:
				case SHADING_NORMAL: {
					const real_t nx = dx / radius;
					const real_t ny = dy / radius;
					const Vector3 normal(nx, ny, Math::sqrt(MAX(0.0, 1.0 - nx * nx - ny * ny)));
					const real_t diffuse = MAX(0.0, normal.dot(light));
					if (p_shading == SHADING_LIGHTING) {
						color = p_ink * (0.22 + 0.78 * diffuse);
					} else {
						const real_t facing = MAX((real_t)0.0, normal.dot(half));
						const real_t specular = Math::pow(facing, (real_t)24.0);
						color = p_accent * (0.3 + 0.7 * diffuse);
						color = color.lerp(Color(1, 1, 1), specular * 0.7);
					}
				} break;
				default: {
				} break;
			}
			color.a = alpha;
			image->set_pixel(x, y, color);
		}
	}
	return ImageTexture::create_from_image(image);
}

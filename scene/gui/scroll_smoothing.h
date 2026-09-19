/**************************************************************************/
/*  scroll_smoothing.h                                                    */
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

#include "core/math/math_funcs.h"
#include "scene/gui/scroll_bar.h"

// The wheel moves a target; the bars catch up with it.
//
// Three widgets in the engine scroll themselves with their own scroll bars -
// ScrollContainer, Tree, ItemList - and all three would otherwise carry a copy
// of this. Whether to smooth at all, and how fast, stays with each of them,
// because each reads it from its own theme.
struct ScrollSmoothing {
	// Where the wheel has asked the view to be.
	Vector2 target;
	// Where the bars were left standing after the last step, so that a move made
	// by anything else can be told apart from one of ours.
	Vector2 applied;
	bool active = false;

	bool is_active() const { return active; }
	void stop() { active = false; }

	// Adds a wheel step. Starts from where the bars actually are, so a target
	// left over from before cannot make the view jump.
	void wheel(ScrollBar *p_bar, double p_amount, ScrollBar *p_h, ScrollBar *p_v) {
		if (!active) {
			target = Vector2(p_h ? p_h->get_value() : 0.0, p_v ? p_v->get_value() : 0.0);
			applied = target;
			active = true;
		}
		const bool vertical = p_bar == p_v;
		double value = (vertical ? target.y : target.x) + p_amount;
		value = CLAMP(value, p_bar->get_min(), p_bar->get_max() - p_bar->get_page());
		if (vertical) {
			target.y = value;
		} else {
			target.x = value;
		}
	}

	// Moves the bars a step toward the target. Returns false once it has
	// arrived, so the caller knows it can stop asking for frames.
	//
	// The same fraction of what is left is covered every second however many
	// frames that took, so it feels the same at any framerate.
	bool step(double p_delta, double p_speed, ScrollBar *p_h, ScrollBar *p_v) {
		if (!active) {
			return false;
		}
		const Vector2 current(p_h ? p_h->get_value() : 0.0, p_v ? p_v->get_value() : 0.0);

		// Somebody else has moved the view since the last step - the newer
		// position wins, and the wheel's target is forgotten.
		if (Math::abs(current.x - applied.x) > 0.5 || Math::abs(current.y - applied.y) > 0.5) {
			active = false;
			return false;
		}

		const double t = CLAMP(1.0 - Math::exp(-MAX(1.0, p_speed) * p_delta), 0.0, 1.0);
		Vector2 next = current.lerp(target, t);

		// Close enough that another frame would not show: sit down exactly.
		if (Math::abs(target.x - next.x) < 0.5 && Math::abs(target.y - next.y) < 0.5) {
			next = target;
			active = false;
		}

		if (p_h && next.x != current.x) {
			p_h->set_value(next.x);
		}
		if (p_v && next.y != current.y) {
			p_v->set_value(next.y);
		}
		// Read back rather than assume: the bars clamp, and a target past the end
		// would otherwise look like somebody else's doing on the next step.
		applied = Vector2(p_h ? p_h->get_value() : 0.0, p_v ? p_v->get_value() : 0.0);
		return active;
	}
};

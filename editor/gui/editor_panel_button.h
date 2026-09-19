/**************************************************************************/
/*  editor_panel_button.h                                                 */
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

#include "scene/gui/button.h"

// A button that stands for a kind of panel.
//
// Pressing it asks for one where the button is; dragging it carries the same
// offer to wherever it is let go, so a view can be put where it is wanted in
// one movement instead of being made and then moved. Both are the same thing
// said twice, which is why the button does not decide what happens: it emits
// what it stands for and lets a pane answer.
class EditorPanelButton : public Button {
	GDCLASS(EditorPanelButton, Button);

	StringName panel_type;

protected:
	static void _bind_methods();

public:
	void set_panel_type(const StringName &p_type) { panel_type = p_type; }
	StringName get_panel_type() const { return panel_type; }

	// The offer a pane reads. Nothing is said about what the panel should show:
	// the pane that catches it decides, which is what makes dragging one of
	// these onto a scene's pane give a view of that scene.
	virtual Variant get_drag_data(const Point2 &p_point) override;

	EditorPanelButton();
};

/**************************************************************************/
/*  canvas_item_editor_chrome.h                                           */
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

#include "scene/gui/box_container.h"

class Button;
class CanvasItem;
class EditorSpinSlider;
class GridContainer;
class Label;

// The 2D view's sidebar Item page: the selected Node2Ds' and Controls'
// position, rotation and scale - and a Control's size - as numbers to type
// in, next to where they are being placed. As in the 3D view's (see
// Node3DEditorItemPanel), a value typed in goes to every selected node, on
// that axis alone.
class CanvasItemEditorItemPanel : public VBoxContainer {
	GDCLASS(CanvasItemEditorItemPanel, VBoxContainer);

public:
	enum Row {
		ROW_POSITION,
		ROW_ROTATION,
		ROW_SCALE,
		ROW_SIZE,
		ROW_MAX
	};

private:
	Label *title = nullptr;
	Label *type = nullptr;
	Label *nothing = nullptr;
	VBoxContainer *rows_box = nullptr;
	Control *row_boxes[ROW_MAX] = {};
	EditorSpinSlider *fields[ROW_MAX][2] = {};
	double since_refresh = 0.0;

	// A Control's anchors, as the presets the Layout menu of the 2D view has.
	Control *anchors_box = nullptr;
	GridContainer *anchors = nullptr;
	Label *anchors_note = nullptr;
	Button *anchor_buttons[16] = {};
	void _anchors_pressed(int p_preset);

	Vector<CanvasItem *> _edited_items() const;
	void _field_changed(double p_value, int p_row, int p_axis);
	static bool _has_row(const CanvasItem *p_item, int p_row);
	static Vector2 _read(const CanvasItem *p_item, int p_row);
	static const char *_setter(int p_row);

protected:
	void _notification(int p_what);

public:
	void refresh();
	EditorSpinSlider *get_field(Row p_row, int p_axis) const;
	// The preset buttons, in the order of the grid, for checking them.
	Button *get_anchor_button(int p_index) const { return (p_index >= 0 && p_index < 16) ? anchor_buttons[p_index] : nullptr; }
	static int get_anchor_preset(int p_index);

	CanvasItemEditorItemPanel();
};

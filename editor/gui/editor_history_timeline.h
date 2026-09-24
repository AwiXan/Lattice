/**************************************************************************/
/*  editor_history_timeline.h                                             */
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

#include "core/io/image.h"
#include "core/templates/hash_map.h"
#include "scene/gui/box_container.h"

class Button;
class HBoxContainer;
class Label;
class PanelContainer;
class ScrollContainer;
class TextureRect;

// A picture of the editor's work area after each thing done, for the
// history timeline to show: taken when the scene's history is at its newest
// - what was just done - a couple of frames on, once it is drawn. Kept by the
// action's timestamp, which undoing and redoing leave alone. One for the
// editor, owned by EditorNode.
class EditorHistoryThumbnails : public Node {
	GDCLASS(EditorHistoryThumbnails, Node);

	static inline EditorHistoryThumbnails *singleton = nullptr;
	static constexpr int KEPT = 48;
	static constexpr int WIDTH = 720;

	HashMap<double, Ref<Image>> images;
	// Of which view each was taken (see get_work_area()).
	HashMap<double, int> kinds;
	LocalVector<double> taken_order;
	int frames_to_wait = 0;

	void _version_changed();
	void _frame_drawn();
	void _take();

protected:
	static void _bind_methods();

public:
	static EditorHistoryThumbnails *get_singleton() { return singleton; }
	Ref<Image> get_image(double p_timestamp) const;
	int get_kind(double p_timestamp) const;
	// Where the work is: the 3D view (1), the 2D view (2) - the one in use,
	// if it shows - or failing both, the whole of the main screen (0). What
	// is pictured, and what a picture is shown over.
	static Control *get_work_area(int p_kind);
	static int get_work_kind();
	int get_count() const { return images.size(); }

	EditorHistoryThumbnails();
	~EditorHistoryThumbnails();
};

// The history of the scene being worked in, as a strip of cards: what was
// done, a picture of how it looked after, and how long ago; the step it is
// at marked, what was undone faded. The mouse over a card shows its picture
// over the work area, as it was then; a click goes back - or forward - to it,
// undoing or redoing each step between. A panel ("history_timeline").
class EditorHistoryTimeline : public VBoxContainer {
	GDCLASS(EditorHistoryTimeline, VBoxContainer);

	struct Step {
		double timestamp = 0; // 0: the beginning.
		String name;
		bool done = true;
	};
	Vector<Step> steps;
	int at = 0; // Index of the step the scene is at.

	ScrollContainer *scroll = nullptr;
	HBoxContainer *cards = nullptr;
	Label *empty = nullptr;
	// The picture of the step under the mouse, over the work area.
	TextureRect *preview = nullptr;
	Label *preview_caption = nullptr;
	int previewed = -1;
	bool refresh_queued = false;

	void _queue_refresh();
	void _refresh();
	Control *_make_card(int p_index);
	void _card_input(const Ref<InputEvent> &p_event, int p_index);
	void _card_hovered(int p_index);
	void _card_left(int p_index);
	void _hide_preview();
	void _update_times();

protected:
	void _notification(int p_what);

public:
	// Undoes or redoes the scene's history until p_index is the step it is at.
	void seek(int p_index);
	// Shows the picture of a step over the work area, as the mouse over its
	// card does; -1 hides it.
	void preview_step(int p_index) {
		if (p_index < 0) {
			_hide_preview();
		} else {
			_card_hovered(p_index);
		}
	}
	int get_step_count() const { return steps.size(); }
	int get_current_step() const { return at; }
	String get_step_name(int p_index) const { return p_index >= 0 && p_index < steps.size() ? steps[p_index].name : String(); }

	static Control *create_panel();

	EditorHistoryTimeline();
	~EditorHistoryTimeline();
};

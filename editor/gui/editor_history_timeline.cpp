/**************************************************************************/
/*  editor_history_timeline.cpp                                           */
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

#include "editor_history_timeline.h"

#include "core/input/input_event.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "editor/editor_data.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_string_names.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/gui/editor_view_header_group.h"
#include "editor/scene/3d/node_3d_editor_plugin.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/themes/editor_scale.h"
#include "scene/animation/tween.h"
#include "scene/gui/label.h"
#include "scene/gui/panel_container.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/texture_rect.h"
#include "scene/main/timer.h"
#include "scene/main/window.h"
#include "scene/resources/image_texture.h"
#include "scene/resources/style_box_flat.h"
#include "servers/rendering/rendering_server.h"

//////////////////////////////////////////////////////////////////////////////
// The pictures.

void EditorHistoryThumbnails::_bind_methods() {
	ADD_SIGNAL(MethodInfo("taken"));
}

Ref<Image> EditorHistoryThumbnails::get_image(double p_timestamp) const {
	const Ref<Image> *image = images.getptr(p_timestamp);
	return image ? *image : Ref<Image>();
}

int EditorHistoryThumbnails::get_kind(double p_timestamp) const {
	const int *kind = kinds.getptr(p_timestamp);
	return kind ? *kind : 0;
}

int EditorHistoryThumbnails::get_work_kind() {
	if (Node3DEditor::get_singleton() && Node3DEditor::get_singleton()->is_visible_in_tree()) {
		return 1;
	}
	if (CanvasItemEditor::get_singleton() && CanvasItemEditor::get_singleton()->is_visible_in_tree()) {
		return 2;
	}
	return 0;
}

Control *EditorHistoryThumbnails::get_work_area(int p_kind) {
	if (p_kind == 1 && Node3DEditor::get_singleton() && Node3DEditor::get_singleton()->is_visible_in_tree()) {
		return Node3DEditor::get_singleton();
	}
	if (p_kind == 2 && CanvasItemEditor::get_singleton() && CanvasItemEditor::get_singleton()->is_visible_in_tree()) {
		return CanvasItemEditor::get_singleton();
	}
	return EditorNode::get_singleton() ? EditorNode::get_singleton()->get_editor_main_screen() : nullptr;
}

void EditorHistoryThumbnails::_version_changed() {
	// Once what was done has been drawn: the next frame, and one more for luck.
	frames_to_wait = 2;
	if (!RenderingServer::get_singleton()->is_connected(SNAME("frame_post_draw"), callable_mp(this, &EditorHistoryThumbnails::_frame_drawn))) {
		RenderingServer::get_singleton()->connect(SNAME("frame_post_draw"), callable_mp(this, &EditorHistoryThumbnails::_frame_drawn));
	}
}

void EditorHistoryThumbnails::_frame_drawn() {
	if (--frames_to_wait > 0) {
		return;
	}
	RenderingServer::get_singleton()->disconnect(SNAME("frame_post_draw"), callable_mp(this, &EditorHistoryThumbnails::_frame_drawn));
	_take();
}

void EditorHistoryThumbnails::_take() {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo || !EditorNode::get_singleton()) {
		return;
	}
	const EditorUndoRedoManager::History &history = undo_redo->get_or_create_history(EditorNode::get_editor_data().get_current_edited_scene_history_id());
	// Only at the newest step: anywhere else, the picture is of a step that
	// has one already.
	if (history.undo_stack.is_empty() || !history.redo_stack.is_empty()) {
		return;
	}
	const double stamp = history.undo_stack.back()->get().timestamp;

	const int kind = get_work_kind();
	Control *area = get_work_area(kind);
	Window *window = area ? area->get_window() : nullptr;
	if (!window || !area->is_visible_in_tree() || window->get_mode() == Window::MODE_MINIMIZED) {
		return;
	}
	Ref<Image> full = window->get_texture()->get_image();
	if (full.is_null() || full->is_empty()) {
		return;
	}
	const Rect2i region = Rect2i(area->get_global_rect()).intersection(Rect2i(Point2i(), full->get_size()));
	if (!region.has_area()) {
		return;
	}
	Ref<Image> picture = full->get_region(region);
	picture->resize(WIDTH, MAX(1, WIDTH * region.size.y / region.size.x), Image::INTERPOLATE_BILINEAR);
	picture->convert(Image::FORMAT_RGB8);
	if (!images.has(stamp)) {
		taken_order.push_back(stamp);
	}
	images[stamp] = picture;
	kinds[stamp] = kind;
	while (taken_order.size() > KEPT) {
		images.erase(taken_order[0]);
		kinds.erase(taken_order[0]);
		taken_order.remove_at(0);
	}
	emit_signal(SNAME("taken"));
}

EditorHistoryThumbnails::EditorHistoryThumbnails() {
	singleton = this;
	// Something done is "history_changed"; undone or redone, "version_changed".
	EditorUndoRedoManager::get_singleton()->connect(SNAME("history_changed"), callable_mp(this, &EditorHistoryThumbnails::_version_changed));
	EditorUndoRedoManager::get_singleton()->connect(SNAME("version_changed"), callable_mp(this, &EditorHistoryThumbnails::_version_changed));
}

EditorHistoryThumbnails::~EditorHistoryThumbnails() {
	if (RenderingServer::get_singleton() && RenderingServer::get_singleton()->is_connected(SNAME("frame_post_draw"), callable_mp(this, &EditorHistoryThumbnails::_frame_drawn))) {
		RenderingServer::get_singleton()->disconnect(SNAME("frame_post_draw"), callable_mp(this, &EditorHistoryThumbnails::_frame_drawn));
	}
	if (singleton == this) {
		singleton = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////
// The timeline.

static String _time_ago(double p_timestamp) {
	const int seconds = MAX(0, int(OS::get_singleton()->get_unix_time() - p_timestamp));
	if (seconds < 60) {
		return TTR("just now");
	}
	if (seconds < 3600) {
		return vformat(TTR("%d min ago"), seconds / 60);
	}
	return vformat(TTR("%d h ago"), seconds / 3600);
}

struct StepsByTime {
	_FORCE_INLINE_ bool operator()(const EditorUndoRedoManager::Action &p_a, const EditorUndoRedoManager::Action &p_b) const {
		return p_a.timestamp < p_b.timestamp;
	}
};

Control *EditorHistoryTimeline::create_panel() {
	return memnew(EditorHistoryTimeline);
}

void EditorHistoryTimeline::_queue_refresh() {
	if (!refresh_queued) {
		refresh_queued = true;
		callable_mp(this, &EditorHistoryTimeline::_refresh).call_deferred();
	}
}

void EditorHistoryTimeline::_refresh() {
	refresh_queued = false;
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo) {
		return;
	}
	const EditorUndoRedoManager::History &history = undo_redo->get_or_create_history(EditorNode::get_editor_data().get_current_edited_scene_history_id());
	Vector<EditorUndoRedoManager::Action> done;
	for (const EditorUndoRedoManager::Action &action : history.undo_stack) {
		done.push_back(action);
	}
	Vector<EditorUndoRedoManager::Action> undone;
	for (const EditorUndoRedoManager::Action &action : history.redo_stack) {
		undone.push_back(action);
	}
	done.sort_custom<StepsByTime>();
	undone.sort_custom<StepsByTime>();

	steps.clear();
	steps.push_back(Step{ 0, TTR("The Beginning"), true });
	for (const EditorUndoRedoManager::Action &action : done) {
		steps.push_back(Step{ action.timestamp, action.action_name, true });
	}
	at = steps.size() - 1;
	for (const EditorUndoRedoManager::Action &action : undone) {
		steps.push_back(Step{ action.timestamp, action.action_name, false });
	}

	_hide_preview();
	while (cards->get_child_count() > 0) {
		Node *card = cards->get_child(0);
		cards->remove_child(card);
		// Later: it can be the card whose click is being answered.
		card->queue_free();
	}
	for (int i = 0; i < steps.size(); i++) {
		cards->add_child(_make_card(i));
	}
	empty->set_visible(steps.size() <= 1);
	// The step it is at in sight.
	Control *current = Object::cast_to<Control>(cards->get_child(at));
	if (current) {
		callable_mp(scroll, &ScrollContainer::ensure_control_visible).call_deferred(current);
	}
}

Control *EditorHistoryTimeline::_make_card(int p_index) {
	const Step &step = steps[p_index];
	PanelContainer *card = memnew(PanelContainer);
	card->set_mouse_filter(MOUSE_FILTER_STOP);
	card->set_default_cursor_shape(CURSOR_POINTING_HAND);
	card->set_tooltip_text(step.name);
	Ref<StyleBoxFlat> style = EditorViewHeaderGroup::make_style(this);
	style->set_content_margin_all(4 * EDSCALE);
	if (p_index == at) {
		style->set_border_width_all(MAX(2, (int)Math::round(2 * EDSCALE)));
		style->set_border_color(get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
	}
	card->add_theme_style_override(SceneStringName(panel), style);
	if (!step.done) {
		// Undone: there to go forward to, and faded.
		card->set_modulate(Color(1, 1, 1, 0.45));
	}

	VBoxContainer *column = memnew(VBoxContainer);
	column->add_theme_constant_override("separation", 2 * EDSCALE);
	column->set_mouse_filter(MOUSE_FILTER_IGNORE);
	card->add_child(column);

	TextureRect *picture = memnew(TextureRect);
	picture->set_mouse_filter(MOUSE_FILTER_IGNORE);
	picture->set_custom_minimum_size(Size2(150, 84) * EDSCALE);
	picture->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
	picture->set_stretch_mode(TextureRect::STRETCH_KEEP_ASPECT_COVERED);
	const Ref<Image> image = EditorHistoryThumbnails::get_singleton() ? EditorHistoryThumbnails::get_singleton()->get_image(step.timestamp) : Ref<Image>();
	if (image.is_valid()) {
		picture->set_texture(ImageTexture::create_from_image(image));
	} else {
		// No picture: the beginning, or a step from before the editor started.
		picture->set_texture(get_editor_theme_icon(p_index == 0 ? SNAME("Play") : SNAME("History")));
		picture->set_stretch_mode(TextureRect::STRETCH_KEEP_CENTERED);
		picture->set_self_modulate(Color(1, 1, 1, 0.4));
	}
	column->add_child(picture);

	Label *name = memnew(Label);
	name->set_text(step.name);
	name->set_mouse_filter(MOUSE_FILTER_IGNORE);
	name->set_clip_text(true);
	name->set_custom_minimum_size(Size2(150 * EDSCALE, 0));
	name->set_auto_translate_mode(AUTO_TRANSLATE_MODE_DISABLED);
	column->add_child(name);

	Label *when = memnew(Label);
	when->set_text(p_index == 0 ? String() : _time_ago(step.timestamp));
	when->set_mouse_filter(MOUSE_FILTER_IGNORE);
	when->set_modulate(Color(1, 1, 1, 0.55));
	when->add_theme_font_size_override(SceneStringName(font_size), get_theme_font_size(SNAME("font_size"), SNAME("Label")) * 0.85);
	column->add_child(when);

	card->connect(SceneStringName(gui_input), callable_mp(this, &EditorHistoryTimeline::_card_input).bind(p_index));
	card->connect(SceneStringName(mouse_entered), callable_mp(this, &EditorHistoryTimeline::_card_hovered).bind(p_index));
	card->connect(SceneStringName(mouse_exited), callable_mp(this, &EditorHistoryTimeline::_card_left).bind(p_index));
	return card;
}

void EditorHistoryTimeline::_card_input(const Ref<InputEvent> &p_event, int p_index) {
	Ref<InputEventMouseButton> click = p_event;
	if (click.is_valid() && click->is_pressed() && click->get_button_index() == MouseButton::LEFT) {
		seek(p_index);
		accept_event();
	}
}

void EditorHistoryTimeline::seek(int p_index) {
	EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
	if (!undo_redo || p_index < 0 || p_index >= steps.size()) {
		return;
	}
	const int history = EditorNode::get_editor_data().get_current_edited_scene_history_id();
	_hide_preview();
	while (at > p_index && undo_redo->undo_history(history)) {
		at--;
	}
	while (at < p_index && undo_redo->redo_history(history)) {
		at++;
	}
	_queue_refresh();
}

void EditorHistoryTimeline::_card_hovered(int p_index) {
	if (p_index < 0 || p_index >= steps.size() || p_index == at) {
		// The step it is at is what is on screen already.
		_hide_preview();
		return;
	}
	EditorHistoryThumbnails *thumbnails = EditorHistoryThumbnails::get_singleton();
	const Ref<Image> image = thumbnails ? thumbnails->get_image(steps[p_index].timestamp) : Ref<Image>();
	// Over the view it was taken of, where it is now - not over this strip.
	Control *area = thumbnails ? EditorHistoryThumbnails::get_work_area(thumbnails->get_kind(steps[p_index].timestamp)) : nullptr;
	if (image.is_null() || !area) {
		_hide_preview();
		return;
	}
	if (!preview) {
		preview = memnew(TextureRect);
		preview->set_as_top_level(true);
		preview->set_mouse_filter(MOUSE_FILTER_IGNORE);
		preview->set_expand_mode(TextureRect::EXPAND_IGNORE_SIZE);
		preview->set_stretch_mode(TextureRect::STRETCH_SCALE);
		preview->set_texture_filter(TEXTURE_FILTER_LINEAR);
		// Framed in the accent colour: a picture of then, not now.
		PanelContainer *frame = memnew(PanelContainer);
		frame->set_mouse_filter(MOUSE_FILTER_IGNORE);
		frame->set_anchors_and_offsets_preset(PRESET_FULL_RECT);
		Ref<StyleBoxFlat> border;
		border.instantiate();
		border->set_draw_center(false);
		border->set_border_width_all(MAX(2, (int)Math::round(3 * EDSCALE)));
		border->set_border_color(get_theme_color(SNAME("accent_color"), EditorStringName(Editor)));
		frame->add_theme_style_override(SceneStringName(panel), border);
		preview->add_child(frame);
		EditorViewHeaderGroup *caption_frame = memnew(EditorViewHeaderGroup(false, true));
		caption_frame->set_mouse_filter(MOUSE_FILTER_IGNORE);
		preview_caption = memnew(Label);
		caption_frame->take({ preview_caption });
		caption_frame->set_anchors_and_offsets_preset(PRESET_CENTER_BOTTOM, PRESET_MODE_MINSIZE, 16 * EDSCALE);
		caption_frame->set_h_grow_direction(GROW_DIRECTION_BOTH);
		caption_frame->set_v_grow_direction(GROW_DIRECTION_BEGIN);
		preview->add_child(caption_frame);
		EditorNode::get_singleton()->get_editor_main_screen()->add_child(preview);
	}
	preview->set_texture(ImageTexture::create_from_image(image));
	preview->set_global_position(area->get_global_position());
	preview->set_size(area->get_size());
	preview_caption->set_text(vformat(TTR("As it was after \"%s\", %s. Click to go back to it."), steps[p_index].name, _time_ago(steps[p_index].timestamp)));
	if (!preview->is_visible() || previewed < 0) {
		preview->set_modulate(Color(1, 1, 1, 0));
		preview->create_tween()->tween_property(preview, NodePath("modulate"), Color(1, 1, 1, 1), 0.12);
	}
	preview->show();
	previewed = p_index;
}

void EditorHistoryTimeline::_card_left(int p_index) {
	if (previewed == p_index) {
		_hide_preview();
	}
}

void EditorHistoryTimeline::_hide_preview() {
	if (preview) {
		preview->hide();
	}
	previewed = -1;
}

void EditorHistoryTimeline::_update_times() {
	if (is_visible_in_tree()) {
		_queue_refresh();
	}
}

void EditorHistoryTimeline::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_ENTER_TREE: {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->connect(SNAME("version_changed"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			undo_redo->connect(SNAME("history_changed"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			EditorNode::get_singleton()->connect(SNAME("scene_changed"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			if (EditorHistoryThumbnails::get_singleton()) {
				EditorHistoryThumbnails::get_singleton()->connect(SNAME("taken"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			}
			_queue_refresh();
		} break;

		case NOTIFICATION_EXIT_TREE: {
			EditorUndoRedoManager *undo_redo = EditorUndoRedoManager::get_singleton();
			undo_redo->disconnect(SNAME("version_changed"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			undo_redo->disconnect(SNAME("history_changed"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			EditorNode::get_singleton()->disconnect(SNAME("scene_changed"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			if (EditorHistoryThumbnails::get_singleton()) {
				EditorHistoryThumbnails::get_singleton()->disconnect(SNAME("taken"), callable_mp(this, &EditorHistoryTimeline::_queue_refresh));
			}
			if (preview) {
				preview->queue_free();
				preview = nullptr;
				preview_caption = nullptr;
			}
		} break;

		case NOTIFICATION_VISIBILITY_CHANGED: {
			if (is_visible_in_tree()) {
				_queue_refresh();
			} else {
				_hide_preview();
			}
		} break;
	}
}

EditorHistoryTimeline::EditorHistoryTimeline() {
	set_name(TTR("Timeline"));
	add_theme_constant_override("separation", 0);

	scroll = memnew(ScrollContainer);
	scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	add_child(scroll);
	cards = memnew(HBoxContainer);
	cards->add_theme_constant_override("separation", 6 * EDSCALE);
	cards->set_v_size_flags(SIZE_SHRINK_BEGIN);
	scroll->add_child(cards);

	empty = memnew(Label);
	empty->set_text(TTR("Nothing done in this scene yet: every step will be here, with a picture of how it looked."));
	empty->set_modulate(Color(1, 1, 1, 0.6));
	add_child(empty);

	// How long ago, kept roughly true.
	Timer *timer = memnew(Timer);
	timer->set_wait_time(30);
	timer->set_autostart(true);
	timer->connect("timeout", callable_mp(this, &EditorHistoryTimeline::_update_times));
	add_child(timer);
}

EditorHistoryTimeline::~EditorHistoryTimeline() {
	if (preview) {
		// It lives over the work area, not in here.
		preview->queue_free();
	}
}

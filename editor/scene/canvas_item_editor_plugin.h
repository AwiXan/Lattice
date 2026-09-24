/**************************************************************************/
/*  canvas_item_editor_plugin.h                                           */
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

#include "editor/editor_document_view.h"
#include "editor/plugins/editor_plugin.h"
#include "scene/gui/box_container.h"

class AcceptDialog;
class Button;
class ButtonGroup;
class CanvasItemEditorItemPanel;
class CanvasItemEditorViewport;
class ConfirmationDialog;
class EditorData;
class EditorSelection;
class EditorButtonMirror;
class EditorPieMenu;
class EditorViewHints;
class EditorViewPill;
class EditorViewSidebar;
class EditorZoomWidget;
class HFlowContainer;
class HScrollBar;
class HSplitContainer;
class MenuButton;
class PanelContainer;
class RichTextLabel;
class StyleBoxTexture;
class SubViewport;
class SubViewportContainer;
class Timer;
class ViewPanner;
class VScrollBar;
class VSeparator;
class VSplitContainer;

class CanvasItemEditorSelectedItem : public Object {
	GDCLASS(CanvasItemEditorSelectedItem, Object);

public:
	Transform2D prev_xform;
	Rect2 prev_rect;
	Vector2 prev_pivot;
	Vector2 prev_pivot_ratio;
	real_t prev_anchors[4] = { (real_t)0.0 };

	Transform2D pre_drag_xform;
	Rect2 pre_drag_rect;

	List<real_t> pre_drag_bones_length;
	List<Dictionary> pre_drag_bones_undo_state;

	Dictionary undo_state;
};

class CanvasItemEditor : public EditorDocumentView {
	GDCLASS(CanvasItemEditor, EditorDocumentView);

public:
	enum Tool {
		TOOL_SELECT,
		TOOL_SCENE_PAINT,
		TOOL_LIST_SELECT,
		TOOL_MOVE,
		TOOL_SCALE,
		TOOL_ROTATE,
		TOOL_EDIT_PIVOT,
		TOOL_PAN,
		TOOL_RULER,
		TOOL_MAX
	};

	enum AddNodeOption {
		ADD_NODE,
		ADD_INSTANCE,
		ADD_PASTE,
		ADD_MOVE,
	};

private:
	enum SnapTarget {
		SNAP_TARGET_NONE = 0,
		SNAP_TARGET_PARENT,
		SNAP_TARGET_SELF_ANCHORS,
		SNAP_TARGET_SELF,
		SNAP_TARGET_OTHER_NODE,
		SNAP_TARGET_GUIDE,
		SNAP_TARGET_GRID,
		SNAP_TARGET_PIXEL
	};

	enum MenuOption {
		SNAP_USE,
		SNAP_USE_NODE_PARENT,
		SNAP_USE_NODE_ANCHORS,
		SNAP_USE_NODE_SIDES,
		SNAP_USE_NODE_CENTER,
		SNAP_USE_OTHER_NODES,
		SNAP_USE_GRID,
		SNAP_USE_GUIDES,
		SNAP_USE_ROTATION,
		SNAP_USE_SCALE,
		SNAP_RELATIVE,
		SNAP_CONFIGURE,
		SNAP_USE_PIXEL,
		SHOW_HELPERS,
		SHOW_RULERS,
		SHOW_GUIDES,
		SHOW_ORIGIN,
		SHOW_VIEWPORT,
		SHOW_POSITION_GIZMOS,
		SHOW_LOCK_GIZMOS,
		SHOW_GROUP_GIZMOS,
		SHOW_TRANSFORMATION_GIZMOS,
		LOCK_SELECTED,
		UNLOCK_SELECTED,
		GROUP_SELECTED,
		UNGROUP_SELECTED,
		ANIM_INSERT_KEY,
		ANIM_INSERT_KEY_EXISTING,
		ANIM_INSERT_POS,
		ANIM_INSERT_ROT,
		ANIM_INSERT_SCALE,
		ANIM_COPY_POSE,
		ANIM_PASTE_POSE,
		ANIM_CLEAR_POSE,
		CLEAR_GUIDES,
		VIEW_CENTER_TO_SELECTION,
		VIEW_FRAME_TO_SELECTION,
		PREVIEW_CANVAS_SCALE,
		SKELETON_MAKE_BONES,
		SKELETON_SHOW_BONES,
		AUTO_RESAMPLE_CANVAS_ITEMS,
	};

	enum DragType {
		DRAG_NONE,
		DRAG_BOX_SELECTION,
		DRAG_LEFT,
		DRAG_TOP_LEFT,
		DRAG_TOP,
		DRAG_TOP_RIGHT,
		DRAG_RIGHT,
		DRAG_BOTTOM_RIGHT,
		DRAG_BOTTOM,
		DRAG_BOTTOM_LEFT,
		DRAG_ANCHOR_TOP_LEFT,
		DRAG_ANCHOR_TOP_RIGHT,
		DRAG_ANCHOR_BOTTOM_RIGHT,
		DRAG_ANCHOR_BOTTOM_LEFT,
		DRAG_ANCHOR_ALL,
		DRAG_QUEUED,
		DRAG_MOVE,
		DRAG_MOVE_X,
		DRAG_MOVE_Y,
		DRAG_SCALE_X,
		DRAG_SCALE_Y,
		DRAG_SCALE_BOTH,
		DRAG_ROTATE,
		DRAG_PIVOT,
		DRAG_TEMP_PIVOT,
		DRAG_V_GUIDE,
		DRAG_H_GUIDE,
		DRAG_DOUBLE_GUIDE,
		DRAG_KEY_MOVE
	};

	enum GridVisibility {
		GRID_VISIBILITY_SHOW,
		GRID_VISIBILITY_SHOW_WHEN_SNAPPING,
		GRID_VISIBILITY_HIDE,
	};

	enum TransformType {
		POSITION,
		ROTATION,
		SCALE,
	};

	const String locked_transform_warning = TTRC("All selected CanvasItems are either invisible or locked in some way and can't be transformed.");

	bool selection_menu_additive_selection = false;

	Tool tool = TOOL_SELECT;
	Control *viewport = nullptr;
	Control *viewport_scrollable = nullptr;

	HScrollBar *h_scroll = nullptr;
	VScrollBar *v_scroll = nullptr;

	// Used for secondary menu items which are displayed depending on the currently selected node
	// (such as MeshInstance's "Mesh" menu).
	PanelContainer *context_toolbar_panel = nullptr;
	HBoxContainer *context_toolbar_hbox = nullptr;
	HashMap<Control *, VSeparator *> context_toolbar_separators;

	void _update_context_toolbar();
	// What the view puts in its own context toolbar - the animation keys - as
	// opposed to what plugins hand it, which goes to the primary view.
	void _add_control_to_own_menu_panel(Control *p_control);

	// The view's own layout, as the 3D view has it (see
	// Node3DEditor::_arrange_chrome()), unless interface/editor/appearance/
	// classic_viewport_toolbars asks for the one toolbar.
	HFlowContainer *toolbar_flow = nullptr;
	PanelContainer *tool_column_panel = nullptr;
	HBoxContainer *header_end = nullptr;
	MenuButton *overlays_menu = nullptr;
	PopupMenu *overlays_gizmos_menu = nullptr;
	PopupMenu *overlays_grid_menu = nullptr;
	void _overlays_grid_pressed(int p_id);
	// The view's bar, over its top edge, as each 3D viewport has one (see
	// Node3DEditor::_build_view_bar()): the View menu's items at the top left,
	// beside the zoom, and what is drawn over the scene (overlays_menu) at the
	// top right. The View menu is kept, out of sight: it holds the state,
	// addons may know it, and its shortcuts go on working.
	EditorViewPill *view_options_pill = nullptr;
	HBoxContainer *view_bar_end = nullptr;
	void _place_view_bar();
	EditorViewSidebar *sidebar = nullptr;
	Button *sidebar_button = nullptr;
	CanvasItemEditorItemPanel *item_panel = nullptr;
	EditorViewHints *hints = nullptr;
	bool updating_snap_fields = false;
	void _arrange_chrome();
	void _build_sidebar(Control *p_over);
	void _overlays_about_to_popup();
	void _overlays_id_pressed(int p_id);
	void _overlays_gizmo_pressed(int p_id);
	void _sidebar_button_toggled(bool p_pressed);
	void _sidebar_page_shown(int p_page);
	void _sidebar_fitted();
	void _snap_field_changed();
	void _chrome_tick();
	void _update_hints();

	// As in the 3D view: what plugins and addons hand the 2D editor goes into
	// the view its plugin made, which lasts; the others show copies.
	static inline CanvasItemEditor *primary_instance = nullptr;
	EditorButtonMirror *addon_mirror = nullptr;
	HBoxContainer *addon_mirror_box = nullptr;
	static inline bool addon_mirrors_queued = false;
	static void _queue_addon_mirror_rebuild();
	static void _rebuild_all_addon_mirrors();
	void _rebuild_addon_mirrors();
	void _sync_addon_mirrors();
	void _activate_for_user();
	static bool _is_views_own_control(Node *p_node, Control *p_to);

	// The view pie (`): zoom, centring, and what is drawn over the canvas.
	EditorPieMenu *pie = nullptr;
	void _pie_closed();

	Transform2D transform;
	GridVisibility grid_visibility = GRID_VISIBILITY_SHOW_WHEN_SNAPPING;
	bool show_rulers = true;
	bool show_guides = true;
	bool show_origin = true;
	bool show_viewport = true;
	bool show_helpers = false;
	bool show_position_gizmos = true;
	bool show_lock_gizmos = true;
	bool show_group_gizmos = true;
	bool show_transformation_gizmos = true;

	real_t zoom = 1.0;
	Point2 view_offset;
	Point2 previous_update_view_offset;

	Timer *resample_timer = nullptr;
	bool auto_resampling_enabled = true;
	real_t resample_delay = 0.3;

	bool selected_from_canvas = false;
	bool had_visible_selection = false;

	// Defaults are defined in clear().
	Point2 grid_offset;
	Point2 grid_step;
	Vector2i primary_grid_step;
	int grid_step_multiplier = 0;

	Color selection_rectangle_color;
	Color locked_selection_rectangle_color;

	real_t snap_rotation_step = 0.0;
	real_t snap_rotation_offset = 0.0;
	real_t snap_scale_step = 0.0;
	bool use_local_space = true;
	bool smart_snap_active = false;
	bool grid_snap_active = false;

	bool snap_node_parent = true;
	bool snap_node_anchors = true;
	bool snap_node_sides = true;
	bool snap_node_center = true;
	bool snap_other_nodes = true;
	bool snap_guides = true;
	bool snap_rotation = false;
	bool snap_scale = false;
	bool snap_relative = false;
	// Enable pixel snapping even if pixel snap rendering is disabled in the Project Settings.
	// This results in crisper visuals by preventing 2D nodes from being placed at subpixel coordinates.
	bool snap_pixel = true;

	bool key_pos = true;
	bool key_rot = true;
	bool key_scale = false;

	bool pan_pressed = false;
	Vector2 temp_pivot = Vector2(Math::INF, Math::INF);

	bool ruler_tool_active = false;
	Point2 ruler_tool_origin;
	real_t ruler_width_scaled = 16.0;
	int ruler_font_size = 8;
	Point2 node_create_position;
	real_t grab_distance = 0.0;
	bool simple_panning = false;

	MenuOption last_option = SNAP_USE;

public:
	struct SelectResult {
		CanvasItem *item = nullptr;
		real_t z_index = 0;
		bool has_z = true;
		_FORCE_INLINE_ bool operator<(const SelectResult &p_rr) const {
			return has_z && p_rr.has_z ? p_rr.z_index < z_index : p_rr.has_z;
		}
	};

private:
	Vector<SelectResult> selection_results;
	Vector<SelectResult> selection_results_menu;

	struct _HoverResult {
		Point2 position;
		Ref<Texture2D> icon;
		String name;
	};
	Vector<_HoverResult> hovering_results;

	struct BoneList {
		Transform2D xform;
		real_t length = 0;
		uint64_t last_pass = 0;
	};

	uint64_t bone_last_frame = 0;

	struct BoneKey {
		ObjectID from;
		ObjectID to;
		_FORCE_INLINE_ bool operator<(const BoneKey &p_key) const {
			if (from == p_key.from) {
				return to < p_key.to;
			} else {
				return from < p_key.from;
			}
		}
	};

	HashMap<BoneKey, BoneList> bone_list;
	MenuButton *skeleton_menu = nullptr;

	struct PoseClipboard {
		Vector2 pos;
		Vector2 scale;
		real_t rot = 0;
		ObjectID id;
	};
	List<PoseClipboard> pose_clipboard;

	Button *select_button = nullptr;

	Button *move_button = nullptr;
	Button *scene_paint_button = nullptr;
	Button *scale_button = nullptr;
	Button *rotate_button = nullptr;

	Button *list_select_button = nullptr;
	Button *pivot_button = nullptr;
	Button *pan_button = nullptr;

	Button *ruler_button = nullptr;

	Button *local_space_button = nullptr;
	Button *smart_snap_button = nullptr;
	Button *grid_snap_button = nullptr;
	MenuButton *snap_config_menu = nullptr;
	PopupMenu *smartsnap_config_popup = nullptr;

	Button *lock_button = nullptr;
	Button *unlock_button = nullptr;

	Button *group_button = nullptr;
	Button *ungroup_button = nullptr;

	MenuButton *view_menu = nullptr;
	PopupMenu *grid_menu = nullptr;
	PopupMenu *theme_menu = nullptr;
	PopupMenu *gizmos_menu = nullptr;
	HBoxContainer *animation_hb = nullptr;
	MenuButton *animation_menu = nullptr;

	Button *key_loc_button = nullptr;
	Button *key_rot_button = nullptr;
	Button *key_scale_button = nullptr;
	Button *key_insert_button = nullptr;
	Button *key_auto_insert_button = nullptr;

	PopupMenu *selection_menu = nullptr;
	PopupMenu *add_node_menu = nullptr;

	Control *top_ruler = nullptr;
	Control *left_ruler = nullptr;

	Point2 drag_start_origin;
	DragType drag_type = DRAG_NONE;
	Point2 drag_from;
	Point2 drag_to;
	Point2 drag_rotation_center;
	List<CanvasItem *> drag_selection;
	int dragged_guide_index = -1;
	Point2 dragged_guide_pos;
	bool is_hovering_h_guide = false;
	bool is_hovering_v_guide = false;

	bool updating_value_dialog = false;
	Transform2D original_transform;

	Point2 box_selecting_to;
	CursorShape cursor_shape_override = CURSOR_ARROW;

	Ref<StyleBoxTexture> select_sb;
	Ref<Texture2D> select_handle;
	Ref<Texture2D> anchor_handle;

	Ref<Shortcut> drag_pivot_shortcut;
	Ref<Shortcut> set_pivot_shortcut;
	Ref<Shortcut> multiply_grid_step_shortcut;
	Ref<Shortcut> divide_grid_step_shortcut;
	Ref<Shortcut> reset_transform_position_shortcut;
	Ref<Shortcut> reset_transform_rotation_shortcut;
	Ref<Shortcut> reset_transform_scale_shortcut;

	Ref<ViewPanner> panner;
	void _pan_callback(Vector2 p_scroll_vec, Ref<InputEvent> p_event);
	void _zoom_callback(float p_zoom_factor, Vector2 p_origin, Ref<InputEvent> p_event);

	bool _is_node_locked(const Node *p_node) const;
	bool _is_node_movable(const Node *p_node, bool p_popup_warning = false);
	void _get_canvas_items_at_pos(const Point2 &p_pos, Vector<SelectResult> &r_items, bool p_allow_locked = false);
	void _find_canvas_items_in_rect(const Rect2 &p_rect, Node *p_node, List<CanvasItem *> *r_items, const Transform2D &p_parent_xform = Transform2D(), const Transform2D &p_canvas_xform = Transform2D());

	bool _select_click_on_item(CanvasItem *item, Point2 p_click_pos, bool p_append);

	ConfirmationDialog *snap_dialog = nullptr;

	CanvasItem *ref_item = nullptr;

	void _save_canvas_item_state(const List<CanvasItem *> &p_canvas_items, bool save_bones = false);
	void _restore_canvas_item_state(const List<CanvasItem *> &p_canvas_items, bool restore_bones = false);
	void _commit_canvas_item_state(const List<CanvasItem *> &p_canvas_items, const String &action_name, bool commit_bones = false);

	Vector2 _anchor_to_position(const Control *p_control, Vector2 anchor);
	Vector2 _position_to_anchor(const Control *p_control, Vector2 position);

	void _prepare_view_menu();
	void _popup_callback(int p_op);
	bool updating_scroll = false;
	void _update_scroll(real_t);
	void _update_scrollbars();
	void _snap_changed();
	void _selection_result_pressed(int);
	void _selection_menu_hide();
	void _add_node_pressed(int p_result);
	void _adjust_new_node_position(Node *p_node);
	void _reset_create_position();
	void _update_editor_settings();
	void _prepare_grid_menu();
	void _on_grid_menu_id_pressed(int p_id);
	void _reset_transform(TransformType p_type);
	void _update_oversampling();

public:
	enum ThemePreviewMode {
		THEME_PREVIEW_PROJECT,
		THEME_PREVIEW_EDITOR,
		THEME_PREVIEW_DEFAULT,

		THEME_PREVIEW_MAX // The number of options for enumerating.
	};

private:
	ThemePreviewMode theme_preview = THEME_PREVIEW_PROJECT;
	void _switch_theme_preview(int p_mode);

	List<CanvasItem *> _get_edited_canvas_items(bool p_retrieve_locked = false, bool p_remove_canvas_item_if_parent_in_selection = true, bool *r_has_locked_items = nullptr) const;
	Rect2 _get_encompassing_rect_from_list(const List<CanvasItem *> &p_list);
	void _expand_encompassing_rect_using_children(Rect2 &r_rect, const Node *p_node, bool &r_first, const Transform2D &p_parent_xform = Transform2D(), const Transform2D &p_canvas_xform = Transform2D(), bool include_locked_nodes = true);
	Rect2 _get_encompassing_rect(const Node *p_node);

	Object *_get_editor_data(Object *p_what);

	void _insert_animation_keys(bool p_location, bool p_rotation, bool p_scale, bool p_on_existing);

	void _keying_changed();

	virtual void shortcut_input(const Ref<InputEvent> &p_ev) override;

	void _draw_text_at_position(Point2 p_position, const String &p_string, Side p_side);
	void _draw_margin_at_position(int p_value, Point2 p_position, Side p_side);
	void _draw_percentage_at_position(real_t p_value, Point2 p_position, Side p_side);
	void _draw_straight_line(Point2 p_from, Point2 p_to, Color p_color);

	void _draw_smart_snapping();
	void _draw_rulers();
	void _draw_guides();
	void _draw_focus();
	void _draw_grid();
	void _draw_ruler_tool();
	void _draw_control_anchors(Control *control);
	void _draw_control_helpers(Control *control);
	void _draw_selection();
	void _draw_axis();
	void _draw_invisible_nodes_positions(Node *p_node, const Transform2D &p_parent_xform = Transform2D(), const Transform2D &p_canvas_xform = Transform2D());
	void _draw_locks_and_groups(Node *p_node, const Transform2D &p_parent_xform = Transform2D(), const Transform2D &p_canvas_xform = Transform2D());
	void _draw_hover();
	void _draw_message();

	void _draw_viewport();

	bool _gui_input_anchors(const Ref<InputEvent> &p_event);
	bool _gui_input_move(const Ref<InputEvent> &p_event);
	bool _gui_input_open_scene_on_double_click(const Ref<InputEvent> &p_event);
	bool _gui_input_scale(const Ref<InputEvent> &p_event);
	bool _gui_input_pivot(const Ref<InputEvent> &p_event);
	bool _gui_input_resize(const Ref<InputEvent> &p_event);
	bool _gui_input_rotate(const Ref<InputEvent> &p_event);
	bool _gui_input_select(const Ref<InputEvent> &p_event);
	bool _gui_input_ruler_tool(const Ref<InputEvent> &p_event);
	bool _gui_input_zoom_or_pan(const Ref<InputEvent> &p_event, bool p_already_accepted);
	bool _gui_input_rulers_and_guides(const Ref<InputEvent> &p_event);
	bool _gui_input_hover(const Ref<InputEvent> &p_event);

	void _commit_drag();

	void _gui_input_viewport(const Ref<InputEvent> &p_event);
	void _update_cursor();
	void _update_lock_and_group_button();

	void _selection_changed();
	void _focus_selection(int p_op);
	void _reset_drag();

	void _project_settings_changed();

	SnapTarget snap_target[2];
	Transform2D snap_transform;
	void _snap_if_closer_float(
			const real_t p_value,
			real_t &r_current_snap, SnapTarget &r_current_snap_target,
			const real_t p_target_value, const SnapTarget p_snap_target,
			const real_t p_radius = 10.0);
	void _snap_if_closer_point(
			Point2 p_value,
			Point2 &r_current_snap, SnapTarget (&r_current_snap_target)[2],
			Point2 p_target_value, const SnapTarget p_snap_target,
			const real_t rotation = 0.0,
			const real_t p_radius = 10.0);
	void _snap_other_nodes(
			const Point2 p_value,
			const Transform2D p_transform_to_snap,
			Point2 &r_current_snap, SnapTarget (&r_current_snap_target)[2],
			const SnapTarget p_snap_target, List<const CanvasItem *> p_exceptions,
			const Node *p_current);

	VBoxContainer *controls_vb = nullptr;
	Button *button_center_view = nullptr;
	EditorZoomWidget *zoom_widget = nullptr;
	void _update_zoom(real_t p_zoom);
	void _shortcut_zoom_set(real_t p_zoom);
	void _zoom_on_position(real_t p_zoom, Point2 p_position = Point2());
	void _button_toggle_local_space(bool p_status);
	void _button_toggle_smart_snap(bool p_status);
	void _button_toggle_grid_snap(bool p_status);
	void _button_tool_select(int p_index);

	HSplitContainer *left_panel_split = nullptr;
	HSplitContainer *right_panel_split = nullptr;
	VSplitContainer *bottom_split = nullptr;

	void _set_owner_for_node_and_children(Node *p_node, Node *p_owner);

	friend class CanvasItemEditorPlugin;

protected:
	void _notification(int p_what);

	static void _bind_methods();

	// The instance that currently owns the 2D editing context, and every live
	// instance. More than one exists once several editor spaces are open.
	static CanvasItemEditor *active_instance;
	static Vector<CanvasItemEditor *> instances;

	// The document this view edits, held as a history id because tab indices
	// shift under it; -1 means it follows whichever document is current.
	int bound_document_id = -1;
	// Whether this view's own signals have been connected; see the 3D view.
	bool wired = false;
	int _bound_document_index() const;

	// The view renders the document's 2D world through a viewport of its own
	// rather than displaying the document's viewport directly. A SubViewport has
	// one parent, so adopting it would let only one view ever show a document,
	// and the pan and zoom transform would be shared between any that did.
	SubViewportContainer *scene_viewport_container = nullptr;
	SubViewport *view_viewport = nullptr;

public:
	enum SnapMode {
		SNAP_GRID = 1 << 0,
		SNAP_GUIDES = 1 << 1,
		SNAP_PIXEL = 1 << 2,
		SNAP_NODE_PARENT = 1 << 3,
		SNAP_NODE_ANCHORS = 1 << 4,
		SNAP_NODE_SIDES = 1 << 5,
		SNAP_NODE_CENTER = 1 << 6,
		SNAP_OTHER_NODES = 1 << 7,

		SNAP_DEFAULT = SNAP_GRID | SNAP_GUIDES | SNAP_PIXEL,
	};

	String message;

	Point2 snap_point(Point2 p_target, unsigned int p_modes = SNAP_DEFAULT, unsigned int p_forced_modes = 0, const CanvasItem *p_self_canvas_item = nullptr, const List<CanvasItem *> &p_other_nodes_exceptions = List<CanvasItem *>());
	real_t snap_angle(real_t p_target, real_t p_start = 0) const;

	Transform2D get_canvas_transform() const { return transform; }

	// Where an item sits on this view, composed from the view's own transform.
	// CanvasItem::get_global_transform_with_canvas() cannot answer this any
	// more: it reads the canvas transform of the viewport the item lives in,
	// which is the document's, and this view no longer drives that one.
	Transform2D get_item_view_transform(const CanvasItem *p_item) const;

	// The active instance. With a single editor space open this is the only
	// instance, so callers keep the behavior they had when it was a singleton.
	static CanvasItemEditor *get_singleton() { return active_instance; }
	static const Vector<CanvasItemEditor *> &get_instances() { return instances; }

	void make_active() { active_instance = this; }
	bool is_active() const { return active_instance == this; }
	static CanvasItemEditor *get_primary() { return primary_instance ? primary_instance : active_instance; }
	void make_primary() { primary_instance = this; }

	// The chrome, for checking it; null with the classic toolbar.
	enum Overlay {
		OVERLAY_GRID = 1000,
		OVERLAY_KEY_HINTS,
	};
	enum SidebarPage {
		SIDEBAR_ITEM,
		SIDEBAR_SNAP,
	};
	PanelContainer *get_tool_column() const { return tool_column_panel; }
	HBoxContainer *get_header_end() const { return header_end; }
	HFlowContainer *get_toolbar() const { return toolbar_flow; }
	HBoxContainer *get_context_toolbar() const { return context_toolbar_hbox; }
	MenuButton *get_overlays_menu() const { return overlays_menu; }
	MenuButton *get_view_menu() const { return view_menu; }
	EditorViewPill *get_view_options_pill() const { return view_options_pill; }
	EditorViewSidebar *get_sidebar() const { return sidebar; }
	Button *get_sidebar_button() const { return sidebar_button; }
	CanvasItemEditorItemPanel *get_item_panel() const { return item_panel; }
	EditorViewHints *get_hints() const { return hints; }
	bool are_helpers_shown() const { return show_helpers; }
	// Opens a pie menu - "view" - around the mouse, as its key does.
	void open_pie(const StringName &p_name, Key p_key = Key::NONE);
	EditorPieMenu *get_pie() const { return pie; }
	real_t get_zoom() const { return zoom; }

	// Points this view at the document whose 2D world it should render. Call
	// once, before the view is shown.
	void set_scene_root(SubViewport *p_scene_root);
	SubViewport *get_view_viewport() const { return view_viewport; }

	virtual void bind_document(int p_document_id) override;
	virtual int get_bound_document() const override { return bound_document_id; }
	virtual bool supports_document_binding() const override { return true; }
	virtual StringName get_panel_type() const override { return SNAME("view_2d"); }

	// The document this view edits, and its root. Everything goes through these
	// rather than asking the editor what is current, so a second view can be
	// looking at a different scene entirely.
	Node *get_edited_scene() const;
	SubViewport *get_scene_root() const;
	// Called when the edited document changes: the view follows its world.
	void update_editing_world();

	// Whether a viewport holds something this view may edit. The document's own
	// root counts; anything nested deeper still has to be on screen.
	bool is_viewport_editable(const Viewport *p_viewport) const;

	Dictionary get_state() const;
	void set_state(const Dictionary &p_state);
	void clear();

	void add_control_to_menu_panel(Control *p_control);
	void remove_control_from_menu_panel(Control *p_control);

	void add_control_to_left_panel(Control *p_control);
	void remove_control_from_left_panel(Control *p_control);

	void add_control_to_right_panel(Control *p_control);
	void remove_control_from_right_panel(Control *p_control);

	VSplitContainer *get_bottom_split();

	Control *get_viewport_control() { return viewport; }

	Control *get_controls_container() { return controls_vb; }

	void find_canvas_items_at_pos(const Point2 &p_pos, Node *p_node, Vector<SelectResult> &r_items, const Transform2D &p_parent_xform = Transform2D(), const Transform2D &p_canvas_xform = Transform2D());

	void update_viewport();
	// Redraws every view showing this document, not only this one. What changed
	// is the document, and an outline drawn over a node that moved is stale in
	// every pane drawing it.
	void redraw_views_of_document();

	Tool get_current_tool() { return tool; }
	void set_current_tool(Tool p_tool);

	bool is_grid_visible() const;
	Vector2 get_grid_step() const { return grid_step; }

	void edit(CanvasItem *p_canvas_item);

	void focus_selection();
	void center_at(const Point2 &p_pos);

	void set_cursor_shape_override(CursorShape p_shape = CURSOR_ARROW);
	virtual CursorShape get_cursor_shape(const Point2 &p_pos) const override;

	ThemePreviewMode get_theme_preview() const { return theme_preview; }

	EditorSelection *editor_selection = nullptr;

	CanvasItemEditor();
	~CanvasItemEditor();
};

class CanvasItemEditorPlugin : public EditorPlugin {
	GDCLASS(CanvasItemEditorPlugin, EditorPlugin);

	CanvasItemEditor *canvas_item_editor = nullptr;

protected:
	void _notification(int p_what);

public:
	virtual String get_plugin_name() const override { return TTRC("2D"); }
	bool has_main_screen() const override { return true; }
	virtual Control *create_main_screen_view() override;
	// Where the editor's own view stands when no pane is showing it.
	Control *parked_parent = nullptr;
	bool release_main_screen_view(Control *p_view);
	virtual StringName get_main_screen_panel_type() const override { return canvas_item_editor ? canvas_item_editor->get_panel_type() : StringName(); }
	virtual EditorDocumentView *get_main_screen_view() override { return canvas_item_editor; }
	virtual Control *get_main_screen_control() override { return canvas_item_editor; }
	virtual void edit(Object *p_object) override;
	virtual bool handles(Object *p_object) const override;
	virtual void edited_scene_changed() override;
	virtual void make_visible(bool p_visible) override;
	virtual Dictionary get_state() const override;
	virtual void set_state(const Dictionary &p_state) override;
	// The view whose pan and zoom belong to the current document, if any. Only a
	// view that follows the current document swaps scenes when the tab bar
	// moves, so only that one has a view the document can claim.
	CanvasItemEditor *_view_following_current_document() const;
	virtual void clear() override;

	CanvasItemEditor *get_canvas_item_editor() { return canvas_item_editor; }

	CanvasItemEditorPlugin();
};

class CanvasItemEditorViewport : public Control {
	GDCLASS(CanvasItemEditorViewport, Control);

	// The type of node that will be created when dropping texture into the viewport.
	String default_texture_node_type;
	// Node types that are available to select from when dropping texture into viewport.
	Vector<String> texture_node_types;

	Vector<String> selected_files;
	Node *target_node = nullptr;
	Point2 drop_pos;

	CanvasItemEditor *canvas_item_editor = nullptr;
	Control *preview_node = nullptr;
	AcceptDialog *accept = nullptr;
	AcceptDialog *texture_node_type_selector = nullptr;
	RichTextLabel *tooltip_panel = nullptr;
	Ref<ButtonGroup> button_group;

	void _on_mouse_exit();
	void _on_select_texture_node_type(Object *selected);
	void _on_change_type_confirmed();
	void _on_change_type_closed();

	void _create_preview(const Vector<String> &files) const;
	void _remove_preview();

	bool _cyclical_dependency_exists(const String &p_target_scene_path, Node *p_desired_node) const;
	bool _is_any_texture_selected() const;
	void _add_node_to_scene(Node *p_parent, Node *p_child, const Vector2 &p_target_position);
	void _create_texture_node(Node *p_parent, Node *p_child, const String &p_path, const Point2 &p_point);
	void _create_audio_node(Node *p_parent, const String &p_path, const Point2 &p_point);
	bool _create_instance(Node *p_parent, const String &p_path, const Point2 &p_point);
	void _perform_drop_data();
	void _show_texture_node_type_selector();
	void _update_theme();

	void _show_tooltip(const String &p_title, const String &p_description) const;

protected:
	void _notification(int p_what);

public:
	virtual bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
	virtual void drop_data(const Point2 &p_point, const Variant &p_data) override;

	CanvasItemEditorViewport(CanvasItemEditor *p_canvas_item_editor);
	~CanvasItemEditorViewport();
};

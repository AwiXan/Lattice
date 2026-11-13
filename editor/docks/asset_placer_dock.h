/**************************************************************************/
/*  asset_placer_dock.h                                                   */
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

#include "editor/docks/editor_dock.h"

class EditorFileSystemDirectory;
class Button;
class ConfirmationDialog;
class ColorPicker;
class EditorFileDialog;
class ItemList;
class Label;
class LineEdit;
class MenuButton;
class PopupMenu;
class Texture2D;
class Tree;
class TreeItem;

class AssetPlacerDock : public EditorDock {
	GDCLASS(AssetPlacerDock, EditorDock);

	enum CategoryMenuOption {
		CATEGORY_CREATE,
		CATEGORY_CREATE_SUBCATEGORY,
		CATEGORY_ADD_RESOURCES,
		CATEGORY_QUICK_LOAD,
		CATEGORY_RENAME,
		CATEGORY_SET_COLOR,
		CATEGORY_RESET_COLOR,
		CATEGORY_REMOVE,
	};

	enum AssetMenuOption {
		ASSET_ADD_RESOURCES,
		ASSET_QUICK_LOAD,
		ASSET_REMOVE_SELECTED,
	};

	enum CategoryDialogAction {
		CATEGORY_DIALOG_CREATE,
		CATEGORY_DIALOG_CREATE_SUBCATEGORY,
		CATEGORY_DIALOG_RENAME,
	};

	struct AssetCategory {
		String id;
		String name;
		String parent_id;
		PackedStringArray resources;
		Color color;
		bool has_custom_color = false;
	};

	Tree *categories = nullptr;
	ItemList *assets = nullptr;
	LineEdit *search_box = nullptr;
	Label *assets_title = nullptr;
	MenuButton *category_menu = nullptr;
	PopupMenu *category_popup = nullptr;
	PopupMenu *category_context_popup = nullptr;
	PopupMenu *assets_popup = nullptr;
	Button *quick_load_button = nullptr;
	Button *add_resources_button = nullptr;
	Button *remove_assets_button = nullptr;
	ConfirmationDialog *category_name_dialog = nullptr;
	ConfirmationDialog *category_remove_dialog = nullptr;
	ConfirmationDialog *category_color_dialog = nullptr;
	LineEdit *category_name_edit = nullptr;
	ColorPicker *category_color_picker = nullptr;
	EditorFileDialog *resource_file_dialog = nullptr;
	String selected_category_id;
	String dialog_parent_category_id;
	String pending_resource_category_id;
	String category_to_remove_id;
	String category_to_color_id;
	CategoryDialogAction category_dialog_action = CATEGORY_DIALOG_CREATE;
	Vector<AssetCategory> category_data;
	uint64_t next_category_id = 1;

	void _filesystem_changed();
	void _update_theme_icons();
	void _update_category_popup_icons(PopupMenu *p_popup);
	void _setup_category_popup(PopupMenu *p_popup);
	void _load_categories();
	void _save_categories();
	void _rebuild_categories();
	void _create_category_items(TreeItem *p_parent, const String &p_parent_id);
	int _find_category_index(const String &p_id) const;
	void _category_selected();
	void _search_changed(const String &p_text);
	void _asset_selection_changed(int p_index, bool p_selected);
	void _remove_selected_assets();
	void _update_assets();
	bool _is_placeable_resource(EditorFileSystemDirectory *p_directory, int p_index) const;
	bool _is_placeable_resource(const String &p_path) const;
	void _add_resources_to_category(const Vector<String> &p_paths, const String &p_category_id);
	void _thumbnail_done(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &p_small_preview, int p_index, const String &p_filename);
	void _show_category_name_dialog(CategoryDialogAction p_action);
	void _category_name_confirmed();
	void _category_name_submitted(const String &p_name);
	void _category_remove_requested();
	void _category_remove_confirmed();
	void _category_menu_about_to_popup();
	void _category_menu_option(int p_option);
	void _show_category_color_dialog();
	void _category_color_confirmed();
	void _reset_category_color();
	void _category_item_mouse_selected(const Vector2 &p_position, MouseButton p_button);
	void _assets_item_clicked(int p_item, const Vector2 &p_position, MouseButton p_button);
	void _assets_menu_option(int p_option);
	void _asset_activated(int p_index);
	void _browse_resources();
	void _quick_load_resource();
	void _quick_resource_selected(const String &p_path);
	void _file_dialog_resources_selected(const PackedStringArray &p_paths);

	Variant get_drag_data_fw(const Point2 &p_point, Control *p_from);
	bool can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const;
	void drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from);

protected:
	void _notification(int p_what);

public:
	AssetPlacerDock();
};

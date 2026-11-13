/**************************************************************************/
/*  asset_placer_dock.cpp                                                 */
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

#include "asset_placer_dock.h"

#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/class_db.h"
#include "core/templates/hash_set.h"
#include "editor/docks/filesystem_dock.h"
#include "editor/editor_node.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/gui/editor_quick_open_dialog.h"
#include "editor/inspector/editor_resource_preview.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/color_picker.h"
#include "scene/gui/label.h"
#include "scene/gui/item_list.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/menu_button.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/split_container.h"
#include "scene/gui/tree.h"

void AssetPlacerDock::_filesystem_changed() {
	_rebuild_categories();
}

void AssetPlacerDock::_update_theme_icons() {
	category_menu->set_button_icon(get_editor_theme_icon(SNAME("Add")));
	quick_load_button->set_button_icon(get_editor_theme_icon(SNAME("LoadQuick")));
	add_resources_button->set_button_icon(get_editor_theme_icon(SNAME("Add")));
	remove_assets_button->set_button_icon(get_editor_theme_icon(SNAME("Remove")));

	_update_category_popup_icons(category_popup);
	_update_category_popup_icons(category_context_popup);

	assets_popup->set_item_icon(assets_popup->get_item_index(ASSET_ADD_RESOURCES), get_editor_theme_icon(SNAME("Add")));
	assets_popup->set_item_icon(assets_popup->get_item_index(ASSET_QUICK_LOAD), get_editor_theme_icon(SNAME("LoadQuick")));
	assets_popup->set_item_icon(assets_popup->get_item_index(ASSET_REMOVE_SELECTED), get_editor_theme_icon(SNAME("Remove")));
}

void AssetPlacerDock::_update_category_popup_icons(PopupMenu *p_popup) {
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_CREATE), get_editor_theme_icon(SNAME("Add")));
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_CREATE_SUBCATEGORY), get_editor_theme_icon(SNAME("Add")));
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_ADD_RESOURCES), get_editor_theme_icon(SNAME("Add")));
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_QUICK_LOAD), get_editor_theme_icon(SNAME("LoadQuick")));
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_RENAME), get_editor_theme_icon(SNAME("Rename")));
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_SET_COLOR), get_editor_theme_icon(SNAME("Paint")));
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_RESET_COLOR), get_editor_theme_icon(SNAME("Folder")));
	p_popup->set_item_icon(p_popup->get_item_index(CATEGORY_REMOVE), get_editor_theme_icon(SNAME("Remove")));
}

void AssetPlacerDock::_setup_category_popup(PopupMenu *p_popup) {
	p_popup->add_item(TTR("New Category"), CATEGORY_CREATE);
	p_popup->add_item(TTR("New Subcategory"), CATEGORY_CREATE_SUBCATEGORY);
	p_popup->add_separator();
	p_popup->add_item(TTR("Add Resources..."), CATEGORY_ADD_RESOURCES);
	p_popup->add_item(TTR("Quick Load..."), CATEGORY_QUICK_LOAD);
	p_popup->add_separator();
	p_popup->add_item(TTR("Rename"), CATEGORY_RENAME);
	p_popup->add_item(TTR("Set Folder Color..."), CATEGORY_SET_COLOR);
	p_popup->add_item(TTR("Reset Folder Color"), CATEGORY_RESET_COLOR);
	p_popup->add_item(TTR("Remove"), CATEGORY_REMOVE);
	p_popup->connect(SceneStringName(id_pressed), callable_mp(this, &AssetPlacerDock::_category_menu_option));
	p_popup->connect("about_to_popup", callable_mp(this, &AssetPlacerDock::_category_menu_about_to_popup));
}

void AssetPlacerDock::_load_categories() {
	category_data.clear();
	next_category_id = 1;

	const Array saved_categories = EditorSettings::get_singleton()->get_project_metadata("asset_placer", "categories", Array());
	HashSet<String> loaded_ids;
	for (int i = 0; i < saved_categories.size(); i++) {
		const Dictionary category_dictionary = saved_categories[i];
		AssetCategory category;
		category.id = category_dictionary.get("id", "");
		category.name = category_dictionary.get("name", "");
		category.parent_id = category_dictionary.get("parent_id", "");
		category.has_custom_color = category_dictionary.get("has_custom_color", false);
		category.color = category_dictionary.get("color", Color());
		if (category.name.strip_edges().is_empty() || category.id.is_empty() || loaded_ids.has(category.id)) {
			continue;
		}

		const Array saved_resources = category_dictionary.get("resources", Array());
		for (int j = 0; j < saved_resources.size(); j++) {
			const String path = saved_resources[j];
			if (!path.is_empty() && !category.resources.has(path)) {
				category.resources.push_back(path);
			}
		}

		loaded_ids.insert(category.id);
		next_category_id = MAX(next_category_id, (uint64_t)category.id.to_int() + 1);
		category_data.push_back(category);
	}

	for (AssetCategory &category : category_data) {
		if (!category.parent_id.is_empty() && _find_category_index(category.parent_id) == -1) {
			category.parent_id = "";
		}
	}
}

void AssetPlacerDock::_save_categories() {
	Array serialized_categories;
	for (const AssetCategory &category : category_data) {
		Dictionary category_dictionary;
		category_dictionary["id"] = category.id;
		category_dictionary["name"] = category.name;
		category_dictionary["parent_id"] = category.parent_id;
		category_dictionary["has_custom_color"] = category.has_custom_color;
		category_dictionary["color"] = category.color;

		Array resources;
		for (const String &path : category.resources) {
			resources.push_back(path);
		}
		category_dictionary["resources"] = resources;
		serialized_categories.push_back(category_dictionary);
	}

	EditorSettings::get_singleton()->set_project_metadata("asset_placer", "categories", serialized_categories);
	EditorSettings::get_singleton()->save_project_metadata();
}

void AssetPlacerDock::_rebuild_categories() {
	categories->clear();
	TreeItem *root = categories->create_item();
	_create_category_items(root, "");

	if (categories->get_selected() == nullptr && root->get_first_child()) {
		root->get_first_child()->select(0);
	}

	if (TreeItem *selected = categories->get_selected()) {
		selected_category_id = selected->get_metadata(0);
	} else {
		selected_category_id = "";
	}
	_update_assets();
}

void AssetPlacerDock::_create_category_items(TreeItem *p_parent, const String &p_parent_id) {
	for (const AssetCategory &category : category_data) {
		if (category.parent_id != p_parent_id) {
			continue;
		}

		TreeItem *item = categories->create_item(p_parent);
		item->set_text(0, category.name);
		item->set_metadata(0, category.id);
		item->set_icon(0, get_editor_theme_icon(SNAME("Folder")));
		item->set_icon_modulate(0, category.has_custom_color ? category.color : get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog")));
		if (category.id == selected_category_id) {
			item->select(0);
		}
		_create_category_items(item, category.id);
	}
}

int AssetPlacerDock::_find_category_index(const String &p_id) const {
	for (int i = 0; i < category_data.size(); i++) {
		if (category_data[i].id == p_id) {
			return i;
		}
	}
	return -1;
}

void AssetPlacerDock::_category_selected() {
	TreeItem *selected = categories->get_selected();
	selected_category_id = selected ? String(selected->get_metadata(0)) : String();
	_update_assets();
}

void AssetPlacerDock::_search_changed(const String &) {
	_update_assets();
}

void AssetPlacerDock::_asset_selection_changed(int, bool) {
	remove_assets_button->set_disabled(assets->get_selected_items().is_empty());
}

void AssetPlacerDock::_remove_selected_assets() {
	const int category_index = _find_category_index(selected_category_id);
	if (category_index == -1) {
		return;
	}

	const Vector<int> selected_items = assets->get_selected_items();
	if (selected_items.is_empty()) {
		return;
	}

	HashSet<String> paths_to_remove;
	for (int item_index : selected_items) {
		paths_to_remove.insert(assets->get_item_metadata(item_index));
	}

	PackedStringArray remaining_resources;
	for (const String &path : category_data[category_index].resources) {
		if (!paths_to_remove.has(path)) {
			remaining_resources.push_back(path);
		}
	}
	category_data.write[category_index].resources = remaining_resources;
	_save_categories();
	_update_assets();
}

bool AssetPlacerDock::_is_placeable_resource(EditorFileSystemDirectory *p_directory, int p_index) const {
	const StringName type = p_directory->get_file_type(p_index);
	if (type.is_empty() || type == SNAME("Script") || ClassDB::is_parent_class(type, SNAME("Script"))) {
		return false;
	}
	return type == SNAME("PackedScene") || ClassDB::is_parent_class(type, SNAME("Resource")) ||
			p_directory->get_file_resource_script_class(p_index) != StringName();
}

bool AssetPlacerDock::_is_placeable_resource(const String &p_path) const {
	int file_index;
	EditorFileSystemDirectory *directory = EditorFileSystem::get_singleton()->find_file(p_path, &file_index);
	return directory && _is_placeable_resource(directory, file_index);
}

void AssetPlacerDock::_add_resources_to_category(const Vector<String> &p_paths, const String &p_category_id) {
	const int category_index = _find_category_index(p_category_id);
	if (category_index == -1) {
		EditorNode::get_singleton()->show_warning(TTR("Select a category before adding resources."));
		return;
	}

	bool changed = false;
	Vector<String> rejected_paths;
	for (const String &path : p_paths) {
		if (!_is_placeable_resource(path)) {
			rejected_paths.push_back(path);
			continue;
		}

		if (!category_data[category_index].resources.has(path)) {
			category_data.write[category_index].resources.push_back(path);
			changed = true;
		}
	}

	if (changed) {
		_save_categories();
		if (p_category_id == selected_category_id) {
			_update_assets();
		}
	}

	if (!rejected_paths.is_empty()) {
		EditorNode::get_singleton()->show_warning(vformat(TTR("These files are not supported resources and were not added:\n%s"), String("\n").join(rejected_paths)));
	}
}

void AssetPlacerDock::_update_assets() {
	assets->clear();
	remove_assets_button->set_disabled(true);

	const int category_index = _find_category_index(selected_category_id);
	if (category_index == -1) {
		assets_title->set_text(TTR("Create or select a category"));
		return;
	}

	assets_title->set_text(category_data[category_index].name);
	const String filter = search_box->get_text().to_lower();
	for (const String &path : category_data[category_index].resources) {
		const String filename = path.get_file().get_basename();
		if (!filter.is_empty() && !filename.to_lower().contains(filter)) {
			continue;
		}

		int file_index;
		EditorFileSystemDirectory *directory = EditorFileSystem::get_singleton()->find_file(path, &file_index);
		Ref<Texture2D> type_icon;
		Color scene_background;
		const bool is_scene = directory && directory->get_file_type(file_index) == SNAME("PackedScene");
		if (directory) {
			if (is_scene) {
				type_icon = FileSystemDock::get_singleton()->get_scene_class_icon(path, scene_background);
			} else {
				const String icon_path = directory->get_file_icon_path(file_index);
				if (!icon_path.is_empty()) {
					type_icon = ResourceLoader::load(icon_path);
				}
			}
			if (type_icon.is_null()) {
				type_icon = EditorNode::get_singleton()->get_class_icon(directory->get_file_type(file_index), "File");
			}
		} else {
			type_icon = get_editor_theme_icon(SNAME("ImportFail"));
		}

		const int item_index = assets->add_item(filename, type_icon, true);
		assets->set_item_metadata(item_index, path);
		assets->set_item_tooltip(item_index, path);
		if (is_scene) {
			assets->set_item_custom_bg_color(item_index, scene_background);
			assets->set_item_custom_bg_corner_radius(item_index, 8.0 * EDSCALE);
		}
		if (directory) {
			assets->set_item_tag_icon(item_index, type_icon);
			if (directory->get_file_import_is_valid(file_index)) {
				EditorResourcePreview::get_singleton()->queue_resource_preview(path, callable_mp(this, &AssetPlacerDock::_thumbnail_done).bind(item_index, filename));
			}
		} else {
			assets->set_item_tooltip(item_index, vformat(TTR("%s\nThis resource is missing from the project."), path));
		}
	}
}

void AssetPlacerDock::_thumbnail_done(const String &p_path, const Ref<Texture2D> &p_preview, const Ref<Texture2D> &, int p_index, const String &p_filename) {
	if (p_preview.is_valid() && p_index < assets->get_item_count() &&
			assets->get_item_text(p_index) == p_filename && assets->get_item_metadata(p_index) == p_path) {
		assets->set_item_icon(p_index, p_preview);
	}
}

void AssetPlacerDock::_category_menu_about_to_popup() {
	const bool has_category = _find_category_index(selected_category_id) != -1;
	PopupMenu *menus[] = { category_popup, category_context_popup };
	for (PopupMenu *menu : menus) {
		menu->set_item_disabled(menu->get_item_index(CATEGORY_CREATE_SUBCATEGORY), !has_category);
		menu->set_item_disabled(menu->get_item_index(CATEGORY_ADD_RESOURCES), !has_category);
		menu->set_item_disabled(menu->get_item_index(CATEGORY_QUICK_LOAD), !has_category);
		menu->set_item_disabled(menu->get_item_index(CATEGORY_RENAME), !has_category);
		menu->set_item_disabled(menu->get_item_index(CATEGORY_SET_COLOR), !has_category);
		menu->set_item_disabled(menu->get_item_index(CATEGORY_RESET_COLOR), !has_category);
		menu->set_item_disabled(menu->get_item_index(CATEGORY_REMOVE), !has_category);
	}
}

void AssetPlacerDock::_category_menu_option(int p_option) {
	switch (p_option) {
		case CATEGORY_CREATE:
			_show_category_name_dialog(CATEGORY_DIALOG_CREATE);
			break;
		case CATEGORY_CREATE_SUBCATEGORY:
			_show_category_name_dialog(CATEGORY_DIALOG_CREATE_SUBCATEGORY);
			break;
		case CATEGORY_ADD_RESOURCES:
			_browse_resources();
			break;
		case CATEGORY_QUICK_LOAD:
			_quick_load_resource();
			break;
		case CATEGORY_RENAME:
			_show_category_name_dialog(CATEGORY_DIALOG_RENAME);
			break;
		case CATEGORY_SET_COLOR:
			_show_category_color_dialog();
			break;
		case CATEGORY_RESET_COLOR:
			_reset_category_color();
			break;
		case CATEGORY_REMOVE:
			_category_remove_requested();
			break;
	}
}

void AssetPlacerDock::_show_category_color_dialog() {
	const int category_index = _find_category_index(selected_category_id);
	if (category_index == -1) {
		return;
	}

	category_to_color_id = selected_category_id;
	category_color_picker->set_pick_color(category_data[category_index].has_custom_color ? category_data[category_index].color : get_theme_color(SNAME("folder_icon_color"), SNAME("FileDialog")));
	category_color_dialog->popup_centered(Size2(360, 420) * EDSCALE);
}

void AssetPlacerDock::_category_color_confirmed() {
	const int category_index = _find_category_index(category_to_color_id);
	if (category_index == -1) {
		return;
	}

	category_data.write[category_index].color = category_color_picker->get_pick_color();
	category_data.write[category_index].has_custom_color = true;
	_save_categories();
	_rebuild_categories();
}

void AssetPlacerDock::_reset_category_color() {
	const int category_index = _find_category_index(selected_category_id);
	if (category_index == -1) {
		return;
	}

	category_data.write[category_index].has_custom_color = false;
	_save_categories();
	_rebuild_categories();
}

void AssetPlacerDock::_category_item_mouse_selected(const Vector2 &p_position, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	categories->grab_focus(true);
	TreeItem *item = categories->get_item_at_position(p_position);
	if (item && _find_category_index(item->get_metadata(0)) != -1) {
		item->select(0);
		selected_category_id = item->get_metadata(0);
	} else {
		categories->deselect_all();
		selected_category_id = "";
	}
	_update_assets();

	_category_menu_about_to_popup();
	category_context_popup->set_position(categories->get_screen_position() + p_position);
	category_context_popup->reset_size();
	category_context_popup->popup();
}

void AssetPlacerDock::_assets_item_clicked(int p_item, const Vector2 &p_position, MouseButton p_button) {
	if (p_button != MouseButton::RIGHT) {
		return;
	}

	if (p_item >= 0 && !assets->is_selected(p_item)) {
		assets->deselect_all();
		assets->select(p_item);
		_asset_selection_changed(p_item, true);
	}

	assets_popup->set_item_disabled(assets_popup->get_item_index(ASSET_REMOVE_SELECTED), assets->get_selected_items().is_empty());
	assets_popup->set_position(assets->get_screen_position() + p_position);
	assets_popup->reset_size();
	assets_popup->popup();
}

void AssetPlacerDock::_assets_menu_option(int p_option) {
	switch (p_option) {
		case ASSET_ADD_RESOURCES:
			_browse_resources();
			break;
		case ASSET_QUICK_LOAD:
			_quick_load_resource();
			break;
		case ASSET_REMOVE_SELECTED:
			_remove_selected_assets();
			break;
	}
}

void AssetPlacerDock::_asset_activated(int p_index) {
	if (p_index < 0 || p_index >= assets->get_item_count()) {
		return;
	}

	EditorNode::get_singleton()->load_scene_or_resource(assets->get_item_metadata(p_index));
}

void AssetPlacerDock::_show_category_name_dialog(CategoryDialogAction p_action) {
	category_dialog_action = p_action;
	dialog_parent_category_id = "";

	if (p_action != CATEGORY_DIALOG_CREATE) {
		const int category_index = _find_category_index(selected_category_id);
		if (category_index == -1) {
			return;
		}
		dialog_parent_category_id = p_action == CATEGORY_DIALOG_CREATE_SUBCATEGORY ? selected_category_id : category_data[category_index].parent_id;
		category_name_edit->set_text(p_action == CATEGORY_DIALOG_RENAME ? category_data[category_index].name : "");
	} else {
		category_name_edit->set_text("");
	}

	if (p_action == CATEGORY_DIALOG_RENAME) {
		category_name_dialog->set_title(TTRC("Rename Category"));
		category_name_dialog->get_ok_button()->set_text(TTRC("Rename"));
	} else {
		category_name_dialog->set_title(p_action == CATEGORY_DIALOG_CREATE_SUBCATEGORY ? TTRC("Create Subcategory") : TTRC("Create Category"));
		category_name_dialog->get_ok_button()->set_text(TTRC("Create"));
	}

	category_name_dialog->popup_centered(Size2(360, 100) * EDSCALE);
	category_name_edit->grab_focus();
	category_name_edit->select_all();
}

void AssetPlacerDock::_category_name_confirmed() {
	const String name = category_name_edit->get_text().strip_edges();
	if (name.is_empty() || name.contains("/") || name.contains("\\")) {
		EditorNode::get_singleton()->show_warning(TTR("Category names must not be empty or contain path separators."));
		return;
	}

	const int edited_index = category_dialog_action == CATEGORY_DIALOG_RENAME ? _find_category_index(selected_category_id) : -1;
	if (category_dialog_action == CATEGORY_DIALOG_RENAME && edited_index == -1) {
		return;
	}
	const String parent_id = category_dialog_action == CATEGORY_DIALOG_RENAME ? category_data[edited_index].parent_id : dialog_parent_category_id;
	for (int i = 0; i < category_data.size(); i++) {
		if (i != edited_index && category_data[i].parent_id == parent_id && category_data[i].name.nocasecmp_to(name) == 0) {
			EditorNode::get_singleton()->show_warning(TTR("A category with this name already exists here."));
			return;
		}
	}

	if (category_dialog_action == CATEGORY_DIALOG_RENAME) {
		category_data.write[edited_index].name = name;
	} else {
		AssetCategory category;
		category.id = itos(next_category_id++);
		category.name = name;
		category.parent_id = parent_id;
		category_data.push_back(category);
		selected_category_id = category.id;
	}

	_save_categories();
	_rebuild_categories();
}

void AssetPlacerDock::_category_name_submitted(const String &) {
	category_name_dialog->hide();
	_category_name_confirmed();
}

void AssetPlacerDock::_category_remove_requested() {
	const int category_index = _find_category_index(selected_category_id);
	if (category_index == -1) {
		return;
	}

	category_to_remove_id = selected_category_id;
	category_remove_dialog->set_text(vformat(TTR("Remove \"%s\" and all its subcategories from Asset Placer? Resources will remain in the project."), category_data[category_index].name));
	category_remove_dialog->popup_centered();
}

void AssetPlacerDock::_category_remove_confirmed() {
	const int category_index = _find_category_index(category_to_remove_id);
	if (category_index == -1) {
		return;
	}

	const String parent_id = category_data[category_index].parent_id;
	HashSet<String> removed_ids;
	removed_ids.insert(category_to_remove_id);
	bool found_child = true;
	while (found_child) {
		found_child = false;
		for (const AssetCategory &category : category_data) {
			if (removed_ids.has(category.parent_id) && !removed_ids.has(category.id)) {
				removed_ids.insert(category.id);
				found_child = true;
			}
		}
	}

	for (int i = category_data.size() - 1; i >= 0; i--) {
		if (removed_ids.has(category_data[i].id)) {
			category_data.remove_at(i);
		}
	}
	selected_category_id = parent_id;
	_save_categories();
	_rebuild_categories();
}

void AssetPlacerDock::_browse_resources() {
	if (_find_category_index(selected_category_id) == -1) {
		EditorNode::get_singleton()->show_warning(TTR("Select a category before adding resources."));
		return;
	}

	pending_resource_category_id = selected_category_id;
	resource_file_dialog->popup_file_dialog();
}

void AssetPlacerDock::_quick_load_resource() {
	if (_find_category_index(selected_category_id) == -1) {
		EditorNode::get_singleton()->show_warning(TTR("Select a category before adding resources."));
		return;
	}

	pending_resource_category_id = selected_category_id;
	Vector<StringName> base_types;
	base_types.push_back(SNAME("Resource"));
	EditorNode::get_singleton()->get_quick_open_dialog()->popup_dialog(base_types, callable_mp(this, &AssetPlacerDock::_quick_resource_selected));
}

void AssetPlacerDock::_file_dialog_resources_selected(const PackedStringArray &p_paths) {
	Vector<String> paths;
	for (const String &path : p_paths) {
		paths.push_back(path);
	}
	_add_resources_to_category(paths, pending_resource_category_id);
}

void AssetPlacerDock::_quick_resource_selected(const String &p_path) {
	Vector<String> paths;
	paths.push_back(p_path);
	_add_resources_to_category(paths, pending_resource_category_id);
}

Variant AssetPlacerDock::get_drag_data_fw(const Point2 &p_point, Control *p_from) {
	if (p_from != assets) {
		return Variant();
	}

	const int hovered_item = assets->get_item_at_position(p_point, true);
	if (hovered_item < 0 || !assets->is_selected(hovered_item)) {
		return Variant();
	}

	Vector<String> paths;
	for (int i = 0; i < assets->get_item_count(); i++) {
		if (assets->is_selected(i)) {
			paths.push_back(assets->get_item_metadata(i));
		}
	}
	if (paths.is_empty()) {
		return Variant();
	}

	return EditorNode::get_singleton()->drag_files_and_dirs(paths, p_from);
}

bool AssetPlacerDock::can_drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) const {
	if (p_from != categories || p_data.get_type() != Variant::DICTIONARY) {
		return false;
	}

	const Dictionary drag_data = p_data;
	if (!drag_data.has("type") || !drag_data.has("files")) {
		return false;
	}
	const String type = drag_data["type"];
	if (type != "files" && type != "files_and_dirs") {
		return false;
	}

	TreeItem *target = categories->get_item_at_position(p_point);
	if (!target || _find_category_index(target->get_metadata(0)) == -1) {
		return false;
	}

	const Vector<String> paths = drag_data["files"];
	if (paths.is_empty()) {
		return false;
	}
	for (const String &path : paths) {
		if (path.ends_with("/") || !_is_placeable_resource(path)) {
			return false;
		}
	}
	return true;
}

void AssetPlacerDock::drop_data_fw(const Point2 &p_point, const Variant &p_data, Control *p_from) {
	if (!can_drop_data_fw(p_point, p_data, p_from)) {
		return;
	}

	TreeItem *target = categories->get_item_at_position(p_point);
	const Dictionary drag_data = p_data;
	_add_resources_to_category(drag_data["files"], target->get_metadata(0));
}

void AssetPlacerDock::_notification(int p_what) {
	if (p_what == NOTIFICATION_READY) {
		EditorFileSystem::get_singleton()->connect("filesystem_changed", callable_mp(this, &AssetPlacerDock::_filesystem_changed));
		_load_categories();
		_filesystem_changed();
	} else if (p_what == NOTIFICATION_POSTINITIALIZE) {
		_update_theme_icons();
	} else if (p_what == NOTIFICATION_THEME_CHANGED && categories) {
		_update_theme_icons();
		_rebuild_categories();
	}
}

AssetPlacerDock::AssetPlacerDock() {
	set_name(TTRC("Asset Placer"));
	set_icon_name("PackedScene");
	set_default_slot(EditorDock::DOCK_SLOT_LEFT_BR);

	VBoxContainer *main_vbox = memnew(VBoxContainer);
	add_child(main_vbox);

	SplitContainer *split = memnew(SplitContainer);
	split->set_v_size_flags(SIZE_EXPAND_FILL);
	main_vbox->add_child(split);

	VBoxContainer *category_panel = memnew(VBoxContainer);
	category_panel->set_custom_minimum_size(Size2(140 * EDSCALE, 0));
	split->add_child(category_panel);

	HBoxContainer *category_header = memnew(HBoxContainer);
	category_panel->add_child(category_header);

	Label *category_label = memnew(Label);
	category_label->set_text(TTRC("Categories"));
	category_label->set_h_size_flags(SIZE_EXPAND_FILL);
	category_header->add_child(category_label);

	category_menu = memnew(MenuButton);
	category_menu->set_accessibility_name(TTRC("Category Actions"));
	category_menu->set_tooltip_text(TTRC("Manage categories, add resources, and set folder colors."));
	category_menu->set_theme_type_variation("FlatMenuButton");
	category_header->add_child(category_menu);

	category_popup = category_menu->get_popup();
	_setup_category_popup(category_popup);

	category_context_popup = memnew(PopupMenu);
	_setup_category_popup(category_context_popup);
	add_child(category_context_popup);

	categories = memnew(Tree);
	categories->set_accessibility_name(TTRC("Asset Categories"));
	categories->set_hide_root(true);
	categories->set_v_size_flags(SIZE_EXPAND_FILL);
	categories->set_drop_mode_flags(Tree::DROP_MODE_ON_ITEM);
	categories->set_allow_rmb_select(true);
	categories->connect(SceneStringName(item_selected), callable_mp(this, &AssetPlacerDock::_category_selected));
	categories->connect("item_mouse_selected", callable_mp(this, &AssetPlacerDock::_category_item_mouse_selected));
	categories->connect("empty_clicked", callable_mp(this, &AssetPlacerDock::_category_item_mouse_selected));
	SET_DRAG_FORWARDING_GCD(categories, AssetPlacerDock);
	category_panel->add_child(categories);

	VBoxContainer *asset_panel = memnew(VBoxContainer);
	asset_panel->set_v_size_flags(SIZE_EXPAND_FILL);
	split->add_child(asset_panel);

	HBoxContainer *asset_toolbar = memnew(HBoxContainer);
	asset_panel->add_child(asset_toolbar);

	assets_title = memnew(Label);
	assets_title->set_text(TTRC("Resources"));
	assets_title->set_clip_text(true);
	assets_title->set_h_size_flags(SIZE_EXPAND_FILL);
	asset_toolbar->add_child(assets_title);

	search_box = memnew(LineEdit);
	search_box->set_accessibility_name(TTRC("Filter Assets"));
	search_box->set_placeholder(TTRC("Filter"));
	search_box->set_clear_button_enabled(true);
	search_box->set_custom_minimum_size(Size2(100 * EDSCALE, 0));
	search_box->connect(SceneStringName(text_changed), callable_mp(this, &AssetPlacerDock::_search_changed));
	asset_toolbar->add_child(search_box);

	quick_load_button = memnew(Button);
	quick_load_button->set_accessibility_name(TTRC("Quick Load Resource"));
	quick_load_button->set_tooltip_text(TTRC("Quick Load..."));
	quick_load_button->set_theme_type_variation("FlatMenuButton");
	quick_load_button->connect(SceneStringName(pressed), callable_mp(this, &AssetPlacerDock::_quick_load_resource));
	asset_toolbar->add_child(quick_load_button);

	add_resources_button = memnew(Button);
	add_resources_button->set_accessibility_name(TTRC("Add Resources"));
	add_resources_button->set_tooltip_text(TTRC("Browse and add resources to this category."));
	add_resources_button->set_theme_type_variation("FlatMenuButton");
	add_resources_button->connect(SceneStringName(pressed), callable_mp(this, &AssetPlacerDock::_browse_resources));
	asset_toolbar->add_child(add_resources_button);

	remove_assets_button = memnew(Button);
	remove_assets_button->set_accessibility_name(TTRC("Remove Selected Assets"));
	remove_assets_button->set_tooltip_text(TTRC("Remove selected resources from this category."));
	remove_assets_button->set_theme_type_variation("FlatMenuButton");
	remove_assets_button->set_disabled(true);
	remove_assets_button->connect(SceneStringName(pressed), callable_mp(this, &AssetPlacerDock::_remove_selected_assets));
	asset_toolbar->add_child(remove_assets_button);

	assets = memnew(ItemList);
	assets->set_accessibility_name(TTRC("Assets"));
	assets->set_select_mode(ItemList::SELECT_MULTI);
	assets->set_max_columns(0);
	assets->set_icon_mode(ItemList::ICON_MODE_TOP);
	assets->set_fixed_icon_size(Size2(64, 64) * EDSCALE);
	assets->set_max_text_lines(2);
	assets->set_v_size_flags(SIZE_EXPAND_FILL);
	assets->set_theme_type_variation("ItemListSecondary");
	assets->connect("multi_selected", callable_mp(this, &AssetPlacerDock::_asset_selection_changed));
	assets->connect("item_clicked", callable_mp(this, &AssetPlacerDock::_assets_item_clicked));
	assets->connect("item_activated", callable_mp(this, &AssetPlacerDock::_asset_activated));
	SET_DRAG_FORWARDING_GCD(assets, AssetPlacerDock);
	asset_panel->add_child(assets);

	assets_popup = memnew(PopupMenu);
	assets_popup->add_item(TTR("Add Resources..."), ASSET_ADD_RESOURCES);
	assets_popup->add_item(TTR("Quick Load..."), ASSET_QUICK_LOAD);
	assets_popup->add_separator();
	assets_popup->add_item(TTR("Remove Selected"), ASSET_REMOVE_SELECTED);
	assets_popup->connect(SceneStringName(id_pressed), callable_mp(this, &AssetPlacerDock::_assets_menu_option));
	add_child(assets_popup);

	category_name_dialog = memnew(ConfirmationDialog);
	category_name_dialog->set_title(TTRC("Create Category"));
	category_name_dialog->get_ok_button()->set_text(TTRC("Create"));
	add_child(category_name_dialog);
	VBoxContainer *name_dialog_content = memnew(VBoxContainer);
	category_name_dialog->add_child(name_dialog_content);
	category_name_edit = memnew(LineEdit);
	category_name_edit->set_placeholder(TTRC("Category name"));
	category_name_edit->connect(SceneStringName(text_submitted), callable_mp(this, &AssetPlacerDock::_category_name_submitted));
	name_dialog_content->add_child(category_name_edit);
	category_name_dialog->connect(SceneStringName(confirmed), callable_mp(this, &AssetPlacerDock::_category_name_confirmed));

	category_remove_dialog = memnew(ConfirmationDialog);
	category_remove_dialog->set_title(TTRC("Remove Category"));
	category_remove_dialog->get_ok_button()->set_text(TTRC("Remove"));
	category_remove_dialog->connect(SceneStringName(confirmed), callable_mp(this, &AssetPlacerDock::_category_remove_confirmed));
	add_child(category_remove_dialog);

	category_color_dialog = memnew(ConfirmationDialog);
	category_color_dialog->set_title(TTRC("Set Folder Color"));
	category_color_dialog->get_ok_button()->set_text(TTRC("Apply"));
	category_color_dialog->connect(SceneStringName(confirmed), callable_mp(this, &AssetPlacerDock::_category_color_confirmed));
	add_child(category_color_dialog);
	category_color_picker = memnew(ColorPicker);
	category_color_picker->set_custom_minimum_size(Size2(320, 320) * EDSCALE);
	category_color_dialog->add_child(category_color_picker);

	resource_file_dialog = memnew(EditorFileDialog);
	resource_file_dialog->set_title(TTRC("Add Resources"));
	resource_file_dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
	resource_file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILES);
	resource_file_dialog->set_current_dir("res://");
	resource_file_dialog->connect("files_selected", callable_mp(this, &AssetPlacerDock::_file_dialog_resources_selected));
	add_child(resource_file_dialog);
}

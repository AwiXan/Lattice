/**************************************************************************/
/*  cargo_manager_dock.h                                                  */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/

#pragma once

#include "core/object/callable_mp.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/item_list.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/rich_text_label.h"
#include "scene/gui/tab_container.h"

class CargoManagerDock : public VBoxContainer {
	GDCLASS(CargoManagerDock, VBoxContainer);

	static CargoManagerDock *singleton;

	Label *status_label = nullptr;
	Button *btn_build_debug = nullptr;
	Button *btn_build_release = nullptr;
	Button *btn_check = nullptr;
	Button *btn_clean = nullptr;
	Button *btn_reload = nullptr;
	Button *btn_open_toml = nullptr;

	TabContainer *tabs = nullptr;
	RichTextLabel *log_view = nullptr;
	Button *btn_clear_log = nullptr;
	Label *build_status_label = nullptr;

	ItemList *deps_list = nullptr;
	LineEdit *dep_name_edit = nullptr;
	LineEdit *dep_version_edit = nullptr;
	Button *btn_add_dep = nullptr;
	Button *btn_refresh_deps = nullptr;

	Label *scaffold_info = nullptr;
	Button *btn_create_scaffold = nullptr;

	bool is_building = false;

	struct BuildTaskData {
		String action;
		bool is_release = false;
		String output;
		int exit_code = 0;
		Error err = OK;
	};

	static void _build_worker(void *p_userdata);
	void _on_build_completed(String p_action, int p_exit_code, String p_output, int p_err);

	void _on_build_debug();
	void _on_build_release();
	void _on_check();
	void _on_clean();
	void _on_reload();
	void _on_open_toml();
	void _on_clear_log();
	void _on_add_dep();
	void _on_refresh_deps();
	void _on_create_scaffold();

	void _start_cargo_task(const String &p_action, bool p_release);
	void _update_status();
	void _update_deps();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	static CargoManagerDock *get_singleton() { return singleton; }

	void trigger_build(bool p_release = false);

	CargoManagerDock();
	~CargoManagerDock();
};

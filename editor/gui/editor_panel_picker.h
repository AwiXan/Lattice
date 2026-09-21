/**************************************************************************/
/*  editor_panel_picker.h                                                 */
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

#include "scene/gui/dialogs.h"

class ItemList;
class LineEdit;

// Every kind of panel there is, laid out to be looked through.
//
// A pane's header offers the few things used all the time. This is for the
// rest: a list with icons and a filter, so that something used once a month
// can be found by typing two letters rather than by remembering which menu it
// is under. One is shared by every pane; whichever asked is told the answer.
class EditorPanelPicker : public ConfirmationDialog {
	GDCLASS(EditorPanelPicker, ConfirmationDialog);

	LineEdit *filter = nullptr;
	ItemList *list = nullptr;
	Callable chosen;

	void _update_list();
	void _filter_changed(const String &p_text);
	void _item_activated(int p_index);
	void _filter_input(const Ref<InputEvent> &p_event);

protected:
	void _notification(int p_what);
	virtual void ok_pressed() override;

public:
	// Opens it, and calls back with the type id picked, if one is.
	void pick(const Callable &p_chosen);

	static EditorPanelPicker *get_shared();

	EditorPanelPicker();
};

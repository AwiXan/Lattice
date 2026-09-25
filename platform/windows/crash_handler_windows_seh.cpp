/**************************************************************************/
/*  crash_handler_windows_seh.cpp                                         */
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

#include "crash_handler_windows.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/object/script_language.h"
#include "core/os/main_loop.h"
#include "core/os/os.h"
#include "core/string/print_string.h"
#include "core/os/thread.h"
#include "core/templates/safe_refcount.h"
#include "core/version.h"

#ifdef CRASH_HANDLER_EXCEPTION

// Backtrace code based on: https://stackoverflow.com/questions/6205981/windows-c-stack-trace-from-a-running-app

#include <psapi.h>

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <string>
#include <vector>

// Some versions of imagehlp.dll lack the proper packing directives themselves
// so we need to do it.
#pragma pack(push, before_imagehlp, 8)
#include <imagehlp.h>
#pragma pack(pop, before_imagehlp)

// Whether dbghelp has its symbols already: it may be initialized once only.
static bool stall_symbols_ready = false;

struct module_data {
	std::string image_name;
	std::string module_name;
	void *base_address = nullptr;
	DWORD load_size;
};

class symbol {
	typedef IMAGEHLP_SYMBOL64 sym_type;
	sym_type *sym;
	static const int max_name_len = 1024;

public:
	symbol(HANDLE process, DWORD64 address) :
			sym((sym_type *)::operator new(sizeof(*sym) + max_name_len)) {
		memset(sym, '\0', sizeof(*sym) + max_name_len);
		sym->SizeOfStruct = sizeof(*sym);
		sym->MaxNameLength = max_name_len;
		DWORD64 displacement;

		SymGetSymFromAddr64(process, address, &displacement, sym);
	}

	std::string name() { return std::string(sym->Name); }
	std::string undecorated_name() {
		if (*sym->Name == '\0') {
			return "<couldn't map PC to fn name>";
		}
		std::vector<char> und_name(max_name_len);
		UnDecorateSymbolName(sym->Name, &und_name[0], max_name_len, UNDNAME_COMPLETE);
		return std::string(&und_name[0], strlen(&und_name[0]));
	}
};

class get_mod_info {
	HANDLE process;

public:
	get_mod_info(HANDLE h) :
			process(h) {}

	module_data operator()(HMODULE module) {
		module_data ret;
		char temp[4096];
		MODULEINFO mi;

		GetModuleInformation(process, module, &mi, sizeof(mi));
		ret.base_address = mi.lpBaseOfDll;
		ret.load_size = mi.SizeOfImage;

		GetModuleFileNameEx(process, module, temp, sizeof(temp));
		ret.image_name = temp;
		GetModuleBaseName(process, module, temp, sizeof(temp));
		ret.module_name = temp;
		SymLoadModule64(process, nullptr, ret.image_name.c_str(), ret.module_name.c_str(), (DWORD64)ret.base_address, ret.load_size);
		return ret;
	}
};

DWORD CrashHandlerException(EXCEPTION_POINTERS *ep) {
	HANDLE process = GetCurrentProcess();
	HANDLE hThread = GetCurrentThread();
	DWORD offset_from_symbol = 0;
	IMAGEHLP_LINE64 line = {};
	std::vector<module_data> modules;
	DWORD cbNeeded;
	std::vector<HMODULE> module_handles(1);

	if (OS::get_singleton() == nullptr || OS::get_singleton()->is_disable_crash_handler() || IsDebuggerPresent()) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (OS::get_singleton()->is_crash_handler_silent()) {
		std::_Exit(0);
	}

	String msg;
	if (ProjectSettings::get_singleton()) {
		msg = GLOBAL_GET("debug/settings/crash_handler/message");
	}

	// Tell MainLoop about the crash. This can be handled by users too in Node.
	if (OS::get_singleton()->get_main_loop()) {
		OS::get_singleton()->get_main_loop()->notification(MainLoop::NOTIFICATION_CRASH);
	}

	print_error("\n================================================================");
	print_error(vformat("%s: Program crashed", __FUNCTION__));

	// Print the engine version just before, so that people are reminded to include the version in backtrace reports.
	if (String(GODOT_VERSION_HASH).is_empty()) {
		print_error(vformat("Engine version: %s", GODOT_VERSION_FULL_NAME));
	} else {
		print_error(vformat("Engine version: %s (%s)", GODOT_VERSION_FULL_NAME, GODOT_VERSION_HASH));
	}
	print_error(vformat("Dumping the backtrace. %s", msg));

	// Load the symbols - unless the stall watchdog has already.
	if (!stall_symbols_ready && !SymInitialize(process, nullptr, false)) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_EXACT_SYMBOLS);
	EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
	module_handles.resize(cbNeeded / sizeof(HMODULE));
	EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &cbNeeded);
	std::transform(module_handles.begin(), module_handles.end(), std::back_inserter(modules), get_mod_info(process));
	void *base = modules[0].base_address;

	print_error(vformat("Load address: %x\n", (uint64_t)base));

	// Setup stuff:
	CONTEXT *context = ep->ContextRecord;
	STACKFRAME64 frame;
	bool skip_first = false;

	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Mode = AddrModeFlat;

#if defined(_M_X64)
	frame.AddrPC.Offset = context->Rip;
	frame.AddrStack.Offset = context->Rsp;
	frame.AddrFrame.Offset = context->Rbp;
#elif defined(_M_ARM64) || defined(_M_ARM64EC)
	frame.AddrPC.Offset = context->Pc;
	frame.AddrStack.Offset = context->Sp;
	frame.AddrFrame.Offset = context->Fp;
#elif defined(_M_ARM)
	frame.AddrPC.Offset = context->Pc;
	frame.AddrStack.Offset = context->Sp;
	frame.AddrFrame.Offset = context->R11;
#else
	frame.AddrPC.Offset = context->Eip;
	frame.AddrStack.Offset = context->Esp;
	frame.AddrFrame.Offset = context->Ebp;

	// Skip the first one to avoid a duplicate on 32-bit mode
	skip_first = true;
#endif

	line.SizeOfStruct = sizeof(line);
	IMAGE_NT_HEADERS *h = ImageNtHeader(base);
	DWORD image_type = h->FileHeader.Machine;

	int n = 0;
	do {
		if (skip_first) {
			skip_first = false;
		} else {
			if (frame.AddrPC.Offset != 0) {
				std::string fnName = symbol(process, frame.AddrPC.Offset).undecorated_name();

				IMAGEHLP_MODULE64 mod_info;
				memset(&mod_info, 0, sizeof(IMAGEHLP_MODULE64));
				mod_info.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
				uint64_t offset = (uint64_t)base;
				String mod_name = "main";
				if (SymGetModuleInfo64(process, frame.AddrPC.Offset, &mod_info)) {
					offset = mod_info.BaseOfImage;
					if (offset != (uint64_t)base) {
						if (mod_info.ImageName[0] != 0) {
							mod_name = String((const char *)mod_info.ImageName).to_lower().get_file();
						} else if (mod_info.ModuleName[0] != 0) {
							mod_name = String((const char *)mod_info.ModuleName).to_lower();
						} else {
							mod_name = "<unknown module>";
						}
					}
				}
				if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &offset_from_symbol, &line)) {
					print_error(vformat("[%d] %x (%s+%x) - %s (%s:%d)", n, (uint64_t)frame.AddrPC.Offset, mod_name, (uint64_t)frame.AddrPC.Offset - offset, fnName.c_str(), (char *)line.FileName, (int)line.LineNumber));
				} else if (!fnName.empty()) {
					print_error(vformat("[%d] %x (%s+%x) - %s", n, (uint64_t)frame.AddrPC.Offset, mod_name, (uint64_t)frame.AddrPC.Offset - offset, fnName.c_str()));
				} else {
					print_error(vformat("[%d] %x (%s+%x) - ???", n, (uint64_t)frame.AddrPC.Offset, mod_name, (uint64_t)frame.AddrPC.Offset - offset));
				}
			}

			n++;
		}

		if (!StackWalk64(image_type, process, hThread, &frame, context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr)) {
			break;
		}
	} while (frame.AddrReturn.Offset != 0 && n < 256);

	print_error("-- END OF C++ BACKTRACE --");
	print_error("================================================================");

	SymCleanup(process);

	for (const Ref<ScriptBacktrace> &backtrace : ScriptServer::capture_script_backtraces(false)) {
		if (!backtrace->is_empty()) {
			print_error(backtrace->format());
			print_error(vformat("-- END OF %s BACKTRACE --", backtrace->get_language_name().to_upper()));
			print_error("================================================================");
		}
	}

	// Pass the exception to the OS
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

#ifdef CRASH_HANDLER_EXCEPTION
// The editor's stall watchdog. A frozen editor leaves nothing behind: killed,
// its log ends wherever it was, and it cannot be asked. So a thread watches
// the main loop go round, and when it has not for STALL_FIRST_MSEC - frozen,
// or busy for longer than anything should be - it writes where the main
// thread is into the log, and again at STALL_AGAIN_MSEC.
//
// The stack is taken the one way that cannot wait on the thread it is taken
// from: suspended, unwound into an array with the unwinder of the OS, which
// takes no lock the main thread could be holding, and let go - and only then
// named, which allocates and prints.

static constexpr uint64_t STALL_FIRST_MSEC = 5000;
static constexpr uint64_t STALL_AGAIN_MSEC = 30000;
static constexpr int STALL_MAX_FRAMES = 64;

static SafeFlag stall_watchdog_running;
static Thread stall_watchdog_thread;
static HANDLE stall_main_thread = nullptr;

static int _capture_stalled_stack(HANDLE p_thread, DWORD64 *r_frames) {
#if defined(_M_X64)
	if (SuspendThread(p_thread) == (DWORD)-1) {
		return 0;
	}
	int count = 0;
	CONTEXT context;
	memset(&context, 0, sizeof(context));
	context.ContextFlags = CONTEXT_FULL;
	if (GetThreadContext(p_thread, &context)) {
		while (count < STALL_MAX_FRAMES && context.Rip != 0) {
			r_frames[count++] = context.Rip;
			DWORD64 image_base = 0;
			PRUNTIME_FUNCTION function = RtlLookupFunctionEntry(context.Rip, &image_base, nullptr);
			if (!function) {
				// A leaf function: the return address is on top of the stack.
				context.Rip = *(DWORD64 *)context.Rsp;
				context.Rsp += 8;
			} else {
				PVOID handler_data = nullptr;
				DWORD64 establisher_frame = 0;
				RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, context.Rip, function, &context, &handler_data, &establisher_frame, nullptr);
			}
		}
	}
	ResumeThread(p_thread);
	return count;
#else
	return 0;
#endif
}

// Two stalls are nobody's fault, and say so rather than alarm whoever reads
// the log: Windows keeping the main thread in its own handling of a window,
// and shaders compiling.
static const char *_stall_likely_cause(HANDLE p_process, const DWORD64 *p_frames, const std::vector<std::string> &p_names) {
	MODULEINFO engine;
	if (!GetModuleInformation(p_process, GetModuleHandle(nullptr), &engine, sizeof(engine))) {
		return nullptr;
	}
	const DWORD64 engine_begin = (DWORD64)engine.lpBaseOfDll;
	const DWORD64 engine_end = engine_begin + engine.SizeOfImage;
	const int count = (int)p_names.size();

	// The innermost frame of the editor's own: when that is where it hands a
	// window's message to Windows, Windows has kept it since - a button held on
	// a title bar, a system menu open, a move or resize not yet begun (once
	// begun, the editor draws on through a timer).
	for (int i = 0; i < count; i++) {
		if (p_frames[i] >= engine_begin && p_frames[i] < engine_end) {
			if (i > 0 && p_names[i].find("WndProc") != std::string::npos) {
				return "Lattice: this is Windows handling an editor window, not the editor: a mouse button held on the title bar or its buttons, the window's menu open, or the like. The editor goes on when that ends.";
			}
			if (p_names[i].find("ShaderGLES3::") != std::string::npos) {
				return "Lattice: the graphics driver is compiling shaders. Slow the first time after the engine or its shaders change; cached for the next time.";
			}
			break;
		}
	}

	// Waiting on the worker threads compiling shaders or pipelines, to draw.
	for (int i = 0; i < count; i++) {
		if (p_names[i].find("wait_for_task_completion") == std::string::npos && p_names[i].find("wait_for_group_task_completion") == std::string::npos) {
			continue;
		}
		for (int j = i + 1; j < MIN(count, i + 4); j++) {
			const std::string &name = p_names[j];
			if (name.find("ShaderRD::") != std::string::npos || name.find("get_pipeline") != std::string::npos || name.find("_render_list_template") != std::string::npos || name.find("_render_batch") != std::string::npos) {
				return "Lattice: the editor is waiting for shaders to compile. Slow the first time after the engine or its shaders change; cached for the next time.";
			}
		}
		break;
	}
	return nullptr;
}

static void _print_stalled_stack(uint64_t p_msec) {
	DWORD64 frames[STALL_MAX_FRAMES];
	const int count = _capture_stalled_stack(stall_main_thread, frames);

	HANDLE process = GetCurrentProcess();
	if (!stall_symbols_ready) {
		if (!SymInitialize(process, nullptr, false)) {
			return;
		}
		SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_EXACT_SYMBOLS);
		DWORD needed = 0;
		std::vector<HMODULE> module_handles(1);
		EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &needed);
		module_handles.resize(needed / sizeof(HMODULE));
		EnumProcessModules(process, &module_handles[0], module_handles.size() * sizeof(HMODULE), &needed);
		std::vector<module_data> modules;
		std::transform(module_handles.begin(), module_handles.end(), std::back_inserter(modules), get_mod_info(process));
		stall_symbols_ready = true;
	}

	std::vector<std::string> names(count);
	for (int i = 0; i < count; i++) {
		names[i] = symbol(process, frames[i]).undecorated_name();
		if (names[i].front() == '<') {
			// Somewhere with no symbols: in which module, at least.
			IMAGEHLP_MODULE64 module_info;
			memset(&module_info, 0, sizeof(module_info));
			module_info.SizeOfStruct = sizeof(module_info);
			if (SymGetModuleInfo64(process, frames[i], &module_info)) {
				names[i] = std::string(module_info.ModuleName) + "+" + std::to_string(frames[i] - module_info.BaseOfImage);
			}
		}
	}

	print_error("\n================================================================");
	print_error(vformat("Lattice: the editor has not gone round its main loop for %d s. Its main thread is here:", int(p_msec / 1000)));
	const char *likely = _stall_likely_cause(process, frames, names);
	if (likely) {
		print_error(likely);
	}
	IMAGEHLP_LINE64 line;
	memset(&line, 0, sizeof(line));
	line.SizeOfStruct = sizeof(line);
	for (int i = 0; i < count; i++) {
		DWORD offset_from_symbol = 0;
		if (SymGetLineFromAddr64(process, frames[i], &offset_from_symbol, &line)) {
			print_error(vformat("[%d] %s (%s:%d)", i, names[i].c_str(), String((const char *)line.FileName).get_file(), (int)line.LineNumber));
		} else {
			print_error(vformat("[%d] %s", i, names[i].c_str()));
		}
	}
	print_error("-- END OF THE STALLED MAIN THREAD --");
	print_error("================================================================");
}

static void _stall_watchdog(void *p_userdata) {
	uint64_t last_frame = Engine::get_singleton()->get_process_frames();
	uint64_t since = OS::get_singleton()->get_ticks_msec();
	int reported = 0;
	while (stall_watchdog_running.is_set()) {
		OS::get_singleton()->delay_usec(250000);
		const uint64_t frame = Engine::get_singleton()->get_process_frames();
		const uint64_t now = OS::get_singleton()->get_ticks_msec();
		if (frame != last_frame) {
			last_frame = frame;
			since = now;
			reported = 0;
			continue;
		}
		const uint64_t stalled = now - since;
		if ((reported == 0 && stalled >= STALL_FIRST_MSEC) || (reported == 1 && stalled >= STALL_AGAIN_MSEC)) {
			reported++;
			_print_stalled_stack(stalled);
		}
	}
}

void CrashHandler::start_stall_watchdog() {
	if (stall_watchdog_running.is_set()) {
		return;
	}
	// Called on the main thread, which is the one watched.
	stall_main_thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, GetCurrentThreadId());
	if (!stall_main_thread) {
		return;
	}
	stall_watchdog_running.set();
	stall_watchdog_thread.start(_stall_watchdog, nullptr);
}

void CrashHandler::stop_stall_watchdog() {
	if (!stall_watchdog_running.is_set()) {
		return;
	}
	stall_watchdog_running.clear();
	stall_watchdog_thread.wait_to_finish();
	CloseHandle(stall_main_thread);
	stall_main_thread = nullptr;
}
#else
void CrashHandler::start_stall_watchdog() {
}

void CrashHandler::stop_stall_watchdog() {
}
#endif

CrashHandler::CrashHandler() {
	disabled = false;
}

CrashHandler::~CrashHandler() {
}

void CrashHandler::disable() {
	if (disabled) {
		return;
	}

	disabled = true;
}

void CrashHandler::initialize() {
}

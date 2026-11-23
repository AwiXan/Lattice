#![allow(dead_code)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

static NET_ID_CALLBACK: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

pub fn set_net_id_callback(cb: fn(u32, u64)) {
    NET_ID_CALLBACK.store(cb as *mut (), Ordering::SeqCst);
}

#[inline]
pub fn trigger_net_id_callback(net_id: u32, instance_id: u64) {
    let ptr = NET_ID_CALLBACK.load(Ordering::SeqCst);
    if !ptr.is_null() {
        let cb: fn(u32, u64) = unsafe { std::mem::transmute(ptr) };
        cb(net_id, instance_id);
    }
}

// GDExtension C-ABI Native Registration Bridge for Godot Engine

use std::ffi::{c_char, c_void};
use std::collections::HashMap;
use std::sync::atomic::{AtomicPtr, AtomicBool, Ordering};
pub use std::sync::{LazyLock, Mutex, Condvar, RwLock};
use std::cell::UnsafeCell;


pub type GDExtensionBool = u8;
pub type GDExtensionInt = i64;
pub type GDExtensionObjectPtr = *mut c_void;
pub type GDExtensionClassInstancePtr = *mut c_void;
pub type GDExtensionClassLibraryPtr = *mut c_void;
pub type GDExtensionConstStringNamePtr = *const c_void;
pub type GDExtensionUninitializedStringNamePtr = *mut c_void;
pub type GDExtensionConstStringPtr = *const c_void;
pub type GDExtensionConstVariantPtr = *const c_void;
pub type GDExtensionVariantPtr = *mut c_void;
pub type GDExtensionTypePtr = *mut c_void;
pub type GDExtensionConstTypePtr = *const c_void;

pub const GDEXTENSION_VARIANT_TYPE_NIL: u32 = 0;
pub const GDEXTENSION_VARIANT_TYPE_BOOL: u32 = 1;
pub const GDEXTENSION_VARIANT_TYPE_INT: u32 = 2;
pub const GDEXTENSION_VARIANT_TYPE_FLOAT: u32 = 3;
pub const GDEXTENSION_VARIANT_TYPE_STRING: u32 = 4;
pub const GDEXTENSION_VARIANT_TYPE_VECTOR2: u32 = 5;
pub const GDEXTENSION_VARIANT_TYPE_VECTOR3: u32 = 9;
pub const GDEXTENSION_VARIANT_TYPE_BASIS: u32 = 17;
pub const GDEXTENSION_VARIANT_TYPE_TRANSFORM3D: u32 = 18;
pub const GDEXTENSION_VARIANT_TYPE_STRING_NAME: u32 = 21;
pub const GDEXTENSION_VARIANT_TYPE_NODE_PATH: u32 = 22;
pub const GDEXTENSION_VARIANT_TYPE_OBJECT: u32 = 24;
pub const GDEXTENSION_VARIANT_TYPE_DICTIONARY: u32 = 27;
pub const GDEXTENSION_VARIANT_TYPE_ARRAY: u32 = 28;
pub const GDEXTENSION_VARIANT_TYPE_PACKED_STRING_ARRAY: u32 = 34;

pub const GDEXTENSION_METHOD_FLAG_NORMAL: u32 = 1;
pub const GDEXTENSION_METHOD_FLAG_VIRTUAL: u32 = 8;
pub const GDEXTENSION_METHOD_FLAG_STATIC: u32 = 32;
pub const GDEXTENSION_CALL_OK: u32 = 0;

pub const PROPERTY_HINT_NONE: u32 = 0;
pub const PROPERTY_HINT_RANGE: u32 = 1;
pub const PROPERTY_HINT_ENUM: u32 = 2;
pub const PROPERTY_HINT_ENUM_SUGGESTION: u32 = 3;
pub const PROPERTY_HINT_EXP_EASING: u32 = 4;
pub const PROPERTY_HINT_LINK: u32 = 5;
pub const PROPERTY_HINT_FLAGS: u32 = 6;
pub const PROPERTY_HINT_LAYERS_2D_RENDER: u32 = 7;
pub const PROPERTY_HINT_LAYERS_2D_PHYSICS: u32 = 8;
pub const PROPERTY_HINT_LAYERS_2D_NAVIGATION: u32 = 9;
pub const PROPERTY_HINT_LAYERS_3D_RENDER: u32 = 10;
pub const PROPERTY_HINT_LAYERS_3D_PHYSICS: u32 = 11;
pub const PROPERTY_HINT_LAYERS_3D_NAVIGATION: u32 = 12;
pub const PROPERTY_HINT_FILE: u32 = 13;
pub const PROPERTY_HINT_DIR: u32 = 14;
pub const PROPERTY_HINT_GLOBAL_FILE: u32 = 15;
pub const PROPERTY_HINT_GLOBAL_DIR: u32 = 16;
pub const PROPERTY_HINT_RESOURCE_TYPE: u32 = 17;
pub const PROPERTY_HINT_MULTILINE_TEXT: u32 = 18;
pub const PROPERTY_HINT_EXPRESSION: u32 = 19;
pub const PROPERTY_HINT_PLACEHOLDER_TEXT: u32 = 20;
pub const PROPERTY_HINT_COLOR_NO_ALPHA: u32 = 21;
pub const PROPERTY_HINT_OBJECT_ID: u32 = 22;
pub const PROPERTY_HINT_TYPE_STRING: u32 = 23;
pub const PROPERTY_HINT_NODE_PATH_TO_EDITED_NODE: u32 = 24;
pub const PROPERTY_HINT_OBJECT_TOO_BIG: u32 = 25;
pub const PROPERTY_HINT_NODE_PATH_VALID_TYPES: u32 = 26;
pub const PROPERTY_HINT_SAVE_FILE: u32 = 27;
pub const PROPERTY_HINT_GLOBAL_SAVE_FILE: u32 = 28;
pub const PROPERTY_HINT_INT_IS_OBJECT_ID: u32 = 29;
pub const PROPERTY_HINT_INT_IS_POINTER: u32 = 30;
pub const PROPERTY_HINT_ARRAY_TYPE: u32 = 31;
pub const PROPERTY_HINT_LOCALE_ID: u32 = 32;
pub const PROPERTY_HINT_LOCALIZABLE_STRING: u32 = 33;
pub const PROPERTY_HINT_NODE_TYPE: u32 = 34;
pub const PROPERTY_HINT_HIDE_QUATERNION_EDIT: u32 = 35;
pub const PROPERTY_HINT_PASSWORD: u32 = 36;
pub const PROPERTY_HINT_LAYERS_AVOIDANCE: u32 = 37;
pub const PROPERTY_HINT_DICTIONARY_TYPE: u32 = 38;
pub const PROPERTY_HINT_TOOL_BUTTON: u32 = 39;
pub const PROPERTY_HINT_ONESHOT: u32 = 40;

pub const PROPERTY_USAGE_NONE: u32 = 0;
pub const PROPERTY_USAGE_STORAGE: u32 = 2;
pub const PROPERTY_USAGE_EDITOR: u32 = 4;
pub const PROPERTY_USAGE_INTERNAL: u32 = 8;
pub const PROPERTY_USAGE_GROUP: u32 = 64;
pub const PROPERTY_USAGE_CATEGORY: u32 = 128;
pub const PROPERTY_USAGE_SUBGROUP: u32 = 256;
pub const PROPERTY_USAGE_SCRIPT_VARIABLE: u32 = 4096;
pub const PROPERTY_USAGE_READ_ONLY: u32 = 268435456;
pub const PROPERTY_USAGE_DEFAULT: u32 = 6;

static EMPTY_STRING_NAME: usize = 0;
static EMPTY_STRING: usize = 0;

#[repr(C)]
#[derive(Copy, Clone, PartialEq, Eq)]
pub enum GDExtensionInitializationLevel {
    Core = 0,
    Servers = 1,
    Scene = 2,
    Editor = 3,
}

#[repr(C)]
pub struct GDExtensionInitialization {
    pub minimum_initialization_level: GDExtensionInitializationLevel,
    pub userdata: *mut c_void,
    pub initialize: Option<unsafe extern "C" fn(userdata: *mut c_void, p_level: GDExtensionInitializationLevel)>,
    pub deinitialize: Option<unsafe extern "C" fn(userdata: *mut c_void, p_level: GDExtensionInitializationLevel)>,
}

pub type GDExtensionInterfaceGetProcAddress = Option<unsafe extern "C" fn(p_function_name: *const c_char) -> *mut c_void>;

#[repr(C)]
pub struct GDExtensionClassCreationInfo6 {
    pub is_virtual: GDExtensionBool,
    pub is_abstract: GDExtensionBool,
    pub is_exposed: GDExtensionBool,
    pub is_runtime: GDExtensionBool,
    pub icon_path: GDExtensionConstStringPtr,
    pub set_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, name: GDExtensionConstStringNamePtr, value: GDExtensionConstVariantPtr) -> GDExtensionBool>,
    pub get_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, name: GDExtensionConstStringNamePtr, ret: GDExtensionVariantPtr) -> GDExtensionBool>,
    pub get_property_list_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, count: *mut u32) -> *const c_void>,
    pub free_property_list_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, list: *const c_void, count: u32)>,
    pub property_can_revert_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, name: GDExtensionConstStringNamePtr) -> GDExtensionBool>,
    pub property_get_revert_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, name: GDExtensionConstStringNamePtr, ret: GDExtensionVariantPtr) -> GDExtensionBool>,
    pub validate_property_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, prop: *mut c_void) -> GDExtensionBool>,
    pub notification_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, what: i32, reversed: GDExtensionBool)>,
    pub to_string_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, is_valid: *mut GDExtensionBool, out: *mut c_void)>,
    pub reference_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr)>,
    pub unreference_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr)>,
    pub create_instance_func: Option<unsafe extern "C" fn(class_userdata: *mut c_void, notify_postinitialize: GDExtensionBool) -> GDExtensionObjectPtr>,
    pub free_instance_func: Option<unsafe extern "C" fn(class_userdata: *mut c_void, instance: GDExtensionClassInstancePtr)>,
    pub recreate_instance_func: Option<unsafe extern "C" fn(class_userdata: *mut c_void, object: GDExtensionObjectPtr) -> GDExtensionClassInstancePtr>,
    pub get_virtual_func: Option<unsafe extern "C" fn(class_userdata: *mut c_void, name: GDExtensionConstStringNamePtr, hash: u32) -> *mut c_void>,
    pub get_virtual_call_data_func: Option<unsafe extern "C" fn(class_userdata: *mut c_void, name: GDExtensionConstStringNamePtr, hash: u32) -> *mut c_void>,
    pub call_virtual_with_data_func: Option<unsafe extern "C" fn(instance: GDExtensionClassInstancePtr, name: GDExtensionConstStringNamePtr, data: *mut c_void, args: *const GDExtensionConstVariantPtr, ret: GDExtensionVariantPtr)>,
    pub class_userdata: *mut c_void,
}

#[repr(C)]
pub struct GDExtensionPropertyInfo {
    pub r#type: u32,
    pub name: GDExtensionStringNamePtr,
    pub class_name: GDExtensionStringNamePtr,
    pub hint: u32,
    pub hint_string: GDExtensionStringPtr,
    pub usage: u32,
}

pub type GDExtensionStringNamePtr = *mut c_void;
pub type GDExtensionStringPtr = *mut c_void;

#[repr(C)]
pub struct GDExtensionCallError {
    pub error: u32,
    pub argument: i32,
    pub expected: i32,
}

pub type GDExtensionClassMethodCall = unsafe extern "C" fn(
    method_userdata: *mut c_void,
    p_instance: GDExtensionClassInstancePtr,
    p_args: *const GDExtensionConstVariantPtr,
    p_argument_count: GDExtensionInt,
    r_return: GDExtensionVariantPtr,
    r_error: *mut GDExtensionCallError,
);

pub type GDExtensionClassMethodPtrCall = unsafe extern "C" fn(
    method_userdata: *mut c_void,
    p_instance: GDExtensionClassInstancePtr,
    p_args: *const *const c_void,
    r_ret: *mut c_void,
);

#[repr(C)]
pub struct GDExtensionClassMethodInfo {
    pub name: GDExtensionStringNamePtr,
    pub method_userdata: *mut c_void,
    pub call_func: Option<GDExtensionClassMethodCall>,
    pub ptrcall_func: Option<GDExtensionClassMethodPtrCall>,
    pub method_flags: u32,
    pub has_return_value: GDExtensionBool,
    pub return_value_info: *mut GDExtensionPropertyInfo,
    pub return_value_metadata: u32,
    pub argument_count: u32,
    pub arguments_info: *mut GDExtensionPropertyInfo,
    pub arguments_metadata: *mut u32,
    pub default_argument_count: u32,
    pub default_arguments: *mut GDExtensionVariantPtr,
}

// Function pointer signatures from Godot
type FnStringNameNewUtf8 = unsafe extern "C" fn(r_dest: *mut c_void, p_contents: *const c_char);
type FnClassdbRegisterExtensionClass6 = unsafe extern "C" fn(
    p_library: GDExtensionClassLibraryPtr,
    p_class_name: GDExtensionConstStringNamePtr,
    p_parent_class_name: GDExtensionConstStringNamePtr,
    p_extension_funcs: *const GDExtensionClassCreationInfo6,
);
type FnClassdbRegisterExtensionClassMethod = unsafe extern "C" fn(
    p_library: GDExtensionClassLibraryPtr,
    p_class_name: GDExtensionConstStringNamePtr,
    p_method_info: *const GDExtensionClassMethodInfo,
);
type FnClassdbRegisterExtensionClassProperty = unsafe extern "C" fn(
    p_library: GDExtensionClassLibraryPtr,
    p_class_name: GDExtensionConstStringNamePtr,
    p_info: *const GDExtensionPropertyInfo,
    p_setter: GDExtensionConstStringNamePtr,
    p_getter: GDExtensionConstStringNamePtr,
);
type FnClassdbRegisterExtensionClassSignal = unsafe extern "C" fn(
    p_library: GDExtensionClassLibraryPtr,
    p_class_name: GDExtensionConstStringNamePtr,
    p_signal_name: GDExtensionConstStringNamePtr,
    p_argument_info: *const GDExtensionPropertyInfo,
    p_argument_count: GDExtensionInt,
);
pub type FnClassdbConstructObject = unsafe extern "C" fn(p_classname: GDExtensionConstStringNamePtr) -> GDExtensionObjectPtr;
pub type FnObjectSetInstance = unsafe extern "C" fn(
    p_godot_object: GDExtensionObjectPtr,
    p_classname: GDExtensionConstStringNamePtr,
    p_instance: GDExtensionClassInstancePtr,
);
pub type FnVariantNewCopy = unsafe extern "C" fn(r_dest: *mut c_void, p_src: *const c_void);
pub type FnVariantDestroy = unsafe extern "C" fn(p_self: *mut c_void);
pub type FnVariantNewNil = unsafe extern "C" fn(r_dest: *mut c_void);
pub type FnVariantConstruct = unsafe extern "C" fn(
    p_type: u32,
    r_base: *mut c_void,
    p_args: *const *const c_void,
    p_argument_count: i32,
    r_error: *mut GDExtensionCallError,
);
pub type FnGetVariantFromTypeConstructor = unsafe extern "C" fn(p_type: u32) -> Option<unsafe extern "C" fn(r_dest: *mut c_void, p_src: *const c_void)>;
pub type FnGetVariantToTypeConstructor = unsafe extern "C" fn(p_type: u32) -> Option<unsafe extern "C" fn(r_dest: *mut c_void, p_src: *mut c_void)>;
pub type FnObjectGetInstanceId = unsafe extern "C" fn(p_object: *const c_void) -> u64;
pub type FnObjectGetInstanceFromId = unsafe extern "C" fn(p_instance_id: u64) -> *mut c_void;
pub type FnClassdbRegisterExtensionClassIntegerConstant = unsafe extern "C" fn(
    p_library: GDExtensionClassLibraryPtr,
    p_class_name: GDExtensionConstStringNamePtr,
    p_enum_name: GDExtensionConstStringNamePtr,
    p_constant_name: GDExtensionConstStringNamePtr,
    p_constant_value: GDExtensionInt,
    p_is_bitfield: GDExtensionBool,
);
pub type FnClassdbRegisterExtensionClassPropertyGroup = unsafe extern "C" fn(
    p_library: GDExtensionClassLibraryPtr,
    p_class_name: GDExtensionConstStringNamePtr,
    p_group_name: GDExtensionConstStringPtr,
    p_prefix: GDExtensionConstStringPtr,
);
pub type FnClassdbRegisterExtensionClassPropertySubgroup = unsafe extern "C" fn(
    p_library: GDExtensionClassLibraryPtr,
    p_class_name: GDExtensionConstStringNamePtr,
    p_subgroup_name: GDExtensionConstStringPtr,
    p_prefix: GDExtensionConstStringPtr,
);
pub type FnStringNewWithUtf8Chars = unsafe extern "C" fn(
    r_dest: *mut c_void,
    p_contents: *const c_char,
);

pub type GDExtensionConstObjectPtr = *const c_void;
pub type GDExtensionUninitializedVariantPtr = *mut c_void;
pub type GDExtensionMethodBindPtr = *mut c_void;

pub type FnObjectHasScriptMethod = unsafe extern "C" fn(
    p_object: GDExtensionConstObjectPtr,
    p_method: GDExtensionConstStringNamePtr,
) -> GDExtensionBool;
pub type FnObjectCallScriptMethod = unsafe extern "C" fn(
    p_object: GDExtensionObjectPtr,
    p_method: GDExtensionConstStringNamePtr,
    p_args: *const GDExtensionConstVariantPtr,
    p_argument_count: GDExtensionInt,
    r_return: GDExtensionUninitializedVariantPtr,
    r_error: *mut GDExtensionCallError,
);
pub type FnClassdbRegisterExtensionClassVirtualMethod = unsafe extern "C" fn(
    library: GDExtensionClassLibraryPtr,
    class_name: GDExtensionConstStringNamePtr,
    method_info: *const GDExtensionClassVirtualMethodInfo,
);
pub type FnClassdbGetMethodBind = unsafe extern "C" fn(
    class_name: GDExtensionConstStringNamePtr,
    method_name: GDExtensionConstStringNamePtr,
    hash: i64,
) -> GDExtensionMethodBindPtr;
pub type FnObjectMethodBindPtrcall = unsafe extern "C" fn(
    method_bind: GDExtensionMethodBindPtr,
    instance: GDExtensionObjectPtr,
    args: *const *const c_void,
    ret: *mut c_void,
);

pub type FnVariantCall = unsafe extern "C" fn(
    p_self: GDExtensionVariantPtr,
    p_method: GDExtensionConstStringNamePtr,
    p_args: *const GDExtensionConstVariantPtr,
    p_argcount: GDExtensionInt,
    r_return: GDExtensionUninitializedVariantPtr,
    r_error: *mut GDExtensionCallError,
);
pub type FnVariantGetNamed = unsafe extern "C" fn(
    p_self: GDExtensionConstVariantPtr,
    p_key: GDExtensionConstStringNamePtr,
    r_ret: GDExtensionUninitializedVariantPtr,
    r_valid: *mut GDExtensionBool,
);
pub type FnVariantStringify = unsafe extern "C" fn(
    p_self: GDExtensionConstVariantPtr,
    r_ret: GDExtensionStringPtr,
);
pub type FnStringToUtf8Chars = unsafe extern "C" fn(
    p_self: GDExtensionConstStringPtr,
    r_text: *mut c_char,
    p_max_write_length: GDExtensionInt,
) -> GDExtensionInt;
pub type FnVariantGetPtrDestructor = unsafe extern "C" fn(
    p_type: u32,
) -> Option<unsafe extern "C" fn(p_base: *mut c_void)>;

#[repr(C)]
pub struct GDExtensionClassVirtualMethodInfo {
    pub name: GDExtensionStringNamePtr,
    pub method_flags: u32,
    pub return_value: GDExtensionPropertyInfo,
    pub return_value_metadata: u32,
    pub argument_count: u32,
    pub arguments: *mut GDExtensionPropertyInfo,
    pub arguments_metadata: *mut u32,
}

pub static GLOBAL_GET_SINGLETON: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub type FnGlobalGetSingleton = unsafe extern "C" fn(p_name: GDExtensionConstStringNamePtr) -> GDExtensionObjectPtr;

pub static GET_PROC_ADDRESS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static STRING_NAME_NEW: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static STRING_NEW_WITH_UTF8_CHARS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_REGISTER_CLASS6: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_UNREGISTER_CLASS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub type FnClassdbUnregisterExtensionClass = unsafe extern "C" fn(GDExtensionClassLibraryPtr, *const c_void);
pub static REGISTERED_CLASSES: LazyLock<Mutex<Vec<usize>>> = LazyLock::new(|| Mutex::new(Vec::new()));
pub static CLASSDB_REGISTER_METHOD: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_REGISTER_PROPERTY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_REGISTER_PROPERTY_GROUP: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_REGISTER_PROPERTY_SUBGROUP: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_REGISTER_SIGNAL: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_REGISTER_INTEGER_CONSTANT: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_CONSTRUCT_OBJECT: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static OBJECT_SET_INSTANCE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_NEW_COPY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_DESTROY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_NEW_NIL: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_CONSTRUCT: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static GET_FROM_TYPE_CTOR: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static GET_TO_TYPE_CTOR: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static OBJECT_GET_ID: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static OBJECT_GET_FROM_ID: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static OBJECT_HAS_SCRIPT_METHOD: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static OBJECT_CALL_SCRIPT_METHOD: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_REGISTER_VIRTUAL_METHOD: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static CLASSDB_GET_METHOD_BIND: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static OBJECT_METHOD_BIND_PTRCALL: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_CALL: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_GET_NAMED: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_GET_TYPE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub type FnVariantGetType = unsafe extern "C" fn(p_self: GDExtensionConstVariantPtr) -> u32;

pub unsafe fn variant_get_type(var: GDExtensionConstVariantPtr) -> u32 {
    let ptr = VARIANT_GET_TYPE.load(Ordering::SeqCst);
    if !ptr.is_null() && !var.is_null() {
        let f: FnVariantGetType = std::mem::transmute(ptr);
        f(var)
    } else {
        GDEXTENSION_VARIANT_TYPE_NIL
    }
}

pub unsafe fn variant_from_string(r_dest: GDExtensionVariantPtr, s: &str) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_STRING) {
            let ms = ManagedString::new(s);
            ctor(r_dest, ms.as_ptr());
            let destr_ptr = STRING_DESTRUCTOR.load(Ordering::SeqCst);
            if !destr_ptr.is_null() {
                let destr_fn: unsafe extern "C" fn(*mut c_void) = std::mem::transmute(destr_ptr);
                let mut raw = ms.0;
                destr_fn(&mut raw as *mut _ as *mut c_void);
            }
            return;
        }
    }
    let s_sn = ManagedStringName::new(s);
    variant_from_string_name(r_dest, s_sn.as_ptr());
}

pub static VARIANT_STRINGIFY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static STRING_TO_UTF8_CHARS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static VARIANT_GET_PTR_DESTRUCTOR: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static STRING_DESTRUCTOR: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

pub static EDITOR_HELP_LOAD_XML: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static EDITOR_HELP_LOAD_XML_AND_LEN: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub type FnEditorHelpLoadXmlFromUtf8Chars = unsafe extern "C" fn(p_data: *const c_char);
pub type FnEditorHelpLoadXmlFromUtf8CharsAndLen = unsafe extern "C" fn(p_data: *const c_char, p_size: i64);

pub static REGISTERED_XML_DOCS: LazyLock<Mutex<Vec<&'static str>>> = LazyLock::new(|| Mutex::new(Vec::new()));

/// Registers a class documentation XML string to be loaded into the Godot Editor help system.
pub fn register_class_doc_xml(xml: &'static str) {
    if let Ok(mut list) = REGISTERED_XML_DOCS.lock() {
        list.push(xml);
    }
    load_xml_documentation(xml);
}

/// Loads all registered XML documentation strings into Godot's EditorHelp.
pub fn load_registered_editor_help() {
    if let Ok(list) = REGISTERED_XML_DOCS.lock() {
        for xml in list.iter() {
            load_xml_documentation(xml);
        }
    }
}

/// Directly feeds an XML documentation buffer into Godot's EditorHelp.
pub fn load_xml_documentation(xml_str: &str) {
    let fn_ptr_len = EDITOR_HELP_LOAD_XML_AND_LEN.load(Ordering::SeqCst);
    if !fn_ptr_len.is_null() {
        let f: FnEditorHelpLoadXmlFromUtf8CharsAndLen = unsafe { std::mem::transmute(fn_ptr_len) };
        unsafe {
            f(xml_str.as_ptr() as *const c_char, xml_str.len() as i64);
        }
        return;
    }
    let fn_ptr = EDITOR_HELP_LOAD_XML.load(Ordering::SeqCst);
    if !fn_ptr.is_null() {
        if let Ok(c_str) = std::ffi::CString::new(xml_str) {
            let f: FnEditorHelpLoadXmlFromUtf8Chars = unsafe { std::mem::transmute(fn_ptr) };
            unsafe {
                f(c_str.as_ptr());
            }
        }
    }
}

pub static METHOD_SET_PROCESS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_PHYSICS_PROCESS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_GET_PROCESS_DELTA: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_GET_PHYSICS_PROCESS_DELTA: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

pub static METHOD_SET_CENTER_OF_MASS_MODE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_GRAVITY_SCALE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_GET_GRAVITY_SCALE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_FREEZE_ENABLED: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_FREEZE_MODE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_LINEAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_GET_LINEAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_ANGULAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_GET_ANGULAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_USE_CUSTOM_INTEGRATOR: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_IS_USING_CUSTOM_INTEGRATOR: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_IS_LOCK_ROTATION_ENABLED: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_LOCK_ROTATION_ENABLED: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_MASS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_GET_MASS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_CENTER_OF_MASS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_GET_CENTER_OF_MASS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_CONTACT_MONITOR: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_SET_MAX_CONTACTS_REPORTED: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

pub static METHOD_NODE3D_GET_GLOBAL_TRANSFORM: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_NODE3D_SET_GLOBAL_TRANSFORM: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_NODE3D_GET_GLOBAL_POSITION: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_NODE3D_SET_GLOBAL_POSITION: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_NODE3D_GET_TRANSFORM: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_NODE3D_SET_TRANSFORM: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_NODE3D_SET_NOTIFY_TRANSFORM: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

pub static METHOD_STATE_GET_CONTACT_COUNT: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_CONTACT_LOCAL_SHAPE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_CONTACT_IMPULSE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_CONTACT_LOCAL_NORMAL: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_CONTACT_LOCAL_POSITION: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_CONTACT_COLLIDER_POSITION: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_CONTACT_COLLIDER_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_CONTACT_COLLIDER_OBJECT: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_LINEAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_SET_LINEAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_ANGULAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_SET_ANGULAR_VELOCITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_TRANSFORM: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_SET_TRANSFORM: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_TOTAL_GRAVITY: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_RIGIDBODY_APPLY_CENTRAL_IMPULSE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_RIGIDBODY_APPLY_TORQUE_IMPULSE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_APPLY_CENTRAL_IMPULSE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_APPLY_CENTRAL_FORCE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_APPLY_TORQUE: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_INVERSE_MASS: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static METHOD_STATE_GET_STEP: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

pub static DICTIONARY_OPERATOR_INDEX: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub type FnDictionaryOperatorIndex = unsafe extern "C" fn(p_self: GDExtensionTypePtr, p_key: GDExtensionConstVariantPtr) -> GDExtensionVariantPtr;

pub struct ManagedStringName(pub usize);
impl ManagedStringName {
    pub fn new(name: &str) -> Self {
        let mut sn: usize = 0;
        let c_str = std::ffi::CString::new(name).unwrap_or_default();
        let sn_ptr = STRING_NAME_NEW.load(Ordering::SeqCst);
        if !sn_ptr.is_null() {
            let sn_fn: FnStringNameNewUtf8 = unsafe { std::mem::transmute(sn_ptr) };
            unsafe {
                sn_fn(&mut sn as *mut _ as *mut c_void, c_str.as_ptr());
            }
        }
        ManagedStringName(sn)
    }
    pub fn as_ptr(&self) -> GDExtensionConstStringNamePtr {
        &self.0 as *const _ as *const c_void
    }
}

pub fn get_cached_string_name(name: &str) -> GDExtensionConstStringNamePtr {
    static STRING_NAME_CACHE: LazyLock<RwLock<HashMap<String, &'static ManagedStringName>>> =
        LazyLock::new(|| RwLock::new(HashMap::new()));

    if let Ok(map) = STRING_NAME_CACHE.read() {
        if let Some(sn) = map.get(name) {
            return sn.as_ptr();
        }
    }

    let mut map = STRING_NAME_CACHE.write().unwrap();
    if let Some(sn) = map.get(name) {
        return sn.as_ptr();
    }
    let boxed = Box::leak(Box::new(ManagedStringName::new(name)));
    map.insert(name.to_string(), boxed);
    boxed.as_ptr()
}

pub struct ManagedString(pub usize);
impl ManagedString {
    pub fn new(s: &str) -> Self {
        let mut str_val: usize = 0;
        let c_str = std::ffi::CString::new(s).unwrap_or_default();
        let s_ptr = STRING_NEW_WITH_UTF8_CHARS.load(Ordering::SeqCst);
        if !s_ptr.is_null() {
            let s_fn: FnStringNewWithUtf8Chars = unsafe { std::mem::transmute(s_ptr) };
            unsafe {
                s_fn(&mut str_val as *mut _ as *mut c_void, c_str.as_ptr());
            }
        }
        ManagedString(str_val)
    }
    pub fn as_ptr(&self) -> GDExtensionConstStringPtr {
        &self.0 as *const _ as *const c_void
    }
}

pub fn get_empty_string_ptr() -> GDExtensionConstStringPtr {
    static EMPTY: LazyLock<ManagedString> = LazyLock::new(|| ManagedString::new(""));
    EMPTY.as_ptr()
}

pub fn get_empty_string_name_ptr() -> GDExtensionConstStringNamePtr {
    static EMPTY_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new(""));
    EMPTY_SN.as_ptr()
}

pub unsafe fn variant_new_nil(r_dest: GDExtensionVariantPtr) {
    let f_ptr = VARIANT_NEW_NIL.load(Ordering::SeqCst);
    if !f_ptr.is_null() {
        let f: FnVariantNewNil = std::mem::transmute(f_ptr);
        f(r_dest);
    }
}

pub unsafe fn variant_copy(r_dest: GDExtensionVariantPtr, p_src: GDExtensionConstVariantPtr) {
    let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
    if !copy_ptr.is_null() {
        let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
        copy_fn(r_dest, p_src);
    }
}

pub unsafe fn variant_from_int(r_dest: GDExtensionVariantPtr, val: i64) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_INT) {
            ctor(r_dest, &val as *const _ as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn variant_from_bool(r_dest: GDExtensionVariantPtr, val: bool) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_BOOL) {
            let b: u8 = if val { 1 } else { 0 };
            ctor(r_dest, &b as *const _ as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn variant_from_object(r_dest: GDExtensionVariantPtr, obj: *mut c_void) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_OBJECT) {
            ctor(r_dest, &obj as *const _ as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn variant_from_dictionary(r_dest: GDExtensionVariantPtr) {
    let construct_ptr = VARIANT_CONSTRUCT.load(Ordering::SeqCst);
    if !construct_ptr.is_null() {
        let construct: FnVariantConstruct = std::mem::transmute(construct_ptr);
        let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
        construct(GDEXTENSION_VARIANT_TYPE_DICTIONARY, r_dest, std::ptr::null(), 0, &mut err);
    } else {
        variant_new_nil(r_dest);
    }
}

pub unsafe fn variant_from_array(r_dest: GDExtensionVariantPtr) {
    let construct_ptr = VARIANT_CONSTRUCT.load(Ordering::SeqCst);
    if !construct_ptr.is_null() {
        let construct: FnVariantConstruct = std::mem::transmute(construct_ptr);
        let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
        construct(GDEXTENSION_VARIANT_TYPE_ARRAY, r_dest, std::ptr::null(), 0, &mut err);
    } else {
        variant_new_nil(r_dest);
    }
}

pub unsafe fn variant_to_int(var: GDExtensionConstVariantPtr) -> i64 {
    let mut val: i64 = 0;
    let get_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_INT) {
            ctor(&mut val as *mut _ as *mut c_void, var as *mut c_void);
        }
    }
    val
}

pub unsafe fn variant_to_object(var: GDExtensionConstVariantPtr) -> *mut c_void {
    let mut obj: *mut c_void = std::ptr::null_mut();
    let get_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_OBJECT) {
            ctor(&mut obj as *mut _ as *mut c_void, var as *mut c_void);
        }
    }
    obj
}

pub unsafe fn variant_to_bool(var: GDExtensionConstVariantPtr) -> bool {
    let mut b: u8 = 0;
    let get_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_BOOL) {
            ctor(&mut b as *mut _ as *mut c_void, var as *mut c_void);
        }
    }
    b != 0
}

pub unsafe fn variant_from_float(r_dest: GDExtensionVariantPtr, val: f64) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_FLOAT) {
            ctor(r_dest, &val as *const _ as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn variant_to_float(var: GDExtensionConstVariantPtr) -> f64 {
    let mut val: f64 = 0.0;
    let get_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_FLOAT) {
            ctor(&mut val as *mut _ as *mut c_void, var as *mut c_void);
        }
    }
    val
}

pub unsafe fn variant_from_vector2(r_dest: GDExtensionVariantPtr, val: [f32; 2]) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_VECTOR2) {
            ctor(r_dest, val.as_ptr() as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn variant_to_vector2(var: GDExtensionConstVariantPtr) -> [f32; 2] {
    let mut val = [0.0f32; 2];
    let get_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_VECTOR2) {
            ctor(val.as_mut_ptr() as *mut c_void, var as *mut c_void);
        }
    }
    val
}

pub unsafe fn variant_from_vector3(r_dest: GDExtensionVariantPtr, val: [f32; 3]) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_VECTOR3) {
            ctor(r_dest, val.as_ptr() as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn variant_to_vector3(var: GDExtensionConstVariantPtr) -> [f32; 3] {
    let mut val = [0.0f32; 3];
    let get_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_VECTOR3) {
            ctor(val.as_mut_ptr() as *mut c_void, var as *mut c_void);
        }
    }
    val
}

pub unsafe fn create_default_variant(vtype: u32) -> GDExtensionVariantPtr {
    let raw = Box::leak(Box::new([0u8; 24]));
    construct_default_variant(raw.as_mut_ptr() as *mut c_void, vtype);
    raw.as_mut_ptr() as GDExtensionVariantPtr
}

pub unsafe fn create_bool_variant(val: bool) -> GDExtensionVariantPtr {
    let raw = Box::leak(Box::new([0u8; 24]));
    variant_from_bool(raw.as_mut_ptr() as *mut c_void, val);
    raw.as_mut_ptr() as GDExtensionVariantPtr
}

pub unsafe fn create_int_variant(val: i64) -> GDExtensionVariantPtr {
    let raw = Box::leak(Box::new([0u8; 24]));
    variant_from_int(raw.as_mut_ptr() as *mut c_void, val);
    raw.as_mut_ptr() as GDExtensionVariantPtr
}

pub unsafe fn create_float_variant(val: f64) -> GDExtensionVariantPtr {
    let raw = Box::leak(Box::new([0u8; 24]));
    variant_from_float(raw.as_mut_ptr() as *mut c_void, val);
    raw.as_mut_ptr() as GDExtensionVariantPtr
}

pub unsafe fn create_array_variant() -> GDExtensionVariantPtr {
    create_default_variant(GDEXTENSION_VARIANT_TYPE_ARRAY)
}

pub unsafe fn create_string_variant(val: &str) -> GDExtensionVariantPtr {
    let raw = Box::leak(Box::new([0u8; 24]));
    variant_from_string(raw.as_mut_ptr() as *mut c_void, &val.to_string());
    raw.as_mut_ptr() as GDExtensionVariantPtr
}

pub unsafe fn create_nil_variant() -> GDExtensionVariantPtr {
    let raw = Box::leak(Box::new([0u8; 24]));
    variant_new_nil(raw.as_mut_ptr() as *mut c_void);
    raw.as_mut_ptr() as GDExtensionVariantPtr
}

pub unsafe fn create_vector3_variant(val: [f32; 3]) -> GDExtensionVariantPtr {
    let raw = Box::leak(Box::new([0u8; 24]));
    variant_from_vector3(raw.as_mut_ptr() as *mut c_void, val);
    raw.as_mut_ptr() as GDExtensionVariantPtr
}

pub unsafe fn construct_default_variant(r_dest: GDExtensionVariantPtr, prop_type: u32) {
    let construct_ptr = VARIANT_CONSTRUCT.load(Ordering::SeqCst);
    if !construct_ptr.is_null() {
        let construct: FnVariantConstruct = std::mem::transmute(construct_ptr);
        let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
        construct(prop_type, r_dest, std::ptr::null(), 0, &mut err);
    } else {
        variant_new_nil(r_dest);
    }
}

pub struct BoundPropInfo {
    pub name: &'static str,
    pub name_sn: usize,
    pub prop_type: u32,
    pub setter_name: &'static str,
    pub getter_name: &'static str,
    pub default_int: i64,
    pub default_float: f64,
}

pub unsafe fn object_get_instance_id(obj: *const c_void) -> u64 {
    let f_ptr = OBJECT_GET_ID.load(Ordering::SeqCst);
    if !f_ptr.is_null() && !obj.is_null() {
        let f: FnObjectGetInstanceId = std::mem::transmute(f_ptr);
        return f(obj);
    }
    0
}

pub unsafe fn object_get_instance_from_id(id: u64) -> *mut c_void {
    let f_ptr = OBJECT_GET_FROM_ID.load(Ordering::SeqCst);
    if !f_ptr.is_null() && id != 0 {
        let f: FnObjectGetInstanceFromId = std::mem::transmute(f_ptr);
        return f(id);
    }
    std::ptr::null_mut()
}

pub struct RustClassDescriptor {
    pub name: &'static str,
    pub parent: &'static str,
    pub base_godot_class: &'static str,
    pub class_sn: usize,
    pub parent_sn: usize,
    pub base_sn: usize,
}

pub struct VariantBuffer {
    pub data: [u8; 48],
}

impl Drop for VariantBuffer {
    fn drop(&mut self) {
        let destroy_ptr = VARIANT_DESTROY.load(Ordering::SeqCst);
        if !destroy_ptr.is_null() {
            let destroy_fn: FnVariantDestroy = unsafe { std::mem::transmute(destroy_ptr) };
            unsafe {
                destroy_fn(self.data.as_mut_ptr() as *mut c_void);
            }
        }
    }
}



#[cfg(windows)]
extern "system" {
    fn GetCurrentThreadId() -> u32;
}

#[inline(always)]
pub fn current_thread_id() -> u64 {
    #[cfg(windows)]
    unsafe {
        GetCurrentThreadId() as u64
    }
    #[cfg(not(windows))]
    {
        use std::sync::atomic::{AtomicU64, Ordering};
        thread_local! {
            static THREAD_ID: u64 = {
                static COUNTER: AtomicU64 = AtomicU64::new(1);
                COUNTER.fetch_add(1, Ordering::Relaxed)
            };
        }
        THREAD_ID.with(|id| *id)
    }
}

pub struct ReentrantMutex<T> {
    state: Mutex<(u64, usize)>,
    condvar: Condvar,
    data: UnsafeCell<T>,
}

unsafe impl<T: Send> Send for ReentrantMutex<T> {}
unsafe impl<T: Send> Sync for ReentrantMutex<T> {}

impl<T> ReentrantMutex<T> {
    pub fn new(val: T) -> Self {
        Self {
            state: Mutex::new((0, 0)),
            condvar: Condvar::new(),
            data: UnsafeCell::new(val),
        }
    }

    pub fn with_lock<R>(&self, f: impl FnOnce(&mut T) -> R) -> R {
        let tid = current_thread_id();
        {
            let mut state = self.state.lock().unwrap_or_else(|e| e.into_inner());
            while state.0 != 0 && state.0 != tid {
                state = self.condvar.wait(state).unwrap_or_else(|e| e.into_inner());
            }
            state.0 = tid;
            state.1 += 1;
        }

        struct ReentrantGuard<'a, T> {
            mutex: &'a ReentrantMutex<T>,
        }

        impl<'a, T> Drop for ReentrantGuard<'a, T> {
            fn drop(&mut self) {
                let mut state = self.mutex.state.lock().unwrap_or_else(|e| e.into_inner());
                state.1 -= 1;
                if state.1 == 0 {
                    state.0 = 0;
                    self.mutex.condvar.notify_one();
                }
            }
        }

        let _guard = ReentrantGuard { mutex: self };
        let ptr = self.data.get();
        f(unsafe { &mut *ptr })
    }

    pub fn with_ref<R>(&self, f: impl FnOnce(&T) -> R) -> R {
        self.with_lock(|val| f(val))
    }
}

pub struct RustNodeInstance {
    pub godot_object: GDExtensionObjectPtr,
    pub class_name: &'static str,
    pub properties: Mutex<HashMap<usize, VariantBuffer>>,
    pub user_data: ReentrantMutex<Option<Box<dyn std::any::Any + Send + Sync>>>,
}

pub static RUST_INSTANCES: LazyLock<Mutex<HashMap<usize, usize>>> = LazyLock::new(|| Mutex::new(HashMap::new()));

pub unsafe fn get_rust_instance(object: GDExtensionObjectPtr) -> Option<*mut RustNodeInstance> {
    if object.is_null() {
        return None;
    }
    let map = RUST_INSTANCES.lock().ok()?;
    let ptr = map.get(&(object as usize))?;
    Some(*ptr as *mut RustNodeInstance)
}

pub unsafe fn resolve_node_path_from_variant(node_obj: GDExtensionObjectPtr, path_var: GDExtensionConstVariantPtr) -> GDExtensionObjectPtr {
    node_get_node_or_null(node_obj, path_var)
}

pub type InstanceFactory = fn(godot_object: GDExtensionObjectPtr, class_name: &'static str) -> Option<Box<dyn std::any::Any + Send + Sync>>;
pub type NotificationHook = fn(instance: &RustNodeInstance, what: i32);
pub type PropertySetHook = fn(instance: &RustNodeInstance, name_key: usize, value: GDExtensionConstVariantPtr) -> bool;
pub type PropertyGetHook = fn(instance: &RustNodeInstance, name_key: usize, ret: GDExtensionVariantPtr) -> bool;

static INSTANCE_FACTORIES: Mutex<Vec<InstanceFactory>> = Mutex::new(Vec::new());
static NOTIFICATION_HOOKS: Mutex<Vec<NotificationHook>> = Mutex::new(Vec::new());
static HAS_NOTIFICATION_HOOKS: AtomicBool = AtomicBool::new(false);
static PROPERTY_SET_HOOKS: Mutex<Vec<PropertySetHook>> = Mutex::new(Vec::new());
static HAS_PROPERTY_SET_HOOKS: AtomicBool = AtomicBool::new(false);
static PROPERTY_GET_HOOKS: Mutex<Vec<PropertyGetHook>> = Mutex::new(Vec::new());
static HAS_PROPERTY_GET_HOOKS: AtomicBool = AtomicBool::new(false);

static NOTIFICATION_CHAINS: LazyLock<RwLock<HashMap<(&'static str, u8), Vec<fn(&RustNodeInstance, i32)>>>> =
    LazyLock::new(|| RwLock::new(HashMap::new()));

pub fn register_instance_factory(factory: InstanceFactory) {
    if let Ok(mut list) = INSTANCE_FACTORIES.lock() {
        list.push(factory);
    }
}

pub fn register_notification_hook(hook: NotificationHook) {
    if let Ok(mut list) = NOTIFICATION_HOOKS.lock() {
        list.push(hook);
        HAS_NOTIFICATION_HOOKS.store(true, Ordering::SeqCst);
    }
}

pub fn register_property_set_hook(hook: PropertySetHook) {
    if let Ok(mut list) = PROPERTY_SET_HOOKS.lock() {
        list.push(hook);
        HAS_PROPERTY_SET_HOOKS.store(true, Ordering::SeqCst);
    }
}

pub fn register_property_get_hook(hook: PropertyGetHook) {
    if let Ok(mut list) = PROPERTY_GET_HOOKS.lock() {
        list.push(hook);
        HAS_PROPERTY_GET_HOOKS.store(true, Ordering::SeqCst);
    }
}

pub type UserDataDowncastHook = fn(&mut (dyn std::any::Any + Send + Sync), std::any::TypeId) -> Option<*mut ()>;
pub type UserDataDowncastRefHook = fn(&(dyn std::any::Any + Send + Sync), std::any::TypeId) -> Option<*const ()>;

pub static USER_DATA_DOWNCAST_HOOK: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());
pub static USER_DATA_DOWNCAST_REF_HOOK: AtomicPtr<()> = AtomicPtr::new(std::ptr::null_mut());

pub fn set_user_data_downcast_hooks(mut_hook: UserDataDowncastHook, ref_hook: UserDataDowncastRefHook) {
    USER_DATA_DOWNCAST_HOOK.store(mut_hook as *mut (), Ordering::SeqCst);
    USER_DATA_DOWNCAST_REF_HOOK.store(ref_hook as *mut (), Ordering::SeqCst);
}

pub const NOTIFICATION_PREDELETE: i32 = 1;
pub const NOTIFICATION_ENTER_TREE: i32 = 10;
pub const NOTIFICATION_EXIT_TREE: i32 = 11;
pub const NOTIFICATION_READY: i32 = 13;
pub const NOTIFICATION_PHYSICS_PROCESS: i32 = 16;
pub const NOTIFICATION_PROCESS: i32 = 17;
pub const NOTIFICATION_TRANSFORM_CHANGED: i32 = 2000;

pub struct ClassRuntimeHandlers {
    pub factory: fn(GDExtensionObjectPtr) -> Box<dyn std::any::Any + Send + Sync>,
    pub on_notification: fn(&RustNodeInstance, i32),
    pub resolve_virtual: fn(&str) -> Option<GDExtensionClassCallVirtual>,
    pub on_property_set: fn(&RustNodeInstance, usize, GDExtensionConstVariantPtr) -> bool,
}

struct ParentEdge {
    parent_id: std::any::TypeId,
    cast_mut: &'static (dyn Fn(*mut ()) -> *mut () + Send + Sync),
    cast_ref: &'static (dyn Fn(*const ()) -> *const () + Send + Sync),
}

static CLASS_HANDLERS: LazyLock<Mutex<HashMap<&'static str, ClassRuntimeHandlers>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));
static CLASS_PARENTS: LazyLock<Mutex<HashMap<&'static str, &'static str>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));
static PARENT_EDGES: LazyLock<Mutex<HashMap<std::any::TypeId, ParentEdge>>> =
    LazyLock::new(|| Mutex::new(HashMap::new()));

/// Trait implemented by every Godot-exposed Rust class.
/// Eliminates all manual factories, notification switches, property hooks, and downcast lists in `lib.rs`.
pub trait GodotRustClass: Default + Send + Sync + 'static {
    const CLASS_NAME: &'static str;
    const DOC_XML: &'static str = "";

    fn register_class(library: GDExtensionClassLibraryPtr);

    /// Automatically chained along Rust inheritance by `rust_class_notification`
    /// (Base -> Derived when `reversed == 0`, Derived -> Base when `reversed != 0`).
    fn on_notification(_instance: &RustNodeInstance, _what: i32) {}

    /// Automatically walked up the Rust inheritance chain by `rust_class_get_virtual`.
    fn resolve_virtual(_method_name: &str) -> Option<GDExtensionClassCallVirtual> {
        None
    }

    /// Optional fallback hook for `bind_prop`-backed properties (walked up the inheritance chain).
    fn on_property_set(
        _instance: &RustNodeInstance,
        _key: usize,
        _value: GDExtensionConstVariantPtr,
    ) -> bool {
        false
    }
}

/// Registers a `GodotRustClass` into runtime handler tables and Godot's ClassDB.
pub unsafe fn register_rust_class<T: GodotRustClass>(library: GDExtensionClassLibraryPtr) {
    if let Ok(mut map) = CLASS_HANDLERS.lock() {
        map.insert(
            T::CLASS_NAME,
            ClassRuntimeHandlers {
                factory: |_obj| Box::new(T::default()),
                on_notification: T::on_notification,
                resolve_virtual: T::resolve_virtual,
                on_property_set: T::on_property_set,
            },
        );
    }
    if !T::DOC_XML.is_empty() {
        register_class_doc_xml(T::DOC_XML);
    }
    T::register_class(library);
}

/// Registers a direct `Child -> Parent` upcast edge. Transitive chains
/// (e.g. FGCFisterMovement -> FGMovementComponent -> FGComponent) resolve automatically.
pub fn register_parent_cast<Child: 'static, Parent: 'static>(
    to_parent_mut: impl Fn(&mut Child) -> &mut Parent + Send + Sync + 'static,
    to_parent_ref: impl Fn(&Child) -> &Parent + Send + Sync + 'static,
) {
    let cast_mut: &'static (dyn Fn(*mut ()) -> *mut () + Send + Sync) =
        Box::leak(Box::new(move |ptr: *mut ()| {
            let child = unsafe { &mut *(ptr as *mut Child) };
            to_parent_mut(child) as *mut Parent as *mut ()
        }));
    let cast_ref: &'static (dyn Fn(*const ()) -> *const () + Send + Sync) =
        Box::leak(Box::new(move |ptr: *const ()| {
            let child = unsafe { &*(ptr as *const Child) };
            to_parent_ref(child) as *const Parent as *const ()
        }));
    if let Ok(mut map) = PARENT_EDGES.lock() {
        map.insert(
            std::any::TypeId::of::<Child>(),
            ParentEdge {
                parent_id: std::any::TypeId::of::<Parent>(),
                cast_mut,
                cast_ref,
            },
        );
    }
}

/// Checks class inheritance in Rust using the parent links registered in `make_class`.
pub fn is_rust_subclass_of(mut class_name: &str, target_class: &str) -> bool {
    if class_name == target_class {
        return true;
    }
    if let Ok(map) = CLASS_PARENTS.lock() {
        let mut depth = 0;
        while let Some(&parent) = map.get(class_name) {
            if parent == target_class {
                return true;
            }
            class_name = parent;
            depth += 1;
            if depth > 32 {
                break;
            }
        }
    }
    false
}

pub fn upcast_user_data_mut(
    boxed: &mut (dyn std::any::Any + Send + Sync),
    target: std::any::TypeId,
) -> Option<*mut ()> {
    let mut cur_type = (&*boxed).type_id();
    let mut cur_ptr = (boxed as *mut (dyn std::any::Any + Send + Sync)) as *mut ();
    if cur_type == target {
        return Some(cur_ptr);
    }
    let map = PARENT_EDGES.lock().ok()?;
    let mut depth = 0;
    while cur_type != target {
        let edge = map.get(&cur_type)?;
        cur_ptr = (edge.cast_mut)(cur_ptr);
        cur_type = edge.parent_id;
        depth += 1;
        if depth > 32 {
            return None;
        }
    }
    Some(cur_ptr)
}

pub fn upcast_user_data_ref(
    boxed: &(dyn std::any::Any + Send + Sync),
    target: std::any::TypeId,
) -> Option<*const ()> {
    let mut cur_type = boxed.type_id();
    let mut cur_ptr = (boxed as *const (dyn std::any::Any + Send + Sync)) as *const ();
    if cur_type == target {
        return Some(cur_ptr);
    }
    let map = PARENT_EDGES.lock().ok()?;
    let mut depth = 0;
    while cur_type != target {
        let edge = map.get(&cur_type)?;
        cur_ptr = (edge.cast_ref)(cur_ptr);
        cur_type = edge.parent_id;
        depth += 1;
        if depth > 32 {
            return None;
        }
    }
    Some(cur_ptr)
}

unsafe fn dispatch_class_property_set(
    rust_inst: &RustNodeInstance,
    key: usize,
    value: GDExtensionConstVariantPtr,
) -> bool {
    let mut chain: Vec<fn(&RustNodeInstance, usize, GDExtensionConstVariantPtr) -> bool> = Vec::new();
    {
        if let (Ok(parents), Ok(handlers)) = (CLASS_PARENTS.lock(), CLASS_HANDLERS.lock()) {
            let mut cur = rust_inst.class_name;
            let mut depth = 0;
            loop {
                if let Some(h) = handlers.get(cur) {
                    chain.push(h.on_property_set);
                }
                match parents.get(cur) {
                    Some(&p) => {
                        cur = p;
                        depth += 1;
                        if depth > 32 {
                            break;
                        }
                    }
                    None => break,
                }
            }
        }
    }
    let mut handled = false;
    for cb in chain {
        if cb(rust_inst, key, value) {
            handled = true;
        }
    }
    handled
}

#[inline(always)]
pub fn get_user_data_downcast_hook() -> Option<UserDataDowncastHook> {
    let ptr = USER_DATA_DOWNCAST_HOOK.load(Ordering::SeqCst);
    if !ptr.is_null() {
        Some(unsafe { std::mem::transmute(ptr) })
    } else {
        Some(upcast_user_data_mut)
    }
}

#[inline(always)]
pub fn get_user_data_downcast_ref_hook() -> Option<UserDataDowncastRefHook> {
    let ptr = USER_DATA_DOWNCAST_REF_HOOK.load(Ordering::SeqCst);
    if !ptr.is_null() {
        Some(unsafe { std::mem::transmute(ptr) })
    } else {
        Some(upcast_user_data_ref)
    }
}

pub unsafe fn with_rust_user_data<T: 'static, R>(
    object: GDExtensionObjectPtr,
    f: impl FnOnce(&mut T) -> R,
) -> Option<R> {
    if let Some(inst) = get_rust_instance(object) {
        return (*inst).user_data.with_lock(|ud| {
            if let Some(ref mut boxed) = *ud {
                if let Some(val) = (**boxed).downcast_mut::<T>() {
                    return Some(f(val));
                }
                if let Some(hook) = get_user_data_downcast_hook() {
                    if let Some(ptr) = hook(&mut **boxed, std::any::TypeId::of::<T>()) {
                        let typed_ref = &mut *(ptr as *mut T);
                        return Some(f(typed_ref));
                    }
                }
            }
            None
        });
    }
    None
}

pub unsafe fn get_rust_user_data_ref<T: 'static, R>(
    object: GDExtensionObjectPtr,
    f: impl FnOnce(&T) -> R,
) -> Option<R> {
    if let Some(inst) = get_rust_instance(object) {
        return (*inst).user_data.with_ref(|ud| {
            if let Some(ref boxed) = *ud {
                if let Some(val) = (**boxed).downcast_ref::<T>() {
                    return Some(f(val));
                }
                if let Some(hook) = get_user_data_downcast_ref_hook() {
                    if let Some(ptr) = hook(&**boxed, std::any::TypeId::of::<T>()) {
                        let typed_ref = &*(ptr as *const T);
                        return Some(f(typed_ref));
                    }
                }
            }
            None
        });
    }
    None
}

pub unsafe extern "C" fn rust_class_set(
    instance: GDExtensionClassInstancePtr,
    name: GDExtensionConstStringNamePtr,
    value: GDExtensionConstVariantPtr,
) -> GDExtensionBool {
    if instance.is_null() || name.is_null() || value.is_null() {
        return 0;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let key = *(name as *const usize);

    if dispatch_class_property_set(rust_inst, key, value) {
        return 1;
    }

    if HAS_PROPERTY_SET_HOOKS.load(Ordering::Relaxed) {
        if let Ok(hooks) = PROPERTY_SET_HOOKS.lock() {
            for hook in hooks.iter() {
                if hook(rust_inst, key, value) {
                    return 1;
                }
            }
        }
    }

    0
}

pub unsafe extern "C" fn rust_class_get(
    instance: GDExtensionClassInstancePtr,
    name: GDExtensionConstStringNamePtr,
    ret: GDExtensionVariantPtr,
) -> GDExtensionBool {
    if instance.is_null() || name.is_null() || ret.is_null() {
        return 0;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let key = *(name as *const usize);

    if HAS_PROPERTY_GET_HOOKS.load(Ordering::Relaxed) {
        if let Ok(hooks) = PROPERTY_GET_HOOKS.lock() {
            for hook in hooks.iter() {
                if hook(rust_inst, key, ret) {
                    return 1;
                }
            }
        }
    }

    0
}



pub unsafe extern "C" fn rust_create_instance(
    class_userdata: *mut c_void,
    _notify_postinitialize: GDExtensionBool,
) -> GDExtensionObjectPtr {
    let desc = &*(class_userdata as *const RustClassDescriptor);
    let construct_ptr = CLASSDB_CONSTRUCT_OBJECT.load(Ordering::SeqCst);
    let set_inst_ptr = OBJECT_SET_INSTANCE.load(Ordering::SeqCst);
    if construct_ptr.is_null() || set_inst_ptr.is_null() {
        return std::ptr::null_mut();
    }
    let construct_fn: FnClassdbConstructObject = std::mem::transmute(construct_ptr);
    let set_inst_fn: FnObjectSetInstance = std::mem::transmute(set_inst_ptr);

    let godot_object = construct_fn(&desc.base_sn as *const _ as *const c_void);
    if godot_object.is_null() {
        return std::ptr::null_mut();
    }

    if desc.name == "FGCore" {
        rigidbody_set_center_of_mass_mode(godot_object, 1);
        rigidbody_set_gravity_scale(godot_object, 0.0);
        rigidbody_set_contact_monitor(godot_object, true);
        rigidbody_set_max_contacts_reported(godot_object, 16);
    }

    let mut user_data = None;
    let class_factory = CLASS_HANDLERS
        .lock()
        .ok()
        .and_then(|map| map.get(desc.name).map(|h| h.factory));
    if let Some(factory) = class_factory {
        user_data = Some(factory(godot_object));
    } else if let Ok(factories) = INSTANCE_FACTORIES.lock() {
        for factory in factories.iter() {
            if let Some(ud) = factory(godot_object, desc.name) {
                user_data = Some(ud);
                break;
            }
        }
    }

    let rust_instance = Box::into_raw(Box::new(RustNodeInstance {
        godot_object,
        class_name: desc.name,
        properties: Mutex::new(HashMap::new()),
        user_data: ReentrantMutex::new(user_data),
    }));

    if let Ok(mut map) = RUST_INSTANCES.lock() {
        map.insert(godot_object as usize, rust_instance as usize);
    }

    set_inst_fn(
        godot_object,
        &desc.class_sn as *const _ as *const c_void,
        rust_instance as *mut c_void,
    );

    godot_object
}

pub unsafe extern "C" fn rust_free_instance(
    _class_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
) {
    if !instance.is_null() {
        let rust_inst = Box::from_raw(instance as *mut RustNodeInstance);
        if let Ok(mut map) = RUST_INSTANCES.lock() {
            map.remove(&(rust_inst.godot_object as usize));
        }
    }
}

pub unsafe extern "C" fn rust_recreate_instance(
    class_userdata: *mut c_void,
    object: GDExtensionObjectPtr,
) -> GDExtensionClassInstancePtr {
    let desc = &*(class_userdata as *const RustClassDescriptor);
    let set_inst_ptr = OBJECT_SET_INSTANCE.load(Ordering::SeqCst);
    if set_inst_ptr.is_null() || object.is_null() {
        return std::ptr::null_mut();
    }
    let set_inst_fn: FnObjectSetInstance = std::mem::transmute(set_inst_ptr);

    let mut user_data = None;
    let class_factory = CLASS_HANDLERS
        .lock()
        .ok()
        .and_then(|map| map.get(desc.name).map(|h| h.factory));
    if let Some(factory) = class_factory {
        user_data = Some(factory(object));
    } else if let Ok(factories) = INSTANCE_FACTORIES.lock() {
        for factory in factories.iter() {
            if let Some(ud) = factory(object, desc.name) {
                user_data = Some(ud);
                break;
            }
        }
    }

    let rust_instance = Box::into_raw(Box::new(RustNodeInstance {
        godot_object: object,
        class_name: desc.name,
        properties: Mutex::new(HashMap::new()),
        user_data: ReentrantMutex::new(user_data),
    }));

    if let Ok(mut map) = RUST_INSTANCES.lock() {
        map.insert(object as usize, rust_instance as usize);
    }

    set_inst_fn(
        object,
        &desc.class_sn as *const _ as *const c_void,
        rust_instance as *mut c_void,
    );

    rust_instance as GDExtensionClassInstancePtr
}



pub unsafe fn register_method(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    call_func: GDExtensionClassMethodCall,
    method_flags: u32,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
) {
    register_method_full(
        library, class_sn, method_name, call_func, None,
        method_flags, has_return, return_type, return_class, args,
    );
}

struct PtrcallBridgeData {
    call_func: GDExtensionClassMethodCall,
    orig_userdata: *mut c_void,
    arg_types: &'static [u32],
    has_return: bool,
    return_type: u32,
}

unsafe extern "C" fn generic_call_bridge(
    method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const GDExtensionConstVariantPtr,
    arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    let bridge = &*(method_userdata as *const PtrcallBridgeData);
    (bridge.call_func)(bridge.orig_userdata, instance, args, arg_count, ret, error);
}

unsafe extern "C" fn generic_ptrcall_bridge(
    method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const *const c_void,
    ret: *mut c_void,
) {
    if method_userdata.is_null() {
        return;
    }
    let bridge = &*(method_userdata as *const PtrcallBridgeData);
    let get_from_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    let get_to_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);

    let arg_count = bridge.arg_types.len();
    let mut var_bufs: Vec<VariantBuffer> = Vec::with_capacity(arg_count);
    for i in 0..arg_count {
        let mut vb = VariantBuffer { data: [0u8; 48] };
        if !args.is_null() {
            let arg_ptr = *args.add(i);
            if !arg_ptr.is_null() {
                let atype = bridge.arg_types[i];
                if atype == GDEXTENSION_VARIANT_TYPE_NIL {
                    variant_copy(vb.data.as_mut_ptr() as *mut c_void, arg_ptr);
                } else if !get_from_ptr.is_null() {
                    let get_from: FnGetVariantFromTypeConstructor = std::mem::transmute(get_from_ptr);
                    if let Some(ctor) = get_from(atype) {
                        ctor(vb.data.as_mut_ptr() as *mut c_void, arg_ptr);
                    } else {
                        variant_new_nil(vb.data.as_mut_ptr() as *mut c_void);
                    }
                }
            }
        }
        var_bufs.push(vb);
    }

    let arg_ptrs: Vec<GDExtensionConstVariantPtr> = var_bufs
        .iter()
        .map(|vb| vb.data.as_ptr() as GDExtensionConstVariantPtr)
        .collect();

    let mut ret_vb = VariantBuffer { data: [0u8; 48] };
    variant_new_nil(ret_vb.data.as_mut_ptr() as *mut c_void);
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    (bridge.call_func)(
        bridge.orig_userdata,
        instance,
        if arg_ptrs.is_empty() { std::ptr::null() } else { arg_ptrs.as_ptr() },
        arg_count as GDExtensionInt,
        ret_vb.data.as_mut_ptr() as GDExtensionVariantPtr,
        &mut err,
    );

    if bridge.has_return && !ret.is_null() {
        if bridge.return_type == GDEXTENSION_VARIANT_TYPE_NIL {
            variant_copy(ret, ret_vb.data.as_ptr() as GDExtensionConstVariantPtr);
        } else if !get_to_ptr.is_null() {
            let get_to: FnGetVariantToTypeConstructor = std::mem::transmute(get_to_ptr);
            if let Some(ctor) = get_to(bridge.return_type) {
                ctor(ret, ret_vb.data.as_mut_ptr() as *mut c_void);
            }
        }
    }
}

pub unsafe fn register_method_full_with_userdata_and_defaults(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    call_func: GDExtensionClassMethodCall,
    ptrcall_func: Option<GDExtensionClassMethodPtrCall>,
    method_userdata: *mut c_void,
    method_flags: u32,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
    default_args: &[GDExtensionVariantPtr],
) {
    let reg_method_ptr = CLASSDB_REGISTER_METHOD.load(Ordering::SeqCst);
    if reg_method_ptr.is_null() {
        return;
    }
    let reg_method_fn: FnClassdbRegisterExtensionClassMethod = std::mem::transmute(reg_method_ptr);

    let method_name_sn = Box::leak(Box::new(ManagedStringName::new(method_name)));

    let mut return_info_ptr: *mut GDExtensionPropertyInfo = std::ptr::null_mut();
    if has_return {
        let ret_name_sn = Box::leak(Box::new(ManagedStringName::new("")));
        let ret_class_sn = Box::leak(Box::new(ManagedStringName::new(return_class.unwrap_or(""))));
        let rinfo = Box::leak(Box::new(GDExtensionPropertyInfo {
            r#type: return_type,
            name: ret_name_sn.as_ptr() as *mut c_void,
            class_name: ret_class_sn.as_ptr() as *mut c_void,
            hint: 0,
            hint_string: get_empty_string_ptr() as *mut c_void,
            usage: 6,
        }));
        return_info_ptr = rinfo as *mut _;
    }

    let mut args_info_ptr: *mut GDExtensionPropertyInfo = std::ptr::null_mut();
    let mut args_meta_ptr: *mut u32 = std::ptr::null_mut();
    if !args.is_empty() {
        let mut a_info: Vec<GDExtensionPropertyInfo> = Vec::with_capacity(args.len());
        let mut a_meta: Vec<u32> = Vec::with_capacity(args.len());
        for &(a_type, a_name) in args {
            let aname_sn = Box::leak(Box::new(ManagedStringName::new(a_name)));
            a_info.push(GDExtensionPropertyInfo {
                r#type: a_type,
                name: aname_sn.as_ptr() as *mut c_void,
                class_name: get_empty_string_name_ptr() as *mut c_void,
                hint: 0,
                hint_string: get_empty_string_ptr() as *mut c_void,
                usage: 6,
            });
            a_meta.push(0);
        }
        args_info_ptr = Box::leak(a_info.into_boxed_slice()).as_mut_ptr();
        args_meta_ptr = Box::leak(a_meta.into_boxed_slice()).as_mut_ptr();
    }

    let (final_call, final_ptrcall, final_userdata) = if let Some(pc) = ptrcall_func {
        (call_func, pc, method_userdata)
    } else {
        let arg_types: &'static [u32] = Box::leak(
            args.iter().map(|&(t, _)| t).collect::<Vec<u32>>().into_boxed_slice(),
        );
        let bridge = Box::leak(Box::new(PtrcallBridgeData {
            call_func,
            orig_userdata: method_userdata,
            arg_types,
            has_return,
            return_type,
        }));
        (
            generic_call_bridge as GDExtensionClassMethodCall,
            generic_ptrcall_bridge as GDExtensionClassMethodPtrCall,
            bridge as *mut PtrcallBridgeData as *mut c_void,
        )
    };

    let leaked_defaults = if default_args.is_empty() {
        std::ptr::null_mut()
    } else {
        Box::leak(default_args.to_vec().into_boxed_slice()).as_mut_ptr()
    };

    let minfo = Box::leak(Box::new(GDExtensionClassMethodInfo {
        name: method_name_sn.as_ptr() as *mut c_void,
        method_userdata: final_userdata,
        call_func: Some(final_call),
        ptrcall_func: Some(final_ptrcall),
        method_flags,
        has_return_value: if has_return { 1 } else { 0 },
        return_value_info: return_info_ptr,
        return_value_metadata: 0,
        argument_count: args.len() as u32,
        arguments_info: args_info_ptr,
        arguments_metadata: args_meta_ptr,
        default_argument_count: default_args.len() as u32,
        default_arguments: leaked_defaults,
    }));

    reg_method_fn(
        library,
        &class_sn as *const _ as *const c_void,
        minfo as *const _,
    );
}

pub unsafe fn register_virtual_method(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
) {
    let reg_vmethod_ptr = CLASSDB_REGISTER_VIRTUAL_METHOD.load(Ordering::SeqCst);
    if reg_vmethod_ptr.is_null() {
        return;
    }
    let reg_vmethod_fn: FnClassdbRegisterExtensionClassVirtualMethod = std::mem::transmute(reg_vmethod_ptr);

    let method_name_sn = Box::leak(Box::new(ManagedStringName::new(method_name)));

    let ret_name_sn = Box::leak(Box::new(ManagedStringName::new("")));
    let ret_class_sn = Box::leak(Box::new(ManagedStringName::new(return_class.unwrap_or(""))));
    let return_value = GDExtensionPropertyInfo {
        r#type: if has_return { return_type } else { GDEXTENSION_VARIANT_TYPE_NIL },
        name: ret_name_sn.as_ptr() as *mut c_void,
        class_name: ret_class_sn.as_ptr() as *mut c_void,
        hint: 0,
        hint_string: get_empty_string_ptr() as *mut c_void,
        usage: 6,
    };

    let mut args_info_ptr: *mut GDExtensionPropertyInfo = std::ptr::null_mut();
    let mut args_meta_ptr: *mut u32 = std::ptr::null_mut();
    if !args.is_empty() {
        let mut a_info: Vec<GDExtensionPropertyInfo> = Vec::with_capacity(args.len());
        let mut a_meta: Vec<u32> = Vec::with_capacity(args.len());
        for &(a_type, a_name) in args {
            let aname_sn = Box::leak(Box::new(ManagedStringName::new(a_name)));
            a_info.push(GDExtensionPropertyInfo {
                r#type: a_type,
                name: aname_sn.as_ptr() as *mut c_void,
                class_name: get_empty_string_name_ptr() as *mut c_void,
                hint: 0,
                hint_string: get_empty_string_ptr() as *mut c_void,
                usage: 6,
            });
            a_meta.push(0);
        }
        args_info_ptr = Box::leak(a_info.into_boxed_slice()).as_mut_ptr();
        args_meta_ptr = Box::leak(a_meta.into_boxed_slice()).as_mut_ptr();
    }

    let minfo = Box::leak(Box::new(GDExtensionClassVirtualMethodInfo {
        name: method_name_sn.as_ptr() as *mut c_void,
        method_flags: GDEXTENSION_METHOD_FLAG_VIRTUAL,
        return_value,
        return_value_metadata: 0,
        argument_count: args.len() as u32,
        arguments: args_info_ptr,
        arguments_metadata: args_meta_ptr,
    }));

    reg_vmethod_fn(
        library,
        &class_sn as *const usize as *const c_void,
        minfo as *const _,
    );
}

pub unsafe fn register_method_full_with_userdata(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    call_func: GDExtensionClassMethodCall,
    ptrcall_func: Option<GDExtensionClassMethodPtrCall>,
    method_userdata: *mut c_void,
    method_flags: u32,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
) {
    register_method_full_with_userdata_and_defaults(
        library, class_sn, method_name, call_func, ptrcall_func, method_userdata,
        method_flags, has_return, return_type, return_class, args, &[],
    );
}

pub unsafe fn register_method_defaults(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    call_func: GDExtensionClassMethodCall,
    method_flags: u32,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
    default_args: &[GDExtensionVariantPtr],
) {
    register_method_full_with_userdata_and_defaults(
        library, class_sn, method_name, call_func, None, std::ptr::null_mut(),
        method_flags, has_return, return_type, return_class, args, default_args,
    );
}

pub unsafe fn register_method_defaults_ptrcall(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    call_func: GDExtensionClassMethodCall,
    ptrcall_func: GDExtensionClassMethodPtrCall,
    method_flags: u32,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
    default_args: &[GDExtensionVariantPtr],
) {
    register_method_full_with_userdata_and_defaults(
        library, class_sn, method_name, call_func, Some(ptrcall_func), std::ptr::null_mut(),
        method_flags, has_return, return_type, return_class, args, default_args,
    );
}

pub unsafe fn register_method_with_ptrcall(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    call_func: GDExtensionClassMethodCall,
    ptrcall_func: GDExtensionClassMethodPtrCall,
    method_flags: u32,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
) {
    register_method_full_with_userdata(
        library, class_sn, method_name, call_func, Some(ptrcall_func), std::ptr::null_mut(),
        method_flags, has_return, return_type, return_class, args,
    );
}

pub unsafe fn register_method_full(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    method_name: &'static str,
    call_func: GDExtensionClassMethodCall,
    ptrcall_func: Option<GDExtensionClassMethodPtrCall>,
    method_flags: u32,
    has_return: bool,
    return_type: u32,
    return_class: Option<&'static str>,
    args: &'static [(u32, &'static str)],
) {
    register_method_full_with_userdata(
        library, class_sn, method_name, call_func, ptrcall_func, std::ptr::null_mut(),
        method_flags, has_return, return_type, return_class, args,
    );
}

pub unsafe fn register_property_with_accessors_full(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
    setter: &'static str,
    getter: &'static str,
    hint: u32,
    hint_string: &'static str,
    usage: u32,
) {
    let reg_prop_ptr = CLASSDB_REGISTER_PROPERTY.load(Ordering::SeqCst);
    if reg_prop_ptr.is_null() {
        return;
    }
    let reg_prop_fn: FnClassdbRegisterExtensionClassProperty = std::mem::transmute(reg_prop_ptr);

    let prop_name_sn = Box::leak(Box::new(ManagedStringName::new(prop_name)));
    let setter_sn = Box::leak(Box::new(ManagedStringName::new(setter)));
    let getter_sn = Box::leak(Box::new(ManagedStringName::new(getter)));
    let hint_str = Box::leak(Box::new(ManagedString::new(hint_string)));

    let pinfo = Box::leak(Box::new(GDExtensionPropertyInfo {
        r#type: prop_type,
        name: prop_name_sn.as_ptr() as *mut c_void,
        class_name: get_empty_string_name_ptr() as *mut c_void,
        hint,
        hint_string: hint_str.as_ptr() as *mut c_void,
        usage,
    }));

    reg_prop_fn(
        library,
        &class_sn as *const _ as *const c_void,
        pinfo as *const _,
        if setter.is_empty() { &EMPTY_STRING_NAME as *const _ as *const c_void } else { setter_sn.as_ptr() as *const c_void },
        if getter.is_empty() { &EMPTY_STRING_NAME as *const _ as *const c_void } else { getter_sn.as_ptr() as *const c_void },
    );
}

pub unsafe fn register_property_with_accessors(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
    setter: &'static str,
    getter: &'static str,
) {
    register_property_with_accessors_full(library, class_sn, prop_type, prop_name, setter, getter, 0, "", 6);
}

pub unsafe fn bind_property_full(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
    setter_override: Option<&'static str>,
    getter_override: Option<&'static str>,
    hint: u32,
    hint_string: &'static str,
    usage: u32,
    default_int: i64,
    default_float: f64,
) {
    let setter_name: &'static str = setter_override.unwrap_or_else(|| {
        Box::leak(format!("set_{}", prop_name).into_boxed_str())
    });
    let getter_name: &'static str = getter_override.unwrap_or_else(|| {
        Box::leak(format!("get_{}", prop_name).into_boxed_str())
    });

    let prop_name_sn = ManagedStringName::new(prop_name);
    let prop_info = Box::leak(Box::new(BoundPropInfo {
        name: prop_name,
        name_sn: prop_name_sn.0,
        prop_type,
        setter_name,
        getter_name,
        default_int,
        default_float,
    }));

    let normal_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    let setter_args: &'static [(u32, &'static str)] = Box::leak(vec![(prop_type, "value")].into_boxed_slice());

    // 1. Setter method: set_<prop_name>(value)
    register_method_full_with_userdata(
        library,
        class_sn,
        setter_name,
        generic_prop_set_call,
        Some(generic_prop_set_ptrcall),
        prop_info as *mut _ as *mut c_void,
        normal_flags,
        false,
        GDEXTENSION_VARIANT_TYPE_NIL,
        None,
        setter_args,
    );

    // 2. Getter method: get_<prop_name>() -> prop_type
    register_method_full_with_userdata(
        library,
        class_sn,
        getter_name,
        generic_prop_get_call,
        Some(generic_prop_get_ptrcall),
        prop_info as *mut _ as *mut c_void,
        normal_flags,
        true,
        prop_type,
        None,
        &[],
    );

    // Also register is_<prop_name> if bool, or vice-versa
    if prop_type == GDEXTENSION_VARIANT_TYPE_BOOL {
        let alt_getter = if getter_name.starts_with("get_") {
            Box::leak(format!("is_{}", prop_name).into_boxed_str()) as &'static str
        } else {
            Box::leak(format!("get_{}", prop_name).into_boxed_str()) as &'static str
        };
        register_method_full_with_userdata(
            library,
            class_sn,
            alt_getter,
            generic_prop_get_call,
            Some(generic_prop_get_ptrcall),
            prop_info as *mut _ as *mut c_void,
            normal_flags,
            true,
            prop_type,
            None,
            &[],
        );
    }

    // 3. Register property with accessors in ClassDB
    register_property_with_accessors_full(
        library,
        class_sn,
        prop_type,
        prop_name,
        setter_name,
        getter_name,
        hint,
        hint_string,
        usage,
    );
}

#[inline]
pub unsafe fn bind_prop(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
) {
    bind_property_full(library, class_sn, prop_type, prop_name, None, None, 0, "", 6, 0, 0.0);
}

#[inline]
pub unsafe fn bind_prop_def(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
    default_int: i64,
    default_float: f64,
) {
    bind_property_full(library, class_sn, prop_type, prop_name, None, None, 0, "", 6, default_int, default_float);
}

#[inline]
pub unsafe fn bind_prop_hint(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
    hint: u32,
    hint_string: &'static str,
) {
    bind_property_full(library, class_sn, prop_type, prop_name, None, None, hint, hint_string, 6, 0, 0.0);
}

#[inline]
pub unsafe fn bind_prop_usage(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
    hint: u32,
    hint_string: &'static str,
    usage: u32,
) {
    bind_property_full(library, class_sn, prop_type, prop_name, None, None, hint, hint_string, usage, 0, 0.0);
}

#[inline]
pub unsafe fn bind_prop_var(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
) {
    bind_property_full(library, class_sn, prop_type, prop_name, None, None, 0, "", PROPERTY_USAGE_SCRIPT_VARIABLE, 0, 0.0);
}

pub unsafe fn bind_get_set(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
) {
    let setter_name: &'static str = Box::leak(format!("set_{}", prop_name).into_boxed_str());
    let getter_name: &'static str = Box::leak(format!("get_{}", prop_name).into_boxed_str());
    let prop_name_sn = ManagedStringName::new(prop_name);
    let prop_info = Box::leak(Box::new(BoundPropInfo {
        name: prop_name,
        name_sn: prop_name_sn.0,
        prop_type,
        setter_name,
        getter_name,
        default_int: 0,
        default_float: 0.0,
    }));
    let normal_flags = GDEXTENSION_METHOD_FLAG_NORMAL;
    let setter_args: &'static [(u32, &'static str)] = Box::leak(vec![(prop_type, "value")].into_boxed_slice());

    register_method_full_with_userdata(
        library,
        class_sn,
        setter_name,
        generic_prop_set_call,
        Some(generic_prop_set_ptrcall),
        prop_info as *mut _ as *mut c_void,
        normal_flags,
        false,
        GDEXTENSION_VARIANT_TYPE_NIL,
        None,
        setter_args,
    );

    register_method_full_with_userdata(
        library,
        class_sn,
        getter_name,
        generic_prop_get_call,
        Some(generic_prop_get_ptrcall),
        prop_info as *mut _ as *mut c_void,
        normal_flags,
        true,
        prop_type,
        None,
        &[],
    );
}

#[inline]
pub unsafe fn bind_prop_read_only(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
) {
    bind_property_full(library, class_sn, prop_type, prop_name, None, None, 0, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_READ_ONLY, 0, 0.0);
}

pub unsafe fn add_group(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    group_name: &'static str,
    prefix: &'static str,
) {
    let reg_group_ptr = CLASSDB_REGISTER_PROPERTY_GROUP.load(Ordering::SeqCst);
    if reg_group_ptr.is_null() {
        return;
    }
    let reg_group_fn: FnClassdbRegisterExtensionClassPropertyGroup = std::mem::transmute(reg_group_ptr);
    let g_name = Box::leak(Box::new(ManagedString::new(group_name)));
    let p_fix = Box::leak(Box::new(ManagedString::new(prefix)));
    reg_group_fn(
        library,
        &class_sn as *const _ as *const c_void,
        g_name.as_ptr(),
        p_fix.as_ptr(),
    );
}

pub unsafe fn add_subgroup(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    subgroup_name: &'static str,
    prefix: &'static str,
) {
    let reg_subgroup_ptr = CLASSDB_REGISTER_PROPERTY_SUBGROUP.load(Ordering::SeqCst);
    if reg_subgroup_ptr.is_null() {
        return;
    }
    let reg_subgroup_fn: FnClassdbRegisterExtensionClassPropertySubgroup = std::mem::transmute(reg_subgroup_ptr);
    let s_name = Box::leak(Box::new(ManagedString::new(subgroup_name)));
    let p_fix = Box::leak(Box::new(ManagedString::new(prefix)));
    reg_subgroup_fn(
        library,
        &class_sn as *const _ as *const c_void,
        s_name.as_ptr(),
        p_fix.as_ptr(),
    );
}

#[inline]
pub unsafe fn register_property(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    prop_type: u32,
    prop_name: &'static str,
) {
    bind_prop(library, class_sn, prop_type, prop_name);
}

pub unsafe fn register_signal(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    signal_name: &'static str,
    args: &'static [(u32, &'static str)],
) {
    let reg_sig_ptr = CLASSDB_REGISTER_SIGNAL.load(Ordering::SeqCst);
    if reg_sig_ptr.is_null() {
        return;
    }
    let reg_sig_fn: FnClassdbRegisterExtensionClassSignal = std::mem::transmute(reg_sig_ptr);

    let sig_name_sn = Box::leak(Box::new(ManagedStringName::new(signal_name)));
    let mut args_info: Vec<GDExtensionPropertyInfo> = Vec::with_capacity(args.len());
    for &(a_type, a_name) in args {
        let aname_sn = Box::leak(Box::new(ManagedStringName::new(a_name)));
        args_info.push(GDExtensionPropertyInfo {
            r#type: a_type,
            name: aname_sn.as_ptr() as *mut c_void,
            class_name: get_empty_string_name_ptr() as *mut c_void,
            hint: 0,
            hint_string: get_empty_string_ptr() as *mut c_void,
            usage: 6,
        });
    }
    let leaked_args = Box::leak(args_info.into_boxed_slice());

    reg_sig_fn(
        library,
        &class_sn as *const _ as *const c_void,
        sig_name_sn.as_ptr() as *const c_void,
        if args.is_empty() { std::ptr::null() } else { leaked_args.as_ptr() },
        args.len() as GDExtensionInt,
    );
}

pub unsafe fn register_integer_constant(
    library: GDExtensionClassLibraryPtr,
    class_sn: usize,
    enum_name: &'static str,
    const_name: &'static str,
    value: i64,
    is_bitfield: bool,
) {
    let reg_const_ptr = CLASSDB_REGISTER_INTEGER_CONSTANT.load(Ordering::SeqCst);
    if reg_const_ptr.is_null() {
        return;
    }
    let reg_const_fn: FnClassdbRegisterExtensionClassIntegerConstant = std::mem::transmute(reg_const_ptr);

    let enum_sn = Box::leak(Box::new(ManagedStringName::new(enum_name)));
    let const_sn = Box::leak(Box::new(ManagedStringName::new(const_name)));

    reg_const_fn(
        library,
        &class_sn as *const _ as *const c_void,
        enum_sn.as_ptr() as *const c_void,
        const_sn.as_ptr() as *const c_void,
        value,
        if is_bitfield { 1 } else { 0 },
    );
}

// ============================================================
// Generic Method Callbacks for ClassDB bindings
// ============================================================

pub unsafe extern "C" fn generic_void_call(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    _ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
}

pub unsafe extern "C" fn generic_bool_true_call(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if !ret.is_null() {
        variant_from_bool(ret, true);
    }
}

pub unsafe extern "C" fn generic_bool_false_call(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if !ret.is_null() {
        variant_from_bool(ret, false);
    }
}

pub unsafe extern "C" fn generic_nil_call(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if !ret.is_null() {
        variant_new_nil(ret);
    }
}

pub unsafe extern "C" fn generic_array_call(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if !ret.is_null() {
        variant_from_array(ret);
    }
}

pub unsafe extern "C" fn generic_int_zero_call(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if !ret.is_null() {
        variant_from_int(ret, 0);
    }
}

pub unsafe extern "C" fn generic_prop_set_call(
    method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const GDExtensionConstVariantPtr,
    arg_count: GDExtensionInt,
    _ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if instance.is_null() || args.is_null() || arg_count < 1 || method_userdata.is_null() {
        return;
    }
    let prop_info = &*(method_userdata as *const BoundPropInfo);
    let rust_inst = &*(instance as *const RustNodeInstance);
    let val_var = *args.add(0);

    let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
    if !copy_ptr.is_null() {
        let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
        let mut vb = VariantBuffer { data: [0u8; 48] };
        copy_fn(vb.data.as_mut_ptr() as *mut c_void, val_var);
        if let Ok(mut props) = rust_inst.properties.lock() {
            props.insert(prop_info.name_sn, vb);
        }
    }

    if prop_info.name == "net_id" && !rust_inst.godot_object.is_null() {
        let net_id = variant_to_int(val_var) as u32;
        if net_id != 0 {
            let instance_id = object_get_instance_id(rust_inst.godot_object);
            trigger_net_id_callback(net_id, instance_id);
        }
    }

    dispatch_class_property_set(rust_inst, prop_info.name_sn, val_var);

    let hooks = if let Ok(h) = PROPERTY_SET_HOOKS.lock() {
        h.clone()
    } else {
        Vec::new()
    };
    for hook in hooks {
        hook(rust_inst, prop_info.name_sn, val_var);
    }
}

pub unsafe extern "C" fn generic_prop_get_call(
    method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if ret.is_null() {
        return;
    }
    if instance.is_null() || method_userdata.is_null() {
        variant_new_nil(ret);
        return;
    }
    let prop_info = &*(method_userdata as *const BoundPropInfo);
    let rust_inst = &*(instance as *const RustNodeInstance);
    let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);

    if let Ok(props) = rust_inst.properties.lock() {
        if let Some(vb) = props.get(&prop_info.name_sn) {
            if !copy_ptr.is_null() {
                let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
                copy_fn(ret, vb.data.as_ptr() as *const c_void);
                return;
            }
        }
    }

    if prop_info.default_int != 0 && prop_info.prop_type == GDEXTENSION_VARIANT_TYPE_INT {
        variant_from_int(ret, prop_info.default_int);
        return;
    }
    if prop_info.default_float != 0.0 && prop_info.prop_type == GDEXTENSION_VARIANT_TYPE_FLOAT {
        variant_from_float(ret, prop_info.default_float);
        return;
    }

    construct_default_variant(ret, prop_info.prop_type);
}

pub unsafe extern "C" fn generic_prop_set_ptrcall(
    method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const *const c_void,
    _ret: *mut c_void,
) {
    if instance.is_null() || args.is_null() || method_userdata.is_null() {
        return;
    }
    let prop_info = &*(method_userdata as *const BoundPropInfo);
    let rust_inst = &*(instance as *const RustNodeInstance);
    let arg_ptr = *args.add(0);
    if arg_ptr.is_null() {
        return;
    }

    let mut vb = VariantBuffer { data: [0u8; 48] };
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(prop_info.prop_type) {
            ctor(vb.data.as_mut_ptr() as *mut c_void, arg_ptr as *const c_void);
        } else {
            variant_new_nil(vb.data.as_mut_ptr() as *mut c_void);
        }
    } else {
        variant_new_nil(vb.data.as_mut_ptr() as *mut c_void);
    }

    if prop_info.name == "net_id" && !rust_inst.godot_object.is_null() {
        let net_id = match prop_info.prop_type {
            GDEXTENSION_VARIANT_TYPE_INT => *(arg_ptr as *const i64) as u32,
            _ => 0,
        };
        if net_id != 0 {
            let instance_id = object_get_instance_id(rust_inst.godot_object);
            trigger_net_id_callback(net_id, instance_id);
        }
    }

    dispatch_class_property_set(rust_inst, prop_info.name_sn, vb.data.as_ptr() as GDExtensionConstVariantPtr);

    let hooks = if let Ok(h) = PROPERTY_SET_HOOKS.lock() {
        h.clone()
    } else {
        Vec::new()
    };
    for hook in &hooks {
        hook(rust_inst, prop_info.name_sn, vb.data.as_ptr() as GDExtensionConstVariantPtr);
    }

    if let Ok(mut props) = rust_inst.properties.lock() {
        props.insert(prop_info.name_sn, vb);
    }
}

pub unsafe extern "C" fn generic_prop_get_ptrcall(
    method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if ret.is_null() || instance.is_null() || method_userdata.is_null() {
        return;
    }
    let prop_info = &*(method_userdata as *const BoundPropInfo);
    let rust_inst = &*(instance as *const RustNodeInstance);

    if let Ok(props) = rust_inst.properties.lock() {
        if let Some(vb) = props.get(&prop_info.name_sn) {
            let vb_ptr = vb.data.as_ptr() as *const c_void;
            let get_to_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
            if !get_to_ctor_ptr.is_null() {
                let get_to_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_to_ctor_ptr);
                if let Some(ctor) = get_to_ctor(prop_info.prop_type) {
                    ctor(ret, vb_ptr as *mut c_void);
                    return;
                }
            }
        }
    }

    match prop_info.prop_type {
        GDEXTENSION_VARIANT_TYPE_BOOL => {
            *(ret as *mut u8) = 0;
        }
        GDEXTENSION_VARIANT_TYPE_INT => {
            *(ret as *mut i64) = prop_info.default_int;
        }
        GDEXTENSION_VARIANT_TYPE_FLOAT => {
            *(ret as *mut f64) = prop_info.default_float;
        }
        _ => {
            let mut def_vb = VariantBuffer { data: [0u8; 48] };
            construct_default_variant(def_vb.data.as_mut_ptr() as *mut c_void, prop_info.prop_type);
            let get_to_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
            if !get_to_ctor_ptr.is_null() {
                let get_to_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_to_ctor_ptr);
                if let Some(ctor) = get_to_ctor(prop_info.prop_type) {
                    ctor(ret, def_vb.data.as_ptr() as *mut c_void);
                }
            }
        }
    }
}

pub unsafe extern "C" fn generic_get_net_id_call(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if instance.is_null() || ret.is_null() {
        variant_new_nil(ret);
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    static NET_ID_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("net_id"));
    let key = NET_ID_SN.0;
    if let Ok(props) = rust_inst.properties.lock() {
        if let Some(vb) = props.get(&key) {
            let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
            if !copy_ptr.is_null() {
                let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
                copy_fn(ret, vb.data.as_ptr() as *const c_void);
                return;
            }
        }
    }
    variant_from_int(ret, 0);
}

pub unsafe extern "C" fn generic_get_net_id_ptrcall(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if ret.is_null() {
        return;
    }
    let mut id: i64 = 0;
    if !instance.is_null() {
        let rust_inst = &*(instance as *const RustNodeInstance);
        static NET_ID_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("net_id"));
        let key = NET_ID_SN.0;
        if let Ok(props) = rust_inst.properties.lock() {
            if let Some(vb) = props.get(&key) {
                id = variant_to_int(vb.data.as_ptr() as *const c_void);
            }
        }
    }
    *(ret as *mut i64) = id;
}

pub unsafe extern "C" fn generic_set_net_id_call(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const GDExtensionConstVariantPtr,
    arg_count: GDExtensionInt,
    _ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if instance.is_null() || args.is_null() || arg_count < 1 {
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let val_var = *args.add(0);
    let net_id = variant_to_int(val_var) as u32;

    static NET_ID_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("net_id"));
    let key = NET_ID_SN.0;

    let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
    if !copy_ptr.is_null() {
        let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
        let mut vb = VariantBuffer { data: [0u8; 48] };
        copy_fn(vb.data.as_mut_ptr() as *mut c_void, val_var);
        if let Ok(mut props) = rust_inst.properties.lock() {
            props.insert(key, vb);
        }
    }

    if net_id != 0 && !rust_inst.godot_object.is_null() {
        let instance_id = object_get_instance_id(rust_inst.godot_object);
        trigger_net_id_callback(net_id, instance_id);
    }
}

pub unsafe extern "C" fn generic_set_net_id_ptrcall(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const *const c_void,
    _ret: *mut c_void,
) {
    if instance.is_null() || args.is_null() {
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let net_id = *(*args.add(0) as *const i64) as u32;

    static NET_ID_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("net_id"));
    let key = NET_ID_SN.0;

    let mut vb = VariantBuffer { data: [0u8; 48] };
    variant_from_int(vb.data.as_mut_ptr() as *mut c_void, net_id as i64);
    if let Ok(mut props) = rust_inst.properties.lock() {
        props.insert(key, vb);
    }

    if net_id != 0 && !rust_inst.godot_object.is_null() {
        let instance_id = object_get_instance_id(rust_inst.godot_object);
        trigger_net_id_callback(net_id, instance_id);
    }
}

pub unsafe extern "C" fn generic_get_owner_member_id_call(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if instance.is_null() || ret.is_null() {
        variant_new_nil(ret);
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    static OWNER_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("owner_member_id"));
    let key = OWNER_SN.0;
    if let Ok(props) = rust_inst.properties.lock() {
        if let Some(vb) = props.get(&key) {
            let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
            if !copy_ptr.is_null() {
                let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
                copy_fn(ret, vb.data.as_ptr() as *const c_void);
                return;
            }
        }
    }
    variant_from_int(ret, -1);
}

pub unsafe extern "C" fn generic_get_owner_member_id_ptrcall(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if ret.is_null() {
        return;
    }
    let mut id: i64 = -1;
    if !instance.is_null() {
        let rust_inst = &*(instance as *const RustNodeInstance);
        static OWNER_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("owner_member_id"));
        let key = OWNER_SN.0;
        if let Ok(props) = rust_inst.properties.lock() {
            if let Some(vb) = props.get(&key) {
                id = variant_to_int(vb.data.as_ptr() as *const c_void);
            }
        }
    }
    *(ret as *mut i64) = id;
}

pub unsafe extern "C" fn generic_set_owner_member_id_call(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const GDExtensionConstVariantPtr,
    arg_count: GDExtensionInt,
    _ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if instance.is_null() || args.is_null() || arg_count < 1 {
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let val_var = *args.add(0);

    static OWNER_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("owner_member_id"));
    let key = OWNER_SN.0;

    let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
    if !copy_ptr.is_null() {
        let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
        let mut vb = VariantBuffer { data: [0u8; 48] };
        copy_fn(vb.data.as_mut_ptr() as *mut c_void, val_var);
        if let Ok(mut props) = rust_inst.properties.lock() {
            props.insert(key, vb);
        }
    }
}

pub unsafe extern "C" fn generic_set_owner_member_id_ptrcall(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const *const c_void,
    _ret: *mut c_void,
) {
    if instance.is_null() || args.is_null() {
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let id = *(*args.add(0) as *const i64);

    static OWNER_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("owner_member_id"));
    let key = OWNER_SN.0;

    let mut vb = VariantBuffer { data: [0u8; 48] };
    variant_from_int(vb.data.as_mut_ptr() as *mut c_void, id);
    if let Ok(mut props) = rust_inst.properties.lock() {
        props.insert(key, vb);
    }
}

pub unsafe extern "C" fn generic_get_replicate_call(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const GDExtensionConstVariantPtr,
    _arg_count: GDExtensionInt,
    ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if instance.is_null() || ret.is_null() {
        variant_new_nil(ret);
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    static REP_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("replicate"));
    let key = REP_SN.0;
    if let Ok(props) = rust_inst.properties.lock() {
        if let Some(vb) = props.get(&key) {
            let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
            if !copy_ptr.is_null() {
                let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
                copy_fn(ret, vb.data.as_ptr() as *const c_void);
                return;
            }
        }
    }
    variant_from_bool(ret, true);
}

pub unsafe extern "C" fn generic_get_replicate_ptrcall(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if ret.is_null() {
        return;
    }
    let mut b: bool = true;
    if !instance.is_null() {
        let rust_inst = &*(instance as *const RustNodeInstance);
        static REP_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("replicate"));
        let key = REP_SN.0;
        if let Ok(props) = rust_inst.properties.lock() {
            if let Some(vb) = props.get(&key) {
                b = variant_to_bool(vb.data.as_ptr() as *const c_void);
            }
        }
    }
    *(ret as *mut u8) = if b { 1 } else { 0 };
}

pub unsafe extern "C" fn generic_set_replicate_call(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const GDExtensionConstVariantPtr,
    arg_count: GDExtensionInt,
    _ret: GDExtensionVariantPtr,
    error: *mut GDExtensionCallError,
) {
    if !error.is_null() {
        (*error).error = GDEXTENSION_CALL_OK;
    }
    if instance.is_null() || args.is_null() || arg_count < 1 {
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let val_var = *args.add(0);

    static REP_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("replicate"));
    let key = REP_SN.0;

    let copy_ptr = VARIANT_NEW_COPY.load(Ordering::SeqCst);
    if !copy_ptr.is_null() {
        let copy_fn: FnVariantNewCopy = std::mem::transmute(copy_ptr);
        let mut vb = VariantBuffer { data: [0u8; 48] };
        copy_fn(vb.data.as_mut_ptr() as *mut c_void, val_var);
        if let Ok(mut props) = rust_inst.properties.lock() {
            props.insert(key, vb);
        }
    }
}

pub unsafe extern "C" fn generic_set_replicate_ptrcall(
    _method_userdata: *mut c_void,
    instance: GDExtensionClassInstancePtr,
    args: *const *const c_void,
    _ret: *mut c_void,
) {
    if instance.is_null() || args.is_null() {
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let b = *(*args.add(0) as *const u8) != 0;

    static REP_SN: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("replicate"));
    let key = REP_SN.0;

    let mut vb = VariantBuffer { data: [0u8; 48] };
    variant_from_bool(vb.data.as_mut_ptr() as *mut c_void, b);
    if let Ok(mut props) = rust_inst.properties.lock() {
        props.insert(key, vb);
    }
}

// ============================================================
// Generic and Specific PtrCall Implementations (GDScript Validated Calls)
// ============================================================

pub unsafe extern "C" fn generic_ptrcall_void(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    _ret: *mut c_void,
) {}

pub unsafe extern "C" fn generic_ptrcall_bool_false(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if !ret.is_null() {
        *(ret as *mut u8) = 0;
    }
}

pub unsafe extern "C" fn generic_ptrcall_bool_true(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if !ret.is_null() {
        *(ret as *mut u8) = 1;
    }
}

pub unsafe extern "C" fn generic_ptrcall_int_zero(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if !ret.is_null() {
        *(ret as *mut i64) = 0;
    }
}

pub unsafe extern "C" fn generic_ptrcall_object_null(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if !ret.is_null() {
        *(ret as *mut *mut c_void) = std::ptr::null_mut();
    }
}

pub unsafe extern "C" fn generic_ptrcall_vector3_zero(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if !ret.is_null() {
        *(ret as *mut [f32; 3]) = [0.0, 0.0, 0.0];
    }
}

pub unsafe extern "C" fn generic_ptrcall_transform3d_identity(
    _method_userdata: *mut c_void,
    _instance: GDExtensionClassInstancePtr,
    _args: *const *const c_void,
    ret: *mut c_void,
) {
    if !ret.is_null() {
        *(ret as *mut [f32; 12]) = [
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            0.0, 0.0, 0.0,
        ];
    }
}



pub unsafe fn node_set_process(obj: GDExtensionObjectPtr, enable: bool) {
    let mb = METHOD_SET_PROCESS.load(Ordering::SeqCst);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::SeqCst);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let arg: u8 = if enable { 1 } else { 0 };
        let args = [&arg as *const u8 as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
    }
}

pub unsafe fn node_set_physics_process(obj: GDExtensionObjectPtr, enable: bool) {
    let mb = METHOD_SET_PHYSICS_PROCESS.load(Ordering::SeqCst);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::SeqCst);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let arg: u8 = if enable { 1 } else { 0 };
        let args = [&arg as *const u8 as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
    }
}

pub unsafe fn node_get_process_delta_time(obj: GDExtensionObjectPtr) -> f64 {
    let mb = METHOD_GET_PROCESS_DELTA.load(Ordering::SeqCst);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::SeqCst);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut delta: f64 = 0.0;
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), &mut delta as *mut f64 as *mut c_void);
        delta
    } else {
        1.0 / 60.0
    }
}

pub unsafe fn node_get_physics_process_delta_time(obj: GDExtensionObjectPtr) -> f64 {
    let mb = METHOD_GET_PHYSICS_PROCESS_DELTA.load(Ordering::SeqCst);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::SeqCst);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut delta: f64 = 0.0;
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), &mut delta as *mut f64 as *mut c_void);
        delta
    } else {
        1.0 / 60.0
    }
}

pub unsafe fn rigidbody_set_center_of_mass_mode(obj: GDExtensionObjectPtr, mode: i64) {
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(var.data.as_mut_ptr() as *mut c_void, mode);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_center_of_mass_mode", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_set_gravity_scale(obj: GDExtensionObjectPtr, scale: f64) {
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_float(var.data.as_mut_ptr() as *mut c_void, scale);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_gravity_scale", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_get_gravity_scale(obj: GDExtensionObjectPtr) -> f64 {
    let mb = METHOD_GET_GRAVITY_SCALE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret: f32 = 0.0;
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), &mut ret as *mut _ as *mut c_void);
        return ret as f64;
    }
    call_object_method_ret_float(obj, "get_gravity_scale")
}

pub unsafe fn call_object_method_ret_vector3(obj: GDExtensionObjectPtr, method_name: &str) -> [f32; 3] {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return [0.0, 0.0, 0.0];
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let res = variant_to_vector3(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn call_object_method_ret_float(obj: GDExtensionObjectPtr, method_name: &str) -> f64 {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return 0.0;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let res = variant_to_float(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn rigidbody_set_freeze_enabled(obj: GDExtensionObjectPtr, freeze: bool) {
    let mb = METHOD_SET_FREEZE_ENABLED.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let b: u8 = if freeze { 1 } else { 0 };
        let args = [&b as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_bool(var.data.as_mut_ptr() as *mut c_void, freeze);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_freeze_enabled", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_set_freeze_mode(obj: GDExtensionObjectPtr, mode: i64) {
    let mb = METHOD_SET_FREEZE_MODE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&mode as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(var.data.as_mut_ptr() as *mut c_void, mode);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_freeze_mode", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_set_linear_velocity(obj: GDExtensionObjectPtr, vel: [f32; 3]) {
    let mb = METHOD_SET_LINEAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&vel as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut vec_var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(vec_var.data.as_mut_ptr() as *mut c_void, vel);
    let args = [vec_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_linear_velocity", &args);
    variant_destroy(vec_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_get_linear_velocity(obj: GDExtensionObjectPtr) -> [f32; 3] {
    let mb = METHOD_GET_LINEAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    call_object_method_ret_vector3(obj, "get_linear_velocity")
}

pub unsafe fn rigidbody_set_angular_velocity(obj: GDExtensionObjectPtr, vel: [f32; 3]) {
    let mb = METHOD_SET_ANGULAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&vel as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut vec_var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(vec_var.data.as_mut_ptr() as *mut c_void, vel);
    let args = [vec_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_angular_velocity", &args);
    variant_destroy(vec_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_get_angular_velocity(obj: GDExtensionObjectPtr) -> [f32; 3] {
    let mb = METHOD_GET_ANGULAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    call_object_method_ret_vector3(obj, "get_angular_velocity")
}

pub unsafe fn rigidbody_set_use_custom_integrator(obj: GDExtensionObjectPtr, enable: bool) {
    let mb = METHOD_SET_USE_CUSTOM_INTEGRATOR.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let b: u8 = if enable { 1 } else { 0 };
        let args = [&b as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_bool(var.data.as_mut_ptr() as *mut c_void, enable);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_use_custom_integrator", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_is_using_custom_integrator(obj: GDExtensionObjectPtr) -> bool {
    let mb = METHOD_IS_USING_CUSTOM_INTEGRATOR.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret: u8 = 0;
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), &mut ret as *mut _ as *mut c_void);
        return ret != 0;
    }
    call_object_method_ret_bool(obj, "is_using_custom_integrator")
}

pub unsafe fn rigidbody_is_lock_rotation_enabled(obj: GDExtensionObjectPtr) -> bool {
    let mb = METHOD_IS_LOCK_ROTATION_ENABLED.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret: u8 = 0;
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), &mut ret as *mut _ as *mut c_void);
        return ret != 0;
    }
    call_object_method_ret_bool(obj, "is_lock_rotation_enabled")
}

pub unsafe fn rigidbody_set_lock_rotation_enabled(obj: GDExtensionObjectPtr, enable: bool) {
    let mb = METHOD_SET_LOCK_ROTATION_ENABLED.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let b: u8 = if enable { 1 } else { 0 };
        let args = [&b as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_bool(var.data.as_mut_ptr() as *mut c_void, enable);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_lock_rotation_enabled", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_set_max_contacts_reported(obj: GDExtensionObjectPtr, amount: i32) {
    let mb = METHOD_SET_MAX_CONTACTS_REPORTED.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&amount as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(var.data.as_mut_ptr() as *mut c_void, amount as i64);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_max_contacts_reported", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_set_contact_monitor(obj: GDExtensionObjectPtr, enable: bool) {
    let mb = METHOD_SET_CONTACT_MONITOR.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let b: u8 = if enable { 1 } else { 0 };
        let args = [&b as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_bool(var.data.as_mut_ptr() as *mut c_void, enable);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_contact_monitor", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_set_mass(obj: GDExtensionObjectPtr, mass: f64) {
    let mb = METHOD_SET_MASS.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let m_f32 = mass as f32;
        let args = [&m_f32 as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_float(var.data.as_mut_ptr() as *mut c_void, mass);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_mass", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_get_mass(obj: GDExtensionObjectPtr) -> f64 {
    let mb = METHOD_GET_MASS.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret: f32 = 0.0;
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), &mut ret as *mut _ as *mut c_void);
        return ret as f64;
    }
    call_object_method_ret_float(obj, "get_mass")
}

pub unsafe fn rigidbody_set_center_of_mass(obj: GDExtensionObjectPtr, com: &[f32; 3]) {
    let mb = METHOD_SET_CENTER_OF_MASS.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [com.as_ptr() as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(var.data.as_mut_ptr() as *mut c_void, *com);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_center_of_mass", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_get_center_of_mass(obj: GDExtensionObjectPtr) -> [f32; 3] {
    let mb = METHOD_GET_CENTER_OF_MASS.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    call_object_method_ret_vector3(obj, "get_center_of_mass")
}

pub unsafe fn rigidbody_apply_central_impulse(obj: GDExtensionObjectPtr, impulse: &[f32; 3]) {
    let mb = METHOD_RIGIDBODY_APPLY_CENTRAL_IMPULSE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [impulse.as_ptr() as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(var.data.as_mut_ptr() as *mut c_void, *impulse);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "apply_central_impulse", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn rigidbody_apply_torque_impulse(obj: GDExtensionObjectPtr, impulse: &[f32; 3]) {
    let mb = METHOD_RIGIDBODY_APPLY_TORQUE_IMPULSE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [impulse.as_ptr() as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(var.data.as_mut_ptr() as *mut c_void, *impulse);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "apply_torque_impulse", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node3d_get_global_transform(obj: GDExtensionObjectPtr) -> [f32; 12] {
    let mb = METHOD_NODE3D_GET_GLOBAL_TRANSFORM.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut trans = [
            1.0f32, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            0.0, 0.0, 0.0,
        ];
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), trans.as_mut_ptr() as *mut c_void);
        return trans;
    }
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if !call_ptr.is_null() && !obj.is_null() {
        let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
        let mut node_var = VariantBuffer { data: [0u8; 48] };
        variant_from_object(node_var.data.as_mut_ptr() as *mut c_void, obj);
        let m_sn_ptr = get_cached_string_name("get_global_transform");
        let mut ret_var = VariantBuffer { data: [0u8; 48] };
        let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
        call_fn(
            node_var.data.as_mut_ptr() as *mut c_void,
            m_sn_ptr as *const c_void,
            std::ptr::null(),
            0,
            ret_var.data.as_mut_ptr() as *mut c_void,
            &mut err,
        );
        let trans = variant_to_transform3d(ret_var.data.as_ptr() as *const c_void);
        variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
        variant_destroy(node_var.data.as_mut_ptr() as *mut c_void);
        return trans;
    }
    [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0]
}

pub unsafe fn variant_to_transform3d(var: GDExtensionConstVariantPtr) -> [f32; 12] {
    let mut trans = [
        1.0f32, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
        0.0, 0.0, 0.0,
    ];
    let get_to_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_to_ctor_ptr.is_null() && !var.is_null() {
        let get_to_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_to_ctor_ptr);
        if let Some(ctor) = get_to_ctor(GDEXTENSION_VARIANT_TYPE_TRANSFORM3D) {
            ctor(trans.as_mut_ptr() as *mut c_void, var as *mut c_void);
        }
    }
    trans
}

pub unsafe fn call_object_method_ret_obj(obj: GDExtensionObjectPtr, method_name: &str) -> GDExtensionObjectPtr {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return std::ptr::null_mut();
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let res_obj = variant_to_object(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
    res_obj
}

pub unsafe fn call_object_method_ret_string(obj: GDExtensionObjectPtr, method_name: &str) -> String {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return String::new();
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let s = variant_to_rust_string(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
    s
}

pub unsafe fn node_get_script_obj(obj: GDExtensionObjectPtr) -> GDExtensionObjectPtr {
    call_object_method_ret_obj(obj, "get_script")
}

pub unsafe fn node_get_name_string(obj: GDExtensionObjectPtr) -> String {
    call_object_method_ret_string(obj, "get_name")
}

pub unsafe fn global_get_singleton(name: &str) -> GDExtensionObjectPtr {
    let fn_ptr = GLOBAL_GET_SINGLETON.load(Ordering::SeqCst);
    if fn_ptr.is_null() {
        return std::ptr::null_mut();
    }
    let f: FnGlobalGetSingleton = std::mem::transmute(fn_ptr);
    let sn = ManagedStringName::new(name);
    f(sn.as_ptr() as *const c_void)
}

pub unsafe fn engine_is_editor_hint() -> bool {
    let engine = global_get_singleton("Engine");
    if engine.is_null() {
        return false;
    }
    call_object_method_ret_bool(engine, "is_editor_hint")
}

pub unsafe fn node_is_class(obj: GDExtensionObjectPtr, class_name: &str) -> bool {
    if obj.is_null() {
        return false;
    }
    if let Some(inst) = get_rust_instance(obj) {
        if is_rust_subclass_of((*inst).class_name, class_name) {
            return true;
        }
    }
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() {
        return false;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let mut name_var = VariantBuffer { data: [0u8; 48] };
    variant_from_string(name_var.data.as_mut_ptr() as *mut c_void, class_name);

    let args = [name_var.data.as_ptr() as GDExtensionConstVariantPtr];
    static METHOD_IS_CLASS: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("is_class"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        METHOD_IS_CLASS.as_ptr() as *const c_void,
        args.as_ptr(),
        1,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let res = variant_to_bool(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(name_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn node_is_inside_tree(obj: GDExtensionObjectPtr) -> bool {
    if obj.is_null() {
        return false;
    }
    call_object_method_ret_bool(obj, "is_inside_tree")
}

pub unsafe fn node_is_ancestor_of(ancestor: GDExtensionObjectPtr, descendant: GDExtensionObjectPtr) -> bool {
    if ancestor.is_null() || descendant.is_null() {
        return false;
    }
    let mut cur = node_get_parent(descendant);
    while !cur.is_null() {
        if cur == ancestor {
            return true;
        }
        cur = node_get_parent(cur);
    }
    false
}

pub unsafe fn node_get_owner(obj: GDExtensionObjectPtr) -> GDExtensionObjectPtr {
    if obj.is_null() {
        return std::ptr::null_mut();
    }
    call_object_method_ret_obj(obj, "get_owner")
}

pub unsafe fn node_set_owner(obj: GDExtensionObjectPtr, owner: GDExtensionObjectPtr) {
    if obj.is_null() || owner.is_null() {
        return;
    }
    let mut o_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(o_var.data.as_mut_ptr() as *mut c_void, owner);
    let args = [o_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_owner", &args);
    variant_destroy(o_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node_move_child(parent: GDExtensionObjectPtr, child: GDExtensionObjectPtr, to_index: i32) {
    if parent.is_null() || child.is_null() {
        return;
    }
    let mut c_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(c_var.data.as_mut_ptr() as *mut c_void, child);
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, to_index as i64);
    let args = [
        c_var.data.as_ptr() as GDExtensionConstVariantPtr,
        idx_var.data.as_ptr() as GDExtensionConstVariantPtr,
    ];
    call_object_method_void(parent, "move_child", &args);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(c_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node_get_index(obj: GDExtensionObjectPtr) -> i32 {
    if obj.is_null() {
        return -1;
    }
    call_object_method_ret_int(obj, "get_index") as i32
}

pub unsafe fn node_set_process_mode(obj: GDExtensionObjectPtr, mode: i32) {
    if obj.is_null() {
        return;
    }
    let mut m_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(m_var.data.as_mut_ptr() as *mut c_void, mode as i64);
    let args = [m_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_process_mode", &args);
    variant_destroy(m_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node_queue_free(obj: GDExtensionObjectPtr) {
    if obj.is_null() {
        return;
    }
    call_object_method_void(obj, "queue_free", &[]);
}

pub unsafe fn node_set_meta(obj: GDExtensionObjectPtr, key: &str, meta_obj: GDExtensionObjectPtr) {
    if obj.is_null() {
        return;
    }
    let mut k_var = VariantBuffer { data: [0u8; 48] };
    variant_from_string(k_var.data.as_mut_ptr() as *mut c_void, key);
    let mut v_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(v_var.data.as_mut_ptr() as *mut c_void, meta_obj);
    let args = [
        k_var.data.as_ptr() as GDExtensionConstVariantPtr,
        v_var.data.as_ptr() as GDExtensionConstVariantPtr,
    ];
    call_object_method_void(obj, "set_meta", &args);
    variant_destroy(v_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(k_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn call_deferred(obj: GDExtensionObjectPtr, method_name: &str) {
    if obj.is_null() {
        return;
    }
    let sn_ptr = get_cached_string_name(method_name);
    let mut m_var = VariantBuffer { data: [0u8; 48] };
    variant_from_string_name(m_var.data.as_mut_ptr() as *mut c_void, sn_ptr);
    let args = [m_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "call_deferred", &args);
    variant_destroy(m_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn call_deferred_3(
    obj: GDExtensionObjectPtr,
    method_name: &str,
    a1: GDExtensionConstVariantPtr,
    a2: GDExtensionConstVariantPtr,
    a3: GDExtensionConstVariantPtr,
) {
    if obj.is_null() {
        return;
    }
    let mut m_var = VariantBuffer { data: [0u8; 48] };
    variant_from_string(m_var.data.as_mut_ptr() as *mut c_void, method_name);
    let args = [
        m_var.data.as_ptr() as GDExtensionConstVariantPtr,
        a1,
        a2,
        a3,
    ];
    call_object_method_void(obj, "call_deferred", &args);
    variant_destroy(m_var.data.as_mut_ptr() as *mut c_void);
}


pub unsafe fn call_script_method(
    obj: GDExtensionObjectPtr,
    method_name: &str,
    args: &[GDExtensionConstVariantPtr],
) -> bool {
    let has_method_ptr = OBJECT_HAS_SCRIPT_METHOD.load(Ordering::SeqCst);
    let call_method_ptr = OBJECT_CALL_SCRIPT_METHOD.load(Ordering::SeqCst);
    if has_method_ptr.is_null() || call_method_ptr.is_null() || obj.is_null() {
        return false;
    }
    let has_method_fn: FnObjectHasScriptMethod = std::mem::transmute(has_method_ptr);
    let call_method_fn: FnObjectCallScriptMethod = std::mem::transmute(call_method_ptr);

    let m_sn_ptr = get_cached_string_name(method_name);
    if has_method_fn(obj, m_sn_ptr) == 0 {
        return false;
    }

    let mut ret_var = [0u8; 48];
    let mut call_err = GDExtensionCallError {
        error: 0,
        argument: 0,
        expected: 0,
    };

    call_method_fn(
        obj,
        m_sn_ptr,
        if args.is_empty() { std::ptr::null() } else { args.as_ptr() },
        args.len() as i64,
        ret_var.as_mut_ptr() as *mut c_void,
        &mut call_err,
    );

    let destroy_ptr = VARIANT_DESTROY.load(Ordering::SeqCst);
    if !destroy_ptr.is_null() {
        let destroy_fn: FnVariantDestroy = std::mem::transmute(destroy_ptr);
        destroy_fn(ret_var.as_mut_ptr() as *mut c_void);
    }

    true
}

pub unsafe fn call_virtual_method_0(
    obj: GDExtensionObjectPtr,
    method_name: &str,
) -> bool {
    call_script_method(obj, method_name, &[])
}

pub unsafe fn call_virtual_method_1_float(
    obj: GDExtensionObjectPtr,
    method_name: &str,
    val: f64,
) -> bool {
    let mut float_var = [0u8; 48];
    variant_from_float(float_var.as_mut_ptr() as *mut c_void, val);
    let arg_ptr = float_var.as_ptr() as GDExtensionConstVariantPtr;
    let args = [arg_ptr];
    let res = call_script_method(obj, method_name, &args);
    variant_destroy(float_var.as_mut_ptr() as *mut c_void);
    res
}


pub unsafe extern "C" fn rust_class_notification(
    instance: GDExtensionClassInstancePtr,
    what: i32,
    reversed: GDExtensionBool,
) {
    if instance.is_null() {
        return;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    if rust_inst.godot_object.is_null() {
        return;
    }

    let class_name = rust_inst.class_name;
    let rev_key = if reversed != 0 { 1 } else { 0 };

    let mut handled_from_cache = false;
    if let Ok(cache) = NOTIFICATION_CHAINS.read() {
        if let Some(chain) = cache.get(&(class_name, rev_key)) {
            for handler in chain.iter() {
                handler(rust_inst, what);
            }
            handled_from_cache = true;
        }
    }

    if !handled_from_cache {
        let mut chain: Vec<fn(&RustNodeInstance, i32)> = Vec::new();
        if let (Ok(parents), Ok(handlers)) = (CLASS_PARENTS.lock(), CLASS_HANDLERS.lock()) {
            let mut cur = class_name;
            let mut depth = 0;
            loop {
                if let Some(h) = handlers.get(cur) {
                    chain.push(h.on_notification);
                }
                match parents.get(cur) {
                    Some(&p) => {
                        cur = p;
                        depth += 1;
                        if depth > 32 {
                            break;
                        }
                    }
                    None => break,
                }
            }
        }

        let final_chain: Vec<fn(&RustNodeInstance, i32)> = if rev_key == 0 {
            chain.into_iter().rev().collect()
        } else {
            chain
        };

        for handler in final_chain.iter() {
            handler(rust_inst, what);
        }

        if let Ok(mut cache) = NOTIFICATION_CHAINS.write() {
            cache.insert((class_name, rev_key), final_chain);
        }
    }

    if HAS_NOTIFICATION_HOOKS.load(Ordering::Relaxed) {
        if let Ok(hooks) = NOTIFICATION_HOOKS.lock() {
            for hook in hooks.iter() {
                hook(rust_inst, what);
            }
        }
    }
}


pub unsafe fn variant_destroy(var: GDExtensionVariantPtr) {
    if !var.is_null() {
        let destroy_ptr = VARIANT_DESTROY.load(Ordering::SeqCst);
        if !destroy_ptr.is_null() {
            let destroy_fn: FnVariantDestroy = std::mem::transmute(destroy_ptr);
            destroy_fn(var);
        }
        std::ptr::write_bytes(var as *mut u8, 0, 48);
    }
}

pub unsafe fn variant_to_rust_string(var: GDExtensionConstVariantPtr) -> String {
    if var.is_null() {
        return String::new();
    }
    let stringify_ptr = VARIANT_STRINGIFY.load(Ordering::SeqCst);
    let to_utf8_ptr = STRING_TO_UTF8_CHARS.load(Ordering::SeqCst);
    if stringify_ptr.is_null() || to_utf8_ptr.is_null() {
        return String::new();
    }
    let stringify_fn: FnVariantStringify = std::mem::transmute(stringify_ptr);
    let to_utf8_fn: FnStringToUtf8Chars = std::mem::transmute(to_utf8_ptr);

    let mut godot_str: usize = 0;
    stringify_fn(var, &mut godot_str as *mut _ as *mut c_void);

    let len = to_utf8_fn(&godot_str as *const _ as *const c_void, std::ptr::null_mut(), 0);
    if len <= 0 {
        let destr_ptr = STRING_DESTRUCTOR.load(Ordering::SeqCst);
        if !destr_ptr.is_null() {
            let destr_fn: unsafe extern "C" fn(*mut c_void) = std::mem::transmute(destr_ptr);
            destr_fn(&mut godot_str as *mut _ as *mut c_void);
        }
        return String::new();
    }

    let mut buf = vec![0u8; len as usize];
    to_utf8_fn(&godot_str as *const _ as *const c_void, buf.as_mut_ptr() as *mut c_char, len);

    let destr_ptr = STRING_DESTRUCTOR.load(Ordering::SeqCst);
    if !destr_ptr.is_null() {
        let destr_fn: unsafe extern "C" fn(*mut c_void) = std::mem::transmute(destr_ptr);
        destr_fn(&mut godot_str as *mut _ as *mut c_void);
    }

    String::from_utf8_lossy(&buf).into_owned()
}

pub unsafe fn variant_from_string_name(r_dest: GDExtensionVariantPtr, sn: GDExtensionConstStringNamePtr) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_STRING_NAME) {
            ctor(r_dest, sn);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn string_name_to_string(sn: GDExtensionConstStringNamePtr) -> String {
    if sn.is_null() {
        return String::new();
    }
    let mut vb = VariantBuffer { data: [0u8; 48] };
    variant_from_string_name(vb.data.as_mut_ptr() as *mut c_void, sn);
    let s = variant_to_rust_string(vb.data.as_ptr() as *const c_void);
    variant_destroy(vb.data.as_mut_ptr() as *mut c_void);
    s
}

pub unsafe fn variant_to_basis(var: GDExtensionConstVariantPtr) -> [f32; 9] {
    let mut basis = [
        1.0f32, 0.0, 0.0,
        0.0, 1.0, 0.0,
        0.0, 0.0, 1.0,
    ];
    let get_to_ctor_ptr = GET_TO_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_to_ctor_ptr.is_null() && !var.is_null() {
        let get_to_ctor: FnGetVariantToTypeConstructor = std::mem::transmute(get_to_ctor_ptr);
        if let Some(ctor) = get_to_ctor(GDEXTENSION_VARIANT_TYPE_BASIS) {
            ctor(basis.as_mut_ptr() as *mut c_void, var as *mut c_void);
        }
    }
    basis
}

pub unsafe fn variant_from_basis(r_dest: GDExtensionVariantPtr, basis: &[f32; 9]) {
    let get_from_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_from_ctor_ptr.is_null() {
        let get_from_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_from_ctor_ptr);
        if let Some(ctor) = get_from_ctor(GDEXTENSION_VARIANT_TYPE_BASIS) {
            ctor(r_dest, basis.as_ptr() as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

// ============================================================
// PhysicsDirectBodyState3D FFI Helpers
// ============================================================

pub unsafe fn state_get_transform(state_obj: GDExtensionObjectPtr) -> [f32; 12] {
    let mb = METHOD_STATE_GET_TRANSFORM.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut trans = [
            1.0f32, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            0.0, 0.0, 0.0,
        ];
        ptrcall_fn(mb as *mut c_void, state_obj, std::ptr::null(), trans.as_mut_ptr() as *mut c_void);
        return trans;
    }
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_transform", &[], ret_var.data.as_mut_ptr() as *mut c_void);
    let trans = variant_to_transform3d(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    trans
}

pub unsafe fn state_set_transform(state_obj: GDExtensionObjectPtr, trans: &[f32; 12]) {
    let mb = METHOD_STATE_SET_TRANSFORM.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [trans.as_ptr() as *const c_void];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut trans_var = VariantBuffer { data: [0u8; 48] };
    variant_from_transform3d(trans_var.data.as_mut_ptr() as *mut c_void, trans);
    let args = [trans_var.data.as_ptr() as *const c_void];
    call_object_method_void(state_obj, "set_transform", &args);
    variant_destroy(trans_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn state_get_linear_velocity(state_obj: GDExtensionObjectPtr) -> [f32; 3] {
    let mb = METHOD_STATE_GET_LINEAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, std::ptr::null(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    call_object_method_ret_vector3(state_obj, "get_linear_velocity")
}

pub unsafe fn state_set_linear_velocity(state_obj: GDExtensionObjectPtr, vel: [f32; 3]) {
    let mb = METHOD_STATE_SET_LINEAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&vel as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut vel_var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(vel_var.data.as_mut_ptr() as *mut c_void, vel);
    let args = [vel_var.data.as_ptr() as *const c_void];
    call_object_method_void(state_obj, "set_linear_velocity", &args);
    variant_destroy(vel_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn state_get_angular_velocity(state_obj: GDExtensionObjectPtr) -> [f32; 3] {
    let mb = METHOD_STATE_GET_ANGULAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, std::ptr::null(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    call_object_method_ret_vector3(state_obj, "get_angular_velocity")
}

pub unsafe fn state_set_angular_velocity(state_obj: GDExtensionObjectPtr, vel: [f32; 3]) {
    let mb = METHOD_STATE_SET_ANGULAR_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&vel as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut vel_var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(vel_var.data.as_mut_ptr() as *mut c_void, vel);
    let args = [vel_var.data.as_ptr() as *const c_void];
    call_object_method_void(state_obj, "set_angular_velocity", &args);
    variant_destroy(vel_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn state_get_contact_count(state_obj: GDExtensionObjectPtr) -> i32 {
    let mb = METHOD_STATE_GET_CONTACT_COUNT.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret: i32 = 0;
        ptrcall_fn(mb as *mut c_void, state_obj, std::ptr::null(), &mut ret as *mut i32 as *mut c_void);
        return ret;
    }
    call_object_method_ret_int(state_obj, "get_contact_count") as i32
}

pub unsafe fn state_get_contact_local_shape(state_obj: GDExtensionObjectPtr, idx: i32) -> i32 {
    let mb = METHOD_STATE_GET_CONTACT_LOCAL_SHAPE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&idx as *const i32 as *const c_void];
        let mut ret: i32 = 0;
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), &mut ret as *mut i32 as *mut c_void);
        return ret;
    }
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx as i64);
    let args = [idx_var.data.as_ptr() as *const c_void];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_contact_local_shape", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let shape = variant_to_int(ret_var.data.as_ptr() as *const c_void) as i32;
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    shape
}

pub unsafe fn state_get_contact_impulse(state_obj: GDExtensionObjectPtr, idx: i32) -> [f32; 3] {
    let mb = METHOD_STATE_GET_CONTACT_IMPULSE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&idx as *const i32 as *const c_void];
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx as i64);
    let args = [idx_var.data.as_ptr() as *const c_void];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_contact_impulse", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let vec = variant_to_vector3(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    vec
}

pub unsafe fn state_get_contact_local_position(state_obj: GDExtensionObjectPtr, idx: i32) -> [f32; 3] {
    let mb = METHOD_STATE_GET_CONTACT_LOCAL_POSITION.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&idx as *const i32 as *const c_void];
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx as i64);
    let args = [idx_var.data.as_ptr() as *const c_void];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_contact_local_position", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let vec = variant_to_vector3(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    vec
}

pub unsafe fn state_get_contact_local_normal(state_obj: GDExtensionObjectPtr, idx: i32) -> [f32; 3] {
    let mb = METHOD_STATE_GET_CONTACT_LOCAL_NORMAL.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&idx as *const i32 as *const c_void];
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx as i64);
    let args = [idx_var.data.as_ptr() as *const c_void];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_contact_local_normal", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let vec = variant_to_vector3(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    vec
}

pub unsafe fn state_get_contact_collider_position(state_obj: GDExtensionObjectPtr, idx: i32) -> [f32; 3] {
    let mb = METHOD_STATE_GET_CONTACT_COLLIDER_POSITION.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&idx as *const i32 as *const c_void];
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx as i64);
    let args = [idx_var.data.as_ptr() as *const c_void];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_contact_collider_position", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let vec = variant_to_vector3(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    vec
}

pub unsafe fn state_get_contact_collider_velocity_at_position(state_obj: GDExtensionObjectPtr, idx: i32) -> [f32; 3] {
    let mb = METHOD_STATE_GET_CONTACT_COLLIDER_VELOCITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&idx as *const i32 as *const c_void];
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx as i64);
    let args = [idx_var.data.as_ptr() as *const c_void];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_contact_collider_velocity_at_position", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let vec = variant_to_vector3(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    vec
}

pub unsafe fn state_get_contact_collider_object(state_obj: GDExtensionObjectPtr, idx: i32) -> GDExtensionObjectPtr {
    let mb = METHOD_STATE_GET_CONTACT_COLLIDER_OBJECT.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&idx as *const i32 as *const c_void];
        let mut ret: GDExtensionObjectPtr = std::ptr::null_mut();
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), &mut ret as *mut _ as *mut c_void);
        return ret;
    }
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx as i64);
    let args = [idx_var.data.as_ptr() as *const c_void];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_contact_collider_object", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let obj = variant_to_object(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    obj
}

pub unsafe fn state_apply_central_impulse(state_obj: GDExtensionObjectPtr, impulse: [f32; 3]) {
    let mb = METHOD_STATE_APPLY_CENTRAL_IMPULSE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&impulse as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(var.data.as_mut_ptr() as *mut c_void, impulse);
    let args = [var.data.as_ptr() as *const c_void];
    call_object_method_void(state_obj, "apply_central_impulse", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn state_apply_central_force(state_obj: GDExtensionObjectPtr, force: [f32; 3]) {
    let mb = METHOD_STATE_APPLY_CENTRAL_FORCE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&force as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(var.data.as_mut_ptr() as *mut c_void, force);
    let args = [var.data.as_ptr() as *const c_void];
    call_object_method_void(state_obj, "apply_central_force", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn state_apply_torque(state_obj: GDExtensionObjectPtr, torque: [f32; 3]) {
    let mb = METHOD_STATE_APPLY_TORQUE.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [&torque as *const _ as *const c_void];
        ptrcall_fn(mb as *mut c_void, state_obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(var.data.as_mut_ptr() as *mut c_void, torque);
    let args = [var.data.as_ptr() as *const c_void];
    call_object_method_void(state_obj, "apply_torque", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn state_get_inverse_inertia_tensor(state_obj: GDExtensionObjectPtr) -> [f32; 9] {
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(state_obj, "get_inverse_inertia_tensor", &[], ret_var.data.as_mut_ptr() as *mut c_void);
    let b = variant_to_basis(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    b
}

pub unsafe fn state_get_total_gravity(state_obj: GDExtensionObjectPtr) -> [f32; 3] {
    let mb = METHOD_STATE_GET_TOTAL_GRAVITY.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, state_obj, std::ptr::null(), ret.as_mut_ptr() as *mut c_void);
        return ret;
    }
    call_object_method_ret_vector3(state_obj, "get_total_gravity")
}

pub unsafe fn state_get_inverse_mass(state_obj: GDExtensionObjectPtr) -> f32 {
    let mb = METHOD_STATE_GET_INVERSE_MASS.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret: f32 = 0.0;
        ptrcall_fn(mb as *mut c_void, state_obj, std::ptr::null(), &mut ret as *mut _ as *mut c_void);
        return ret;
    }
    call_object_method_ret_float(state_obj, "get_inverse_mass") as f32
}

pub unsafe fn state_get_step(state_obj: GDExtensionObjectPtr) -> f64 {
    let mb = METHOD_STATE_GET_STEP.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !state_obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut ret: f32 = 0.0;
        ptrcall_fn(mb as *mut c_void, state_obj, std::ptr::null(), &mut ret as *mut _ as *mut c_void);
        return ret as f64;
    }
    call_object_method_ret_float(state_obj, "get_step")
}

pub unsafe fn array_append(arr: GDExtensionVariantPtr, item: GDExtensionConstVariantPtr) {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || arr.is_null() || item.is_null() {
        return;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    static METHOD_APPEND: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("append"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    let args = [item];
    call_fn(
        arr,
        METHOD_APPEND.as_ptr() as *const c_void,
        args.as_ptr(),
        1,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn dictionary_set(dict: GDExtensionVariantPtr, key: &str, val: GDExtensionConstVariantPtr) {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || dict.is_null() || val.is_null() {
        return;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut key_var = VariantBuffer { data: [0u8; 48] };
    variant_from_string(key_var.data.as_mut_ptr() as *mut c_void, key);
    let args = [
        key_var.data.as_ptr() as GDExtensionConstVariantPtr,
        val,
    ];
    let m_sn_ptr = get_cached_string_name("set");
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    call_fn(
        dict,
        m_sn_ptr as *const c_void,
        args.as_ptr(),
        2,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(key_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn object_get_meta(obj: GDExtensionObjectPtr, name: &str) -> GDExtensionObjectPtr {
    if obj.is_null() {
        return std::ptr::null_mut();
    }
    let mut name_var = VariantBuffer { data: [0u8; 48] };
    let sn = ManagedStringName::new(name);
    variant_from_string_name(name_var.data.as_mut_ptr() as *mut c_void, sn.as_ptr());
    let args = [name_var.data.as_ptr() as GDExtensionConstVariantPtr];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(obj, "get_meta", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let res = variant_to_object(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(name_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn object_set_meta(obj: GDExtensionObjectPtr, name: &str, val: GDExtensionObjectPtr) {
    if obj.is_null() {
        return;
    }
    let mut name_var = VariantBuffer { data: [0u8; 48] };
    let sn = ManagedStringName::new(name);
    variant_from_string_name(name_var.data.as_mut_ptr() as *mut c_void, sn.as_ptr());
    let mut val_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(val_var.data.as_mut_ptr() as *mut c_void, val);
    let args = [
        name_var.data.as_ptr() as GDExtensionConstVariantPtr,
        val_var.data.as_ptr() as GDExtensionConstVariantPtr,
    ];
    call_object_method_void(obj, "set_meta", &args);
    variant_destroy(val_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(name_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn collision_object_shape_find_owner(obj: GDExtensionObjectPtr, shape_index: i32) -> u32 {
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, shape_index as i64);
    let args = [idx_var.data.as_ptr() as GDExtensionConstVariantPtr];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(obj, "shape_find_owner", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let owner_id = variant_to_int(ret_var.data.as_ptr() as *const c_void) as u32;
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    owner_id
}

pub unsafe fn collision_object_shape_owner_get_owner(obj: GDExtensionObjectPtr, owner_id: u32) -> GDExtensionObjectPtr {
    let mut id_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(id_var.data.as_mut_ptr() as *mut c_void, owner_id as i64);
    let args = [id_var.data.as_ptr() as GDExtensionConstVariantPtr];
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    call_object_method_ret_var(obj, "shape_owner_get_owner", &args, ret_var.data.as_mut_ptr() as *mut c_void);
    let owner_obj = variant_to_object(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(id_var.data.as_mut_ptr() as *mut c_void);
    owner_obj
}

pub unsafe fn object_emit_signal_0(obj: GDExtensionObjectPtr, signal_name: &str) {
    if obj.is_null() {
        return;
    }
    let mut sig_var = VariantBuffer { data: [0u8; 48] };
    let sn = ManagedStringName::new(signal_name);
    variant_from_string_name(sig_var.data.as_mut_ptr() as *mut c_void, sn.as_ptr());
    let args = [sig_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "emit_signal", &args);
    variant_destroy(sig_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn object_emit_signal_1(obj: GDExtensionObjectPtr, signal_name: &str, arg1: GDExtensionConstVariantPtr) {
    if obj.is_null() {
        return;
    }
    let mut sig_var = VariantBuffer { data: [0u8; 48] };
    let sn = ManagedStringName::new(signal_name);
    variant_from_string_name(sig_var.data.as_mut_ptr() as *mut c_void, sn.as_ptr());
    let args = [
        sig_var.data.as_ptr() as GDExtensionConstVariantPtr,
        arg1,
    ];
    call_object_method_void(obj, "emit_signal", &args);
    variant_destroy(sig_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn object_emit_signal_2(
    obj: GDExtensionObjectPtr,
    signal_name: &str,
    arg1: GDExtensionObjectPtr,
    arg2: GDExtensionObjectPtr,
) {
    if obj.is_null() {
        return;
    }
    let mut sig_var = VariantBuffer { data: [0u8; 48] };
    let sn = ManagedStringName::new(signal_name);
    variant_from_string_name(sig_var.data.as_mut_ptr() as *mut c_void, sn.as_ptr());

    let mut v1 = VariantBuffer { data: [0u8; 48] };
    variant_from_object(v1.data.as_mut_ptr() as *mut c_void, arg1);
    let mut v2 = VariantBuffer { data: [0u8; 48] };
    variant_from_object(v2.data.as_mut_ptr() as *mut c_void, arg2);

    let args = [
        sig_var.data.as_ptr() as GDExtensionConstVariantPtr,
        v1.data.as_ptr() as GDExtensionConstVariantPtr,
        v2.data.as_ptr() as GDExtensionConstVariantPtr,
    ];
    call_object_method_void(obj, "emit_signal", &args);
    variant_destroy(v2.data.as_mut_ptr() as *mut c_void);
    variant_destroy(v1.data.as_mut_ptr() as *mut c_void);
    variant_destroy(sig_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn object_emit_signal_4(
    obj: GDExtensionObjectPtr,
    signal_name: &str,
    arg1: GDExtensionObjectPtr,
    arg2: GDExtensionObjectPtr,
    arg3: GDExtensionObjectPtr,
    arg4: GDExtensionObjectPtr,
) {
    if obj.is_null() {
        return;
    }
    let mut sig_var = VariantBuffer { data: [0u8; 48] };
    let sn = ManagedStringName::new(signal_name);
    variant_from_string_name(sig_var.data.as_mut_ptr() as *mut c_void, sn.as_ptr());

    let mut v1 = VariantBuffer { data: [0u8; 48] };
    variant_from_object(v1.data.as_mut_ptr() as *mut c_void, arg1);
    let mut v2 = VariantBuffer { data: [0u8; 48] };
    variant_from_object(v2.data.as_mut_ptr() as *mut c_void, arg2);
    let mut v3 = VariantBuffer { data: [0u8; 48] };
    variant_from_object(v3.data.as_mut_ptr() as *mut c_void, arg3);
    let mut v4 = VariantBuffer { data: [0u8; 48] };
    variant_from_object(v4.data.as_mut_ptr() as *mut c_void, arg4);

    let args = [
        sig_var.data.as_ptr() as GDExtensionConstVariantPtr,
        v1.data.as_ptr() as GDExtensionConstVariantPtr,
        v2.data.as_ptr() as GDExtensionConstVariantPtr,
        v3.data.as_ptr() as GDExtensionConstVariantPtr,
        v4.data.as_ptr() as GDExtensionConstVariantPtr,
    ];
    call_object_method_void(obj, "emit_signal", &args);
    variant_destroy(v4.data.as_mut_ptr() as *mut c_void);
    variant_destroy(v3.data.as_mut_ptr() as *mut c_void);
    variant_destroy(v2.data.as_mut_ptr() as *mut c_void);
    variant_destroy(v1.data.as_mut_ptr() as *mut c_void);
    variant_destroy(sig_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node3d_set_notify_transform(obj: GDExtensionObjectPtr, enable: bool) {
    if obj.is_null() {
        return;
    }
    let mb = METHOD_NODE3D_SET_NOTIFY_TRANSFORM.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let arg: u8 = if enable { 1 } else { 0 };
        let args = [&arg as *const u8 as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut b_var = VariantBuffer { data: [0u8; 48] };
    variant_from_bool(b_var.data.as_mut_ptr() as *mut c_void, enable);
    let args = [b_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_notify_transform", &args);
    variant_destroy(b_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node_get_child_count(node_obj: GDExtensionObjectPtr) -> i64 {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || node_obj.is_null() {
        return 0;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut node_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(node_var.data.as_mut_ptr() as *mut c_void, node_obj);

    static METHOD_GET_CHILD_COUNT: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("get_child_count"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        node_var.data.as_mut_ptr() as *mut c_void,
        METHOD_GET_CHILD_COUNT.as_ptr() as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let count = variant_to_int(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(node_var.data.as_mut_ptr() as *mut c_void);
    count
}

pub unsafe fn node_get_child(node_obj: GDExtensionObjectPtr, idx: i64) -> GDExtensionObjectPtr {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || node_obj.is_null() {
        return std::ptr::null_mut();
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut node_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(node_var.data.as_mut_ptr() as *mut c_void, node_obj);

    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, idx);

    let args = [idx_var.data.as_ptr() as GDExtensionConstVariantPtr];
    static METHOD_GET_CHILD: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("get_child"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        node_var.data.as_mut_ptr() as *mut c_void,
        METHOD_GET_CHILD.as_ptr() as *const c_void,
        args.as_ptr(),
        1,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let child_obj = variant_to_object(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(node_var.data.as_mut_ptr() as *mut c_void);
    child_obj
}

pub unsafe fn node_get_node_or_null(node_obj: GDExtensionObjectPtr, path_var: GDExtensionConstVariantPtr) -> GDExtensionObjectPtr {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || node_obj.is_null() || path_var.is_null() {
        return std::ptr::null_mut();
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut node_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(node_var.data.as_mut_ptr() as *mut c_void, node_obj);

    static METHOD_GET_NODE_OR_NULL: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("get_node_or_null"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    let args = [path_var];

    call_fn(
        node_var.data.as_mut_ptr() as *mut c_void,
        METHOD_GET_NODE_OR_NULL.as_ptr() as *const c_void,
        args.as_ptr(),
        1,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let resolved_obj = variant_to_object(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(node_var.data.as_mut_ptr() as *mut c_void);
    resolved_obj
}

pub unsafe fn node_get_node_or_null_str(node_obj: GDExtensionObjectPtr, path_str: &str) -> GDExtensionObjectPtr {
    if node_obj.is_null() || path_str.is_empty() {
        return std::ptr::null_mut();
    }
    let mut str_var = VariantBuffer { data: [0u8; 48] };
    let s = path_str.to_string();
    variant_from_string(str_var.data.as_mut_ptr() as *mut c_void, &s);
    let res = node_get_node_or_null(node_obj, str_var.data.as_ptr() as GDExtensionConstVariantPtr);
    variant_destroy(str_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn construct_object(class_name: &str) -> GDExtensionObjectPtr {
    let construct_ptr = CLASSDB_CONSTRUCT_OBJECT.load(Ordering::SeqCst);
    if construct_ptr.is_null() {
        return std::ptr::null_mut();
    }
    let construct_fn: FnClassdbConstructObject = std::mem::transmute(construct_ptr);
    let sn = ManagedStringName::new(class_name);
    construct_fn(sn.as_ptr())
}

/// An RAII guard holding an active Variant reference to a Godot `RefCounted` / `Resource` object.
///
/// ### Why this is critical:
/// In Godot, newly constructed `RefCounted` instances (shapes, queries, meshes, materials, etc.)
/// have an unreferenced count (0). Any GDExtension method call (`call_object_method_*`) internally
/// wraps the target in a temporary Variant (refcount -> 1) and destroys it on return (refcount -> 0),
/// which immediately calls `memdelete(this)`!
///
/// Holding `RefCountedGuard` ensures the object stays alive (refcount >= 1) until transferred to
/// an owning node or until the guard drops.
pub struct RefCountedGuard {
    pub object: GDExtensionObjectPtr,
    pub variant: VariantBuffer,
}

impl RefCountedGuard {
    pub fn new(obj: GDExtensionObjectPtr) -> Self {
        let mut vb = VariantBuffer { data: [0u8; 48] };
        if !obj.is_null() {
            unsafe {
                variant_from_object(vb.data.as_mut_ptr() as *mut c_void, obj);
            }
        }
        Self { object: obj, variant: vb }
    }
}

/// Constructs a new `RefCounted` / `Resource` object and returns it wrapped in a protective `RefCountedGuard`.
/// Returns `None` if construction failed.
pub unsafe fn construct_ref_counted(class_name: &str) -> Option<RefCountedGuard> {
    let obj = construct_object(class_name);
    if obj.is_null() {
        None
    } else {
        Some(RefCountedGuard::new(obj))
    }
}

pub unsafe fn node_add_child(parent_obj: GDExtensionObjectPtr, child_obj: GDExtensionObjectPtr) {
    node_add_child_obj(parent_obj, child_obj);
}

pub unsafe fn node_remove_child(parent_obj: GDExtensionObjectPtr, child_obj: GDExtensionObjectPtr) {
    node_remove_child_obj(parent_obj, child_obj);
}

pub unsafe fn call_object_method_void(
    obj: GDExtensionObjectPtr,
    method_name: &str,
    args: &[GDExtensionConstVariantPtr],
) {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        if args.is_empty() { std::ptr::null() } else { args.as_ptr() },
        args.len() as i64,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn call_object_method_ret_var(
    obj: GDExtensionObjectPtr,
    method_name: &str,
    args: &[GDExtensionConstVariantPtr],
    ret: GDExtensionVariantPtr,
) {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() || ret.is_null() {
        if !ret.is_null() {
            variant_new_nil(ret);
        }
        return;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        if args.is_empty() { std::ptr::null() } else { args.as_ptr() },
        args.len() as i64,
        ret,
        &mut err,
    );

    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn call_object_method_ret_int(obj: GDExtensionObjectPtr, method_name: &str) -> i64 {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return 0;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let res = variant_to_int(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn call_object_method_ret_bool(obj: GDExtensionObjectPtr, method_name: &str) -> bool {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return false;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    let m_sn_ptr = get_cached_string_name(method_name);
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        m_sn_ptr as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let res = variant_to_bool(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn call_object_callv(
    obj: GDExtensionObjectPtr,
    method_name_var: GDExtensionConstVariantPtr,
    args_array_var: GDExtensionConstVariantPtr,
) {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut obj_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(obj_var.data.as_mut_ptr() as *mut c_void, obj);

    static METHOD_CALLV: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("callv"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    let args = [method_name_var, args_array_var];

    call_fn(
        obj_var.data.as_mut_ptr() as *mut c_void,
        METHOD_CALLV.as_ptr() as *const c_void,
        args.as_ptr(),
        2,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(obj_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn array_size(arr: GDExtensionConstVariantPtr) -> i64 {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || arr.is_null() {
        return 0;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    static METHOD_SIZE: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("size"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    call_fn(
        arr as *mut c_void,
        METHOD_SIZE.as_ptr() as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );
    let res = variant_to_int(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn array_has(arr: GDExtensionConstVariantPtr, item: GDExtensionConstVariantPtr) -> bool {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || arr.is_null() || item.is_null() {
        return false;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    static METHOD_HAS: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("has"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    let args = [item];
    call_fn(
        arr as *mut c_void,
        METHOD_HAS.as_ptr() as *const c_void,
        args.as_ptr(),
        1,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );
    let res = variant_to_bool(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    res
}

pub unsafe fn array_get(arr: GDExtensionConstVariantPtr, index: i64, ret: GDExtensionVariantPtr) -> bool {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || arr.is_null() || ret.is_null() {
        return false;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    static METHOD_GET: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("get"));
    let mut idx_var = VariantBuffer { data: [0u8; 48] };
    variant_from_int(idx_var.data.as_mut_ptr() as *mut c_void, index);
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    let args = [idx_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_fn(
        arr as *mut c_void,
        METHOD_GET.as_ptr() as *const c_void,
        args.as_ptr(),
        1,
        ret,
        &mut err,
    );
    variant_destroy(idx_var.data.as_mut_ptr() as *mut c_void);
    err.error == 0
}

pub unsafe fn variant_array_to_object_ptrs(arr: GDExtensionConstVariantPtr) -> Vec<GDExtensionObjectPtr> {
    let mut vec = Vec::new();
    let sz = array_size(arr);
    for i in 0..sz {
        let mut item_var = VariantBuffer { data: [0u8; 48] };
        if array_get(arr, i, item_var.data.as_mut_ptr() as *mut c_void) {
            let obj = variant_to_object(item_var.data.as_ptr() as *const c_void);
            if !obj.is_null() {
                vec.push(obj);
            }
            variant_destroy(item_var.data.as_mut_ptr() as *mut c_void);
        }
    }
    vec
}

pub unsafe fn variant_from_transform3d(r_dest: GDExtensionVariantPtr, trans: &[f32; 12]) {
    let get_ctor_ptr = GET_FROM_TYPE_CTOR.load(Ordering::SeqCst);
    if !get_ctor_ptr.is_null() {
        let get_ctor: FnGetVariantFromTypeConstructor = std::mem::transmute(get_ctor_ptr);
        if let Some(ctor) = get_ctor(GDEXTENSION_VARIANT_TYPE_TRANSFORM3D) {
            ctor(r_dest, trans.as_ptr() as *const c_void);
            return;
        }
    }
    variant_new_nil(r_dest);
}

pub unsafe fn node3d_set_global_transform(obj: GDExtensionObjectPtr, trans: &[f32; 12]) {
    let mb = METHOD_NODE3D_SET_GLOBAL_TRANSFORM.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [trans.as_ptr() as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut trans_var = VariantBuffer { data: [0u8; 48] };
    variant_from_transform3d(trans_var.data.as_mut_ptr() as *mut c_void, trans);
    let args = [trans_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_global_transform", &args);
    variant_destroy(trans_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node3d_set_global_position(obj: GDExtensionObjectPtr, pos: &[f32; 3]) {
    let mb = METHOD_NODE3D_SET_GLOBAL_POSITION.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [pos.as_ptr() as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    if obj.is_null() {
        return;
    }
    let mut var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(var.data.as_mut_ptr() as *mut c_void, *pos);
    let args = [var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_global_position", &args);
    variant_destroy(var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node3d_get_global_position(obj: GDExtensionObjectPtr) -> [f32; 3] {
    let mb = METHOD_NODE3D_GET_GLOBAL_POSITION.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut pos = [0.0f32; 3];
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), pos.as_mut_ptr() as *mut c_void);
        return pos;
    }
    let trans = node3d_get_global_transform(obj);
    [trans[9], trans[10], trans[11]]
}

pub unsafe fn node3d_get_transform(obj: GDExtensionObjectPtr) -> [f32; 12] {
    let mb = METHOD_NODE3D_GET_TRANSFORM.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let mut trans = [
            1.0f32, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
            0.0, 0.0, 0.0,
        ];
        ptrcall_fn(mb as *mut c_void, obj, std::ptr::null(), trans.as_mut_ptr() as *mut c_void);
        return trans;
    }
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if !call_ptr.is_null() && !obj.is_null() {
        let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
        let mut node_var = VariantBuffer { data: [0u8; 48] };
        variant_from_object(node_var.data.as_mut_ptr() as *mut c_void, obj);
        let m_sn_ptr = get_cached_string_name("get_transform");
        let mut ret_var = VariantBuffer { data: [0u8; 48] };
        let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
        call_fn(
            node_var.data.as_mut_ptr() as *mut c_void,
            m_sn_ptr as *const c_void,
            std::ptr::null(),
            0,
            ret_var.data.as_mut_ptr() as *mut c_void,
            &mut err,
        );
        let trans = variant_to_transform3d(ret_var.data.as_ptr() as *const c_void);
        variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
        variant_destroy(node_var.data.as_mut_ptr() as *mut c_void);
        return trans;
    }
    [1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0]
}

pub unsafe fn node3d_set_transform(obj: GDExtensionObjectPtr, trans: &[f32; 12]) {
    let mb = METHOD_NODE3D_SET_TRANSFORM.load(Ordering::Relaxed);
    let ptrcall = OBJECT_METHOD_BIND_PTRCALL.load(Ordering::Relaxed);
    if !mb.is_null() && !ptrcall.is_null() && !obj.is_null() {
        let ptrcall_fn: FnObjectMethodBindPtrcall = std::mem::transmute(ptrcall);
        let args = [trans.as_ptr() as *const c_void];
        ptrcall_fn(mb as *mut c_void, obj, args.as_ptr(), std::ptr::null_mut());
        return;
    }
    let mut trans_var = VariantBuffer { data: [0u8; 48] };
    variant_from_transform3d(trans_var.data.as_mut_ptr() as *mut c_void, trans);
    let args = [trans_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_transform", &args);
    variant_destroy(trans_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node3d_set_scale(obj: GDExtensionObjectPtr, scale: [f32; 3]) {
    let mut vec_var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(vec_var.data.as_mut_ptr() as *mut c_void, scale);
    let args = [vec_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(obj, "set_scale", &args);
    variant_destroy(vec_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node3d_get_rotation_degrees(obj: GDExtensionObjectPtr) -> [f32; 3] {
    call_object_method_ret_vector3(obj, "get_rotation_degrees")
}

pub unsafe fn node3d_set_rotation_degrees(obj: GDExtensionObjectPtr, rot: &[f32; 3]) {
    let mut rot_var = VariantBuffer { data: [0u8; 48] };
    variant_from_vector3(rot_var.data.as_mut_ptr() as *mut c_void, *rot);
    let args = [rot_var.data.as_ptr() as *const c_void];
    call_object_method_void(obj, "set_rotation_degrees", &args);
    variant_destroy(rot_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node3d_get_rotation_degrees_y(obj: GDExtensionObjectPtr) -> f32 {
    let call_ptr = VARIANT_CALL.load(Ordering::SeqCst);
    if call_ptr.is_null() || obj.is_null() {
        return 0.0;
    }
    let call_fn: FnVariantCall = std::mem::transmute(call_ptr);
    let mut node_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(node_var.data.as_mut_ptr() as *mut c_void, obj);

    static METHOD_GET_ROT: LazyLock<ManagedStringName> = LazyLock::new(|| ManagedStringName::new("get_rotation_degrees"));
    let mut ret_var = VariantBuffer { data: [0u8; 48] };
    let mut err = GDExtensionCallError { error: 0, argument: 0, expected: 0 };
    call_fn(
        node_var.data.as_mut_ptr() as *mut c_void,
        METHOD_GET_ROT.as_ptr() as *const c_void,
        std::ptr::null(),
        0,
        ret_var.data.as_mut_ptr() as *mut c_void,
        &mut err,
    );

    let v = variant_to_vector3(ret_var.data.as_ptr() as *const c_void);
    variant_destroy(ret_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(node_var.data.as_mut_ptr() as *mut c_void);
    v[1]
}

pub unsafe fn node_get_parent(node_obj: GDExtensionObjectPtr) -> GDExtensionObjectPtr {
    call_object_method_ret_obj(node_obj, "get_parent")
}

pub unsafe fn node_reparent(child_obj: GDExtensionObjectPtr, new_parent_obj: GDExtensionObjectPtr, keep_global_transform: bool) {
    if child_obj.is_null() || new_parent_obj.is_null() {
        return;
    }
    let cur_parent = node_get_parent(child_obj);
    if cur_parent == new_parent_obj {
        return;
    }
    let mut parent_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(parent_var.data.as_mut_ptr() as *mut c_void, new_parent_obj);
    let mut keep_var = VariantBuffer { data: [0u8; 48] };
    variant_from_bool(keep_var.data.as_mut_ptr() as *mut c_void, keep_global_transform);
    let args = [
        parent_var.data.as_ptr() as GDExtensionConstVariantPtr,
        keep_var.data.as_ptr() as GDExtensionConstVariantPtr,
    ];
    call_object_method_void(child_obj, "reparent", &args);
    variant_destroy(keep_var.data.as_mut_ptr() as *mut c_void);
    variant_destroy(parent_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node_add_child_obj(parent_obj: GDExtensionObjectPtr, child_obj: GDExtensionObjectPtr) {
    if parent_obj.is_null() || child_obj.is_null() {
        return;
    }
    let mut child_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(child_var.data.as_mut_ptr() as *mut c_void, child_obj);
    let args = [child_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(parent_obj, "add_child", &args);
    variant_destroy(child_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node_remove_child_obj(parent_obj: GDExtensionObjectPtr, child_obj: GDExtensionObjectPtr) {
    if parent_obj.is_null() || child_obj.is_null() {
        return;
    }
    let mut child_var = VariantBuffer { data: [0u8; 48] };
    variant_from_object(child_var.data.as_mut_ptr() as *mut c_void, child_obj);
    let args = [child_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(parent_obj, "remove_child", &args);
    variant_destroy(child_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn node_get_tree(node_obj: GDExtensionObjectPtr) -> GDExtensionObjectPtr {
    call_object_method_ret_obj(node_obj, "get_tree")
}

pub unsafe fn scene_tree_get_current_scene(tree_obj: GDExtensionObjectPtr) -> GDExtensionObjectPtr {
    call_object_method_ret_obj(tree_obj, "get_current_scene")
}

pub unsafe fn collision_shape3d_get_shape(col_obj: GDExtensionObjectPtr) -> GDExtensionObjectPtr {
    call_object_method_ret_obj(col_obj, "get_shape")
}

pub unsafe fn shape_set_radius(shape_obj: GDExtensionObjectPtr, radius: f32) {
    if shape_obj.is_null() {
        return;
    }
    let mut rad_var = VariantBuffer { data: [0u8; 48] };
    variant_from_float(rad_var.data.as_mut_ptr() as *mut c_void, radius as f64);
    let args = [rad_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(shape_obj, "set_radius", &args);
    variant_destroy(rad_var.data.as_mut_ptr() as *mut c_void);
}

pub unsafe fn shape_set_height(shape_obj: GDExtensionObjectPtr, height: f32) {
    if shape_obj.is_null() {
        return;
    }
    let mut h_var = VariantBuffer { data: [0u8; 48] };
    variant_from_float(h_var.data.as_mut_ptr() as *mut c_void, height as f64);
    let args = [h_var.data.as_ptr() as GDExtensionConstVariantPtr];
    call_object_method_void(shape_obj, "set_height", &args);
    variant_destroy(h_var.data.as_mut_ptr() as *mut c_void);
}





unsafe fn init_node_method_binds() {
    let get_method_bind_ptr = CLASSDB_GET_METHOD_BIND.load(Ordering::SeqCst);
    if !get_method_bind_ptr.is_null() {
        let get_mb_fn: FnClassdbGetMethodBind = std::mem::transmute(get_method_bind_ptr);

        let node_sn = ManagedStringName::new("Node");
        let set_proc_sn = ManagedStringName::new("set_process");
        let set_phys_sn = ManagedStringName::new("set_physics_process");
        let get_proc_delta_sn = ManagedStringName::new("get_process_delta_time");
        let get_phys_delta_sn = ManagedStringName::new("get_physics_process_delta_time");

        let mb_set_proc = get_mb_fn(node_sn.as_ptr() as *const c_void, set_proc_sn.as_ptr() as *const c_void, 2586408642);
        let mb_set_phys = get_mb_fn(node_sn.as_ptr() as *const c_void, set_phys_sn.as_ptr() as *const c_void, 2586408642);
        let mb_get_proc_delta = get_mb_fn(node_sn.as_ptr() as *const c_void, get_proc_delta_sn.as_ptr() as *const c_void, 1740695150);
        let mb_get_phys_delta = get_mb_fn(node_sn.as_ptr() as *const c_void, get_phys_delta_sn.as_ptr() as *const c_void, 1740695150);

        METHOD_SET_PROCESS.store(mb_set_proc as *mut (), Ordering::SeqCst);
        METHOD_SET_PHYSICS_PROCESS.store(mb_set_phys as *mut (), Ordering::SeqCst);
        METHOD_GET_PROCESS_DELTA.store(mb_get_proc_delta as *mut (), Ordering::SeqCst);
        METHOD_GET_PHYSICS_PROCESS_DELTA.store(mb_get_phys_delta as *mut (), Ordering::SeqCst);

        let rb_sn = ManagedStringName::new("RigidBody3D");
        let set_com_sn = ManagedStringName::new("set_center_of_mass_mode");
        let set_grav_sn = ManagedStringName::new("set_gravity_scale");
        let get_grav_sn = ManagedStringName::new("get_gravity_scale");
        let set_frz_sn = ManagedStringName::new("set_freeze_enabled");
        let set_frzm_sn = ManagedStringName::new("set_freeze_mode");
        let set_lin_sn = ManagedStringName::new("set_linear_velocity");
        let get_lin_sn = ManagedStringName::new("get_linear_velocity");
        let set_ang_sn = ManagedStringName::new("set_angular_velocity");
        let get_ang_sn = ManagedStringName::new("get_angular_velocity");
        let set_cust_sn = ManagedStringName::new("set_use_custom_integrator");
        let is_cust_sn = ManagedStringName::new("is_using_custom_integrator");

        METHOD_SET_CENTER_OF_MASS_MODE.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_com_sn.as_ptr() as *const c_void, 3625866032) as *mut (), Ordering::SeqCst);
        METHOD_SET_GRAVITY_SCALE.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_grav_sn.as_ptr() as *const c_void, 373806689) as *mut (), Ordering::SeqCst);
        METHOD_GET_GRAVITY_SCALE.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, get_grav_sn.as_ptr() as *const c_void, 1740695150) as *mut (), Ordering::SeqCst);
        METHOD_SET_FREEZE_ENABLED.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_frz_sn.as_ptr() as *const c_void, 2586408642) as *mut (), Ordering::SeqCst);
        METHOD_SET_FREEZE_MODE.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_frzm_sn.as_ptr() as *const c_void, 1319914653) as *mut (), Ordering::SeqCst);
        METHOD_SET_LINEAR_VELOCITY.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_lin_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_GET_LINEAR_VELOCITY.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, get_lin_sn.as_ptr() as *const c_void, 3360562783) as *mut (), Ordering::SeqCst);
        METHOD_SET_ANGULAR_VELOCITY.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_ang_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_GET_ANGULAR_VELOCITY.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, get_ang_sn.as_ptr() as *const c_void, 3360562783) as *mut (), Ordering::SeqCst);
        METHOD_SET_USE_CUSTOM_INTEGRATOR.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_cust_sn.as_ptr() as *const c_void, 2586408642) as *mut (), Ordering::SeqCst);
        METHOD_IS_USING_CUSTOM_INTEGRATOR.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, is_cust_sn.as_ptr() as *const c_void, 2240911060) as *mut (), Ordering::SeqCst);

        let is_lock_sn = ManagedStringName::new("is_lock_rotation_enabled");
        let set_lock_sn = ManagedStringName::new("set_lock_rotation_enabled");
        let set_max_c_sn = ManagedStringName::new("set_max_contacts_reported");
        let set_c_mon_sn = ManagedStringName::new("set_contact_monitor");
        let set_mass_sn = ManagedStringName::new("set_mass");
        let get_mass_sn = ManagedStringName::new("get_mass");
        let set_com_pos_sn = ManagedStringName::new("set_center_of_mass");
        let get_com_pos_sn = ManagedStringName::new("get_center_of_mass");
        let rb_app_c_imp_sn = ManagedStringName::new("apply_central_impulse");
        let rb_app_t_imp_sn = ManagedStringName::new("apply_torque_impulse");

        METHOD_IS_LOCK_ROTATION_ENABLED.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, is_lock_sn.as_ptr() as *const c_void, 36873697) as *mut (), Ordering::SeqCst);
        METHOD_SET_LOCK_ROTATION_ENABLED.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_lock_sn.as_ptr() as *const c_void, 2586408642) as *mut (), Ordering::SeqCst);
        METHOD_SET_MAX_CONTACTS_REPORTED.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_max_c_sn.as_ptr() as *const c_void, 1286410249) as *mut (), Ordering::SeqCst);
        METHOD_SET_CONTACT_MONITOR.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_c_mon_sn.as_ptr() as *const c_void, 2586408642) as *mut (), Ordering::SeqCst);
        METHOD_SET_MASS.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_mass_sn.as_ptr() as *const c_void, 373806689) as *mut (), Ordering::SeqCst);
        METHOD_GET_MASS.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, get_mass_sn.as_ptr() as *const c_void, 1740695150) as *mut (), Ordering::SeqCst);
        METHOD_SET_CENTER_OF_MASS.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, set_com_pos_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_GET_CENTER_OF_MASS.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, get_com_pos_sn.as_ptr() as *const c_void, 3360562783) as *mut (), Ordering::SeqCst);
        METHOD_RIGIDBODY_APPLY_CENTRAL_IMPULSE.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, rb_app_c_imp_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_RIGIDBODY_APPLY_TORQUE_IMPULSE.store(get_mb_fn(rb_sn.as_ptr() as *const c_void, rb_app_t_imp_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);

        let n3d_sn = ManagedStringName::new("Node3D");
        let get_gt_sn = ManagedStringName::new("get_global_transform");
        let set_gt_sn = ManagedStringName::new("set_global_transform");
        let get_gp_sn = ManagedStringName::new("get_global_position");
        let set_gp_sn = ManagedStringName::new("set_global_position");
        let get_t_sn = ManagedStringName::new("get_transform");
        let set_t_sn = ManagedStringName::new("set_transform");
        let set_nt_sn = ManagedStringName::new("set_notify_transform");

        METHOD_NODE3D_GET_GLOBAL_TRANSFORM.store(get_mb_fn(n3d_sn.as_ptr() as *const c_void, get_gt_sn.as_ptr() as *const c_void, 3229777777) as *mut (), Ordering::SeqCst);
        METHOD_NODE3D_SET_GLOBAL_TRANSFORM.store(get_mb_fn(n3d_sn.as_ptr() as *const c_void, set_gt_sn.as_ptr() as *const c_void, 2952846383) as *mut (), Ordering::SeqCst);
        METHOD_NODE3D_GET_GLOBAL_POSITION.store(get_mb_fn(n3d_sn.as_ptr() as *const c_void, get_gp_sn.as_ptr() as *const c_void, 3360562783) as *mut (), Ordering::SeqCst);
        METHOD_NODE3D_SET_GLOBAL_POSITION.store(get_mb_fn(n3d_sn.as_ptr() as *const c_void, set_gp_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_NODE3D_GET_TRANSFORM.store(get_mb_fn(n3d_sn.as_ptr() as *const c_void, get_t_sn.as_ptr() as *const c_void, 3229777777) as *mut (), Ordering::SeqCst);
        METHOD_NODE3D_SET_TRANSFORM.store(get_mb_fn(n3d_sn.as_ptr() as *const c_void, set_t_sn.as_ptr() as *const c_void, 2952846383) as *mut (), Ordering::SeqCst);
        METHOD_NODE3D_SET_NOTIFY_TRANSFORM.store(get_mb_fn(n3d_sn.as_ptr() as *const c_void, set_nt_sn.as_ptr() as *const c_void, 2586408642) as *mut (), Ordering::SeqCst);

        let state_sn = ManagedStringName::new("PhysicsDirectBodyState3D");
        let c_cnt_sn = ManagedStringName::new("get_contact_count");
        let c_lshape_sn = ManagedStringName::new("get_contact_local_shape");
        let c_imp_sn = ManagedStringName::new("get_contact_impulse");
        let c_lnorm_sn = ManagedStringName::new("get_contact_local_normal");
        let c_lpos_sn = ManagedStringName::new("get_contact_local_position");
        let c_cpos_sn = ManagedStringName::new("get_contact_collider_position");
        let c_cvel_sn = ManagedStringName::new("get_contact_collider_velocity_at_position");
        let c_cobj_sn = ManagedStringName::new("get_contact_collider_object");
        let s_glin_sn = ManagedStringName::new("get_linear_velocity");
        let s_slin_sn = ManagedStringName::new("set_linear_velocity");
        let s_gang_sn = ManagedStringName::new("get_angular_velocity");
        let s_sang_sn = ManagedStringName::new("set_angular_velocity");
        let s_gt_sn = ManagedStringName::new("get_transform");
        let s_st_sn = ManagedStringName::new("set_transform");
        let s_grav_sn = ManagedStringName::new("get_total_gravity");
        let s_app_c_imp_sn = ManagedStringName::new("apply_central_impulse");
        let s_app_c_frc_sn = ManagedStringName::new("apply_central_force");
        let s_app_trq_sn = ManagedStringName::new("apply_torque");
        let s_imass_sn = ManagedStringName::new("get_inverse_mass");
        let s_step_sn = ManagedStringName::new("get_step");

        METHOD_STATE_GET_CONTACT_COUNT.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_cnt_sn.as_ptr() as *const c_void, 3905245786) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_CONTACT_LOCAL_SHAPE.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_lshape_sn.as_ptr() as *const c_void, 923996154) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_CONTACT_IMPULSE.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_imp_sn.as_ptr() as *const c_void, 711720468) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_CONTACT_LOCAL_NORMAL.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_lnorm_sn.as_ptr() as *const c_void, 711720468) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_CONTACT_LOCAL_POSITION.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_lpos_sn.as_ptr() as *const c_void, 711720468) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_CONTACT_COLLIDER_POSITION.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_cpos_sn.as_ptr() as *const c_void, 711720468) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_CONTACT_COLLIDER_VELOCITY.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_cvel_sn.as_ptr() as *const c_void, 711720468) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_CONTACT_COLLIDER_OBJECT.store(get_mb_fn(state_sn.as_ptr() as *const c_void, c_cobj_sn.as_ptr() as *const c_void, 3332903315) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_LINEAR_VELOCITY.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_glin_sn.as_ptr() as *const c_void, 3360562783) as *mut (), Ordering::SeqCst);
        METHOD_STATE_SET_LINEAR_VELOCITY.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_slin_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_ANGULAR_VELOCITY.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_gang_sn.as_ptr() as *const c_void, 3360562783) as *mut (), Ordering::SeqCst);
        METHOD_STATE_SET_ANGULAR_VELOCITY.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_sang_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_TRANSFORM.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_gt_sn.as_ptr() as *const c_void, 3229777777) as *mut (), Ordering::SeqCst);
        METHOD_STATE_SET_TRANSFORM.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_st_sn.as_ptr() as *const c_void, 2952846383) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_TOTAL_GRAVITY.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_grav_sn.as_ptr() as *const c_void, 3360562783) as *mut (), Ordering::SeqCst);
        METHOD_STATE_APPLY_CENTRAL_IMPULSE.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_app_c_imp_sn.as_ptr() as *const c_void, 2007698547) as *mut (), Ordering::SeqCst);
        METHOD_STATE_APPLY_CENTRAL_FORCE.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_app_c_frc_sn.as_ptr() as *const c_void, 2007698547) as *mut (), Ordering::SeqCst);
        METHOD_STATE_APPLY_TORQUE.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_app_trq_sn.as_ptr() as *const c_void, 3460891852) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_INVERSE_MASS.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_imass_sn.as_ptr() as *const c_void, 1740695150) as *mut (), Ordering::SeqCst);
        METHOD_STATE_GET_STEP.store(get_mb_fn(state_sn.as_ptr() as *const c_void, s_step_sn.as_ptr() as *const c_void, 1740695150) as *mut (), Ordering::SeqCst);
    }
}

pub type GDExtensionClassCallVirtual = unsafe extern "C" fn(
    instance: GDExtensionClassInstancePtr,
    args: *const *const c_void,
    ret: *mut c_void,
);
pub type VirtualMethodResolver = fn(class_name: &'static str, method_name: &str) -> Option<GDExtensionClassCallVirtual>;
static VIRTUAL_RESOLVERS: Mutex<Vec<VirtualMethodResolver>> = Mutex::new(Vec::new());

pub fn register_virtual_resolver(resolver: VirtualMethodResolver) {
    if let Ok(mut list) = VIRTUAL_RESOLVERS.lock() {
        list.push(resolver);
    }
}

pub unsafe extern "C" fn rust_class_get_virtual(
    class_userdata: *mut c_void,
    name: GDExtensionConstStringNamePtr,
    _hash: u32,
) -> *mut c_void {
    if class_userdata.is_null() || name.is_null() {
        return std::ptr::null_mut();
    }
    let desc = &*(class_userdata as *const RustClassDescriptor);
    let method_name = string_name_to_string(name);

    let mut resolvers_chain: Vec<fn(&str) -> Option<GDExtensionClassCallVirtual>> = Vec::new();
    {
        if let (Ok(parents), Ok(handlers)) = (CLASS_PARENTS.lock(), CLASS_HANDLERS.lock()) {
            let mut cur = desc.name;
            let mut depth = 0;
            loop {
                if let Some(h) = handlers.get(cur) {
                    resolvers_chain.push(h.resolve_virtual);
                }
                match parents.get(cur) {
                    Some(&p) => {
                        cur = p;
                        depth += 1;
                        if depth > 32 {
                            break;
                        }
                    }
                    None => break,
                }
            }
        }
    }
    for resolver in resolvers_chain {
        if let Some(call_v) = resolver(&method_name) {
            return call_v as *mut c_void;
        }
    }

    if let Ok(resolvers) = VIRTUAL_RESOLVERS.lock() {
        for resolver in resolvers.iter() {
            if let Some(call_v) = resolver(desc.name, &method_name) {
                return call_v as *mut c_void;
            }
        }
    }
    std::ptr::null_mut()
}

pub unsafe fn make_class(
    library: GDExtensionClassLibraryPtr,
    name: &'static str,
    parent: &'static str,
    base: &'static str,
) -> usize {
    let sn_new_ptr = STRING_NAME_NEW.load(Ordering::SeqCst);
    let reg_class_ptr = CLASSDB_REGISTER_CLASS6.load(Ordering::SeqCst);
    if sn_new_ptr.is_null() || reg_class_ptr.is_null() {
        return 0;
    }
    let sn_new_fn: FnStringNameNewUtf8 = std::mem::transmute(sn_new_ptr);
    let reg_class_fn: FnClassdbRegisterExtensionClass6 = std::mem::transmute(reg_class_ptr);

    let mut class_sn: usize = 0;
    let mut parent_sn: usize = 0;
    let mut base_sn: usize = 0;
    let c_name = std::ffi::CString::new(name).unwrap();
    let c_parent = std::ffi::CString::new(parent).unwrap();
    let c_base = std::ffi::CString::new(base).unwrap();
    sn_new_fn(&mut class_sn as *mut _ as *mut c_void, c_name.as_ptr());
    sn_new_fn(&mut parent_sn as *mut _ as *mut c_void, c_parent.as_ptr());
    sn_new_fn(&mut base_sn as *mut _ as *mut c_void, c_base.as_ptr());

    let desc = Box::leak(Box::new(RustClassDescriptor {
        name,
        parent,
        base_godot_class: base,
        class_sn,
        parent_sn,
        base_sn,
    }));

    let info = GDExtensionClassCreationInfo6 {
        is_virtual: 0,
        is_abstract: 0,
        is_exposed: 1,
        is_runtime: 0,
        icon_path: std::ptr::null(),
        set_func: Some(rust_class_set),
        get_func: Some(rust_class_get),
        get_property_list_func: None,
        free_property_list_func: None,
        property_can_revert_func: None,
        property_get_revert_func: None,
        validate_property_func: None,
        notification_func: Some(rust_class_notification),
        to_string_func: None,
        reference_func: None,
        unreference_func: None,
        create_instance_func: Some(rust_create_instance),
        free_instance_func: Some(rust_free_instance),
        recreate_instance_func: Some(rust_recreate_instance),
        get_virtual_func: Some(rust_class_get_virtual),
        get_virtual_call_data_func: None,
        call_virtual_with_data_func: None,
        class_userdata: desc as *mut _ as *mut c_void,
    };

    reg_class_fn(
        library,
        &desc.class_sn as *const _ as *const c_void,
        &desc.parent_sn as *const _ as *const c_void,
        &info,
    );

    if let Ok(mut parents) = CLASS_PARENTS.lock() {
        parents.insert(name, parent);
    }

    if let Ok(mut classes) = REGISTERED_CLASSES.lock() {
        classes.push(class_sn);
    }

    class_sn
}

pub type GameInitCallback = unsafe fn(GDExtensionClassLibraryPtr);
static GAME_INIT_CALLBACK: std::sync::atomic::AtomicPtr<()> = std::sync::atomic::AtomicPtr::new(std::ptr::null_mut());

pub fn set_game_init_callback(cb: GameInitCallback) {
    GAME_INIT_CALLBACK.store(cb as *mut (), std::sync::atomic::Ordering::SeqCst);
}

pub fn get_game_init_callback() -> Option<GameInitCallback> {
    let ptr = GAME_INIT_CALLBACK.load(std::sync::atomic::Ordering::SeqCst);
    if ptr.is_null() {
        None
    } else {
        unsafe { Some(std::mem::transmute::<*mut (), GameInitCallback>(ptr)) }
    }
}

unsafe extern "C" fn rust_extension_initialize(
    userdata: *mut c_void,
    level: GDExtensionInitializationLevel,
) {
    if level == GDExtensionInitializationLevel::Scene {
        init_node_method_binds();
        let library = userdata as GDExtensionClassLibraryPtr;
        if let Some(cb) = get_game_init_callback() {
            cb(library);
        }
    } else if level == GDExtensionInitializationLevel::Editor {
        load_registered_editor_help();
    }
}

unsafe extern "C" fn rust_extension_deinitialize(
    _userdata: *mut c_void,
    _level: GDExtensionInitializationLevel,
) {
    // Left intact for Godot's safe GDExtension reload lifecycle without dropping active nodes
}

pub unsafe extern "C" fn gdextension_entry(
    p_get_proc_address: GDExtensionInterfaceGetProcAddress,
    p_library: GDExtensionClassLibraryPtr,
    r_initialization: *mut GDExtensionInitialization,
) -> GDExtensionBool {
    std::panic::set_hook(Box::new(|info| {
        eprintln!("==================================================");
        eprintln!("RUST PANIC: {}", info);
        eprintln!("==================================================");
    }));
    if p_get_proc_address.is_none() || r_initialization.is_null() {
        return 0;
    }
    let get_proc = p_get_proc_address.unwrap();
    GET_PROC_ADDRESS.store(get_proc as *mut (), Ordering::SeqCst);

    let get_symbol = |name: &str| -> *mut c_void {
        if let Ok(c_str) = std::ffi::CString::new(name) {
            get_proc(c_str.as_ptr())
        } else {
            std::ptr::null_mut()
        }
    };

    STRING_NAME_NEW.store(get_symbol("string_name_new_with_utf8_chars") as *mut (), Ordering::SeqCst);
    STRING_NEW_WITH_UTF8_CHARS.store(get_symbol("string_new_with_utf8_chars") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_CLASS6.store(get_symbol("classdb_register_extension_class6") as *mut (), Ordering::SeqCst);
    CLASSDB_UNREGISTER_CLASS.store(get_symbol("classdb_unregister_extension_class") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_METHOD.store(get_symbol("classdb_register_extension_class_method") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_PROPERTY.store(get_symbol("classdb_register_extension_class_property") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_PROPERTY_GROUP.store(get_symbol("classdb_register_extension_class_property_group") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_PROPERTY_SUBGROUP.store(get_symbol("classdb_register_extension_class_property_subgroup") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_SIGNAL.store(get_symbol("classdb_register_extension_class_signal") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_INTEGER_CONSTANT.store(get_symbol("classdb_register_extension_class_integer_constant") as *mut (), Ordering::SeqCst);
    CLASSDB_CONSTRUCT_OBJECT.store(get_symbol("classdb_construct_object") as *mut (), Ordering::SeqCst);
    GLOBAL_GET_SINGLETON.store(get_symbol("global_get_singleton") as *mut (), Ordering::SeqCst);
    OBJECT_SET_INSTANCE.store(get_symbol("object_set_instance") as *mut (), Ordering::SeqCst);
    VARIANT_NEW_COPY.store(get_symbol("variant_new_copy") as *mut (), Ordering::SeqCst);
    VARIANT_DESTROY.store(get_symbol("variant_destroy") as *mut (), Ordering::SeqCst);
    VARIANT_NEW_NIL.store(get_symbol("variant_new_nil") as *mut (), Ordering::SeqCst);
    VARIANT_CONSTRUCT.store(get_symbol("variant_construct") as *mut (), Ordering::SeqCst);
    GET_FROM_TYPE_CTOR.store(get_symbol("get_variant_from_type_constructor") as *mut (), Ordering::SeqCst);
    GET_TO_TYPE_CTOR.store(get_symbol("get_variant_to_type_constructor") as *mut (), Ordering::SeqCst);
    OBJECT_GET_ID.store(get_symbol("object_get_instance_id") as *mut (), Ordering::SeqCst);
    OBJECT_GET_FROM_ID.store(get_symbol("object_get_instance_from_id") as *mut (), Ordering::SeqCst);
    OBJECT_HAS_SCRIPT_METHOD.store(get_symbol("object_has_script_method") as *mut (), Ordering::SeqCst);
    OBJECT_CALL_SCRIPT_METHOD.store(get_symbol("object_call_script_method") as *mut (), Ordering::SeqCst);
    CLASSDB_REGISTER_VIRTUAL_METHOD.store(get_symbol("classdb_register_extension_class_virtual_method") as *mut (), Ordering::SeqCst);
    CLASSDB_GET_METHOD_BIND.store(get_symbol("classdb_get_method_bind") as *mut (), Ordering::SeqCst);
    OBJECT_METHOD_BIND_PTRCALL.store(get_symbol("object_method_bind_ptrcall") as *mut (), Ordering::SeqCst);
    VARIANT_CALL.store(get_symbol("variant_call") as *mut (), Ordering::SeqCst);
    VARIANT_GET_NAMED.store(get_symbol("variant_get_named") as *mut (), Ordering::SeqCst);
    VARIANT_GET_TYPE.store(get_symbol("variant_get_type") as *mut (), Ordering::SeqCst);
    VARIANT_STRINGIFY.store(get_symbol("variant_stringify") as *mut (), Ordering::SeqCst);
    STRING_TO_UTF8_CHARS.store(get_symbol("string_to_utf8_chars") as *mut (), Ordering::SeqCst);
    let destr_sym = get_symbol("variant_get_ptr_destructor");
    if !destr_sym.is_null() {
        VARIANT_GET_PTR_DESTRUCTOR.store(destr_sym as *mut (), Ordering::SeqCst);
        let destr_fn: FnVariantGetPtrDestructor = std::mem::transmute(destr_sym);
        if let Some(str_destr) = destr_fn(GDEXTENSION_VARIANT_TYPE_STRING) {
            STRING_DESTRUCTOR.store(str_destr as *mut (), Ordering::SeqCst);
        }
    }
    EDITOR_HELP_LOAD_XML.store(get_symbol("editor_help_load_xml_from_utf8_chars") as *mut (), Ordering::SeqCst);
    EDITOR_HELP_LOAD_XML_AND_LEN.store(get_symbol("editor_help_load_xml_from_utf8_chars_and_len") as *mut (), Ordering::SeqCst);
    DICTIONARY_OPERATOR_INDEX.store(get_symbol("dictionary_operator_index") as *mut (), Ordering::SeqCst);

    let init = &mut *r_initialization;
    init.minimum_initialization_level = GDExtensionInitializationLevel::Scene;
    init.userdata = p_library;
    init.initialize = Some(rust_extension_initialize);
    init.deinitialize = Some(rust_extension_deinitialize);

    1
}


pub struct GodotApiTable {
    pub print: unsafe extern "C" fn(*const u8),
    pub print_warning: unsafe extern "C" fn(*const u8, *const u8, *const u8, i32),
    pub print_error: unsafe extern "C" fn(*const u8, *const u8, *const u8, i32),
    pub node2d_create: unsafe extern "C" fn() -> u64,
    pub node_add_child: unsafe extern "C" fn(u64, u64),
    pub node_remove_child: unsafe extern "C" fn(u64, u64),
    pub node_free: unsafe extern "C" fn(u64),
    pub node2d_get_position: unsafe extern "C" fn(u64, *mut f32) -> i32,
    pub node2d_set_position: unsafe extern "C" fn(u64, f32, f32) -> i32,
    pub node3d_get_position: unsafe extern "C" fn(u64, *mut f32) -> i32,
    pub node3d_set_position: unsafe extern "C" fn(u64, f32, f32, f32) -> i32,
}

static API_TABLE: AtomicPtr<GodotApiTable> = AtomicPtr::new(std::ptr::null_mut());

#[no_mangle]
pub extern "C" fn init_godot_rust_api(api: *const GodotApiTable) {
    if !api.is_null() {
        API_TABLE.store(api as *mut GodotApiTable, Ordering::SeqCst);
    }
}

pub fn print(s: &str) {
    let table_ptr = API_TABLE.load(Ordering::SeqCst);
    if !table_ptr.is_null() {
        if let Ok(c_str) = std::ffi::CString::new(s) {
            unsafe {
                ((*table_ptr).print)(c_str.as_ptr() as *const u8);
            }
        }
    }
}

pub fn print_warning(s: &str, func: &str, file: &str, line: i32) {
    let table_ptr = API_TABLE.load(Ordering::SeqCst);
    if !table_ptr.is_null() {
        let cs = std::ffi::CString::new(s).unwrap_or_default();
        let cf = std::ffi::CString::new(func).unwrap_or_default();
        let cfi = std::ffi::CString::new(file).unwrap_or_default();
        unsafe {
            ((*table_ptr).print_warning)(cs.as_ptr() as *const u8, cf.as_ptr() as *const u8, cfi.as_ptr() as *const u8, line);
        }
    }
}

pub fn print_error(s: &str, func: &str, file: &str, line: i32) {
    let table_ptr = API_TABLE.load(Ordering::SeqCst);
    if !table_ptr.is_null() {
        let cs = std::ffi::CString::new(s).unwrap_or_default();
        let cf = std::ffi::CString::new(func).unwrap_or_default();
        let cfi = std::ffi::CString::new(file).unwrap_or_default();
        unsafe {
            ((*table_ptr).print_error)(cs.as_ptr() as *const u8, cf.as_ptr() as *const u8, cfi.as_ptr() as *const u8, line);
        }
    }
}

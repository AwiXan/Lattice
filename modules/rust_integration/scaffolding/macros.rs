//! Ergonomic macros for registering Godot GDExtension properties, signals, and methods.

use crate::godot_ffi::{GDExtensionClassInstancePtr, GDExtensionObjectPtr, RustNodeInstance};

#[inline(always)]
pub unsafe fn with_instance_user_data<T: 'static, R>(
    instance: GDExtensionClassInstancePtr,
    f: impl FnOnce(&mut T) -> R,
) -> Option<R> {
    if instance.is_null() {
        return None;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    rust_inst.user_data.with_lock(|ud| {
        if let Some(ref mut boxed) = *ud {
            if let Some(val) = (**boxed).downcast_mut::<T>() {
                return Some(f(val));
            }
            if let Some(hook) = crate::godot_ffi::get_user_data_downcast_hook() {
                if let Some(ptr) = hook(&mut **boxed, std::any::TypeId::of::<T>()) {
                    let typed_ref = &mut *(ptr as *mut T);
                    return Some(f(typed_ref));
                }
            }
        }
        None
    })
}

#[inline(always)]
pub unsafe fn with_instance_user_data_and_host<T: 'static, R>(
    instance: GDExtensionClassInstancePtr,
    f: impl FnOnce(&mut T, GDExtensionObjectPtr) -> R,
) -> Option<R> {
    if instance.is_null() {
        return None;
    }
    let rust_inst = &*(instance as *const RustNodeInstance);
    let host = rust_inst.godot_object;
    rust_inst.user_data.with_lock(|ud| {
        if let Some(ref mut boxed) = *ud {
            if let Some(val) = (**boxed).downcast_mut::<T>() {
                return Some(f(val, host));
            }
            if let Some(hook) = crate::godot_ffi::get_user_data_downcast_hook() {
                if let Some(ptr) = hook(&mut **boxed, std::any::TypeId::of::<T>()) {
                    let typed_ref = &mut *(ptr as *mut T);
                    return Some(f(typed_ref, host));
                }
            }
        }
        None
    })
}

#[macro_export]
macro_rules! bind_properties {
    ($lib:expr, $class_sn:expr, $( $name:literal : $prop_type:expr ),* $(,)?) => {
        $(
            $crate::godot_ffi::bind_prop($lib, $class_sn, $prop_type, $name);
        )*
    };
}

#[macro_export]
macro_rules! bind_prop {
    ($lib:expr, $class_sn:expr, $prop_type:expr, $name:literal) => {
        $crate::godot_ffi::bind_prop($lib, $class_sn, $prop_type, $name);
    };
    ($lib:expr, $class_sn:expr, $prop_type:expr, $name:literal, $hint:expr, $hint_str:literal) => {
        $crate::godot_ffi::bind_prop_hint($lib, $class_sn, $prop_type, $name, $hint, $hint_str);
    };
}

#[macro_export]
macro_rules! bind_signal {
    ($lib:expr, $class_sn:expr, $sig_name:literal) => {
        $crate::godot_ffi::register_signal($lib, $class_sn, $sig_name, &[]);
    };
    ($lib:expr, $class_sn:expr, $sig_name:literal, [ $( ($arg_type:expr, $arg_name:literal) ),* $(,)? ]) => {
        $crate::godot_ffi::register_signal($lib, $class_sn, $sig_name, &[
            $( ($arg_type, $arg_name) ),*
        ]);
    };
}

#[macro_export]
macro_rules! bind_method {
    // Void method with no arguments
    ($lib:expr, $class_sn:expr, $name:literal, $func:expr) => {
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $func,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
            false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None, &[]
        );
    };
    // Void method with arguments
    ($lib:expr, $class_sn:expr, $name:literal, $func:expr, [ $( ($arg_type:expr, $arg_name:literal) ),* $(,)? ]) => {
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $func,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
            false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
            &[ $( ($arg_type, $arg_name) ),* ]
        );
    };
    // Return method with arguments
    ($lib:expr, $class_sn:expr, $name:literal, $func:expr, -> ($ret_type:expr, $ret_class:expr), [ $( ($arg_type:expr, $arg_name:literal) ),* $(,)? ]) => {
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $func,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
            true, $ret_type, $ret_class,
            &[ $( ($arg_type, $arg_name) ),* ]
        );
    };
}

#[macro_export]
macro_rules! bind_static_method {
    // Static method without return
    ($lib:expr, $class_sn:expr, $name:literal, $func:expr, [ $( ($arg_type:expr, $arg_name:literal) ),* $(,)? ]) => {
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $func,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
            false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
            &[ $( ($arg_type, $arg_name) ),* ]
        );
    };
    // Static method with return
    ($lib:expr, $class_sn:expr, $name:literal, $func:expr, -> ($ret_type:expr, $ret_class:expr), [ $( ($arg_type:expr, $arg_name:literal) ),* $(,)? ]) => {
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $func,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
            true, $ret_type, $ret_class,
            &[ $( ($arg_type, $arg_name) ),* ]
        );
    };
}

#[macro_export]
macro_rules! bind_virtual_method {
    // Void virtual method without arguments
    ($lib:expr, $class_sn:expr, $name:literal) => {
        $crate::godot_ffi::register_virtual_method(
            $lib, $class_sn, $name, false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None, &[]
        );
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $crate::godot_ffi::generic_void_call,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_VIRTUAL,
            false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None, &[]
        );
    };
    // Void virtual method with arguments
    ($lib:expr, $class_sn:expr, $name:literal, [ $( ($arg_type:ident, $arg_name:literal) ),* $(,)? ]) => {
        $crate::godot_ffi::register_virtual_method(
            $lib, $class_sn, $name, false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
            &[ $( ($crate::__variant_type_id!($arg_type), $arg_name) ),* ]
        );
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $crate::godot_ffi::generic_void_call,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_VIRTUAL,
            false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
            &[ $( ($crate::__variant_type_id!($arg_type), $arg_name) ),* ]
        );
    };
    // Return virtual method with arguments
    ($lib:expr, $class_sn:expr, $name:literal, -> ($ret_type:ident, $ret_class:expr), [ $( ($arg_type:ident, $arg_name:literal) ),* $(,)? ]) => {
        $crate::godot_ffi::register_virtual_method(
            $lib, $class_sn, $name, true, $crate::__variant_type_id!($ret_type), $ret_class,
            &[ $( ($crate::__variant_type_id!($arg_type), $arg_name) ),* ]
        );
        $crate::godot_ffi::register_method(
            $lib, $class_sn, $name, $crate::godot_ffi::generic_nil_call,
            $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_VIRTUAL,
            true, $crate::__variant_type_id!($ret_type), $ret_class,
            &[ $( ($crate::__variant_type_id!($arg_type), $arg_name) ),* ]
        );
    };
}

#[macro_export]
macro_rules! __variant_type_id {
    (bool) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_BOOL };
    (f32) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_FLOAT };
    (float) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_FLOAT };
    (f64) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_FLOAT };
    (i32) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_INT };
    (int) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_INT };
    (i64) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_INT };
    (u32) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_INT };
    (u64) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_INT };
    (u8) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_INT };
    (vector2) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_VECTOR2 };
    (Vector2) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_VECTOR2 };
    (vector3) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_VECTOR3 };
    (Vector3) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_VECTOR3 };
    (basis) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_BASIS };
    (Basis) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_BASIS };
    (transform3d) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_TRANSFORM3D };
    (Transform3D) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_TRANSFORM3D };
    (string) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_STRING };
    (String) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_STRING };
    (string_name) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_STRING_NAME };
    (StringName) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_STRING_NAME };
    (node_path) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NODE_PATH };
    (NodePath) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NODE_PATH };
    (object) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_OBJECT };
    (Object) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_OBJECT };
    (array) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_ARRAY };
    (Array) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_ARRAY };
    (string_array) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_ARRAY };
    (dictionary) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_DICTIONARY };
    (Dictionary) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_DICTIONARY };
    (variant) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL };
    (Variant) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL };
    (void) => { $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL };
}

#[macro_export]
macro_rules! __unpack_arg {
    (bool, $arg:expr) => { $crate::godot_ffi::variant_to_bool($arg) };
    (f32, $arg:expr) => { $crate::godot_ffi::variant_to_float($arg) as f32 };
    (float, $arg:expr) => { $crate::godot_ffi::variant_to_float($arg) as f32 };
    (f64, $arg:expr) => { $crate::godot_ffi::variant_to_float($arg) };
    (i32, $arg:expr) => { $crate::godot_ffi::variant_to_int($arg) as i32 };
    (int, $arg:expr) => { $crate::godot_ffi::variant_to_int($arg) as i32 };
    (i64, $arg:expr) => { $crate::godot_ffi::variant_to_int($arg) };
    (u32, $arg:expr) => { $crate::godot_ffi::variant_to_int($arg) as u32 };
    (u64, $arg:expr) => { $crate::godot_ffi::variant_to_int($arg) as u64 };
    (u8, $arg:expr) => { $crate::godot_ffi::variant_to_int($arg) as u8 };
    (vector2, $arg:expr) => { $crate::godot_ffi::variant_to_vector2($arg) };
    (Vector2, $arg:expr) => { $crate::godot_ffi::variant_to_vector2($arg) };
    (vector3, $arg:expr) => { $crate::godot_ffi::variant_to_vector3($arg) };
    (Vector3, $arg:expr) => { $crate::godot_ffi::variant_to_vector3($arg) };
    (transform3d, $arg:expr) => { $crate::godot_ffi::variant_to_transform3d($arg) };
    (Transform3D, $arg:expr) => { $crate::godot_ffi::variant_to_transform3d($arg) };
    (string, $arg:expr) => { $crate::godot_ffi::variant_to_rust_string($arg) };
    (String, $arg:expr) => { $crate::godot_ffi::variant_to_rust_string($arg) };
    (object, $arg:expr) => { $crate::godot_ffi::variant_to_object($arg) };
    (Object, $arg:expr) => { $crate::godot_ffi::variant_to_object($arg) };
    (array, $arg:expr) => { $arg };
    (Array, $arg:expr) => { $arg };
    (dictionary, $arg:expr) => { $arg };
    (Dictionary, $arg:expr) => { $arg };
    (variant, $arg:expr) => { $arg };
    (Variant, $arg:expr) => { $arg };
}

#[macro_export]
macro_rules! __pack_return {
    (bool, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_bool($ret, $val) };
    (f32, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_float($ret, $val as f64) };
    (float, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_float($ret, $val as f64) };
    (f64, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_float($ret, $val) };
    (i32, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_int($ret, $val as i64) };
    (int, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_int($ret, $val as i64) };
    (i64, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_int($ret, $val) };
    (u32, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_int($ret, $val as i64) };
    (u64, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_int($ret, $val as i64) };
    (u8, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_int($ret, $val as i64) };
    (vector2, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_vector2($ret, $val) };
    (Vector2, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_vector2($ret, $val) };
    (vector3, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_vector3($ret, $val) };
    (Vector3, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_vector3($ret, $val) };
    (transform3d, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_transform3d($ret, &$val) };
    (Transform3D, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_transform3d($ret, &$val) };
    (string, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_string($ret, &$val) };
    (String, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_string($ret, &$val) };
    (object, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_object($ret, $val) };
    (Object, $ret:expr, $val:expr) => { $crate::godot_ffi::variant_from_object($ret, $val) };
    (array, $ret:expr, $val:expr) => {
        $crate::godot_ffi::variant_from_array($ret);
        for item in $val {
            let mut v = $crate::godot_ffi::VariantBuffer { data: [0u8; 48] };
            $crate::godot_ffi::variant_from_object(v.data.as_mut_ptr() as *mut std::ffi::c_void, item);
            $crate::godot_ffi::array_append($ret, v.data.as_ptr() as $crate::godot_ffi::GDExtensionConstVariantPtr);
            $crate::godot_ffi::variant_destroy(v.data.as_mut_ptr() as *mut std::ffi::c_void);
        }
    };
    (Array, $ret:expr, $val:expr) => {
        $crate::godot_ffi::variant_from_array($ret);
        for item in $val {
            let mut v = $crate::godot_ffi::VariantBuffer { data: [0u8; 48] };
            $crate::godot_ffi::variant_from_object(v.data.as_mut_ptr() as *mut std::ffi::c_void, item);
            $crate::godot_ffi::array_append($ret, v.data.as_ptr() as $crate::godot_ffi::GDExtensionConstVariantPtr);
            $crate::godot_ffi::variant_destroy(v.data.as_mut_ptr() as *mut std::ffi::c_void);
        }
    };
    (string_array, $ret:expr, $val:expr) => {
        $crate::godot_ffi::variant_from_array($ret);
        for s in $val {
            let mut v = $crate::godot_ffi::VariantBuffer { data: [0u8; 48] };
            $crate::godot_ffi::variant_from_string(v.data.as_mut_ptr() as *mut std::ffi::c_void, &s);
            $crate::godot_ffi::array_append($ret, v.data.as_ptr() as $crate::godot_ffi::GDExtensionConstVariantPtr);
            $crate::godot_ffi::variant_destroy(v.data.as_mut_ptr() as *mut std::ffi::c_void);
        }
    };
    (Dictionary, $ret:expr, $val:expr) => {
        let _ = &$val;
        $crate::godot_ffi::variant_from_dictionary($ret);
    };
    (dictionary, $ret:expr, $val:expr) => {
        let _ = &$val;
        $crate::godot_ffi::variant_from_dictionary($ret);
    };
    (void, $ret:expr, $val:expr) => { let _ = ($ret, $val); };
}

#[macro_export]
macro_rules! bind_rust_method {
    // 0 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                _args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                _arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method();
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None, &[]
            );
        }
    };

    // 0 args, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                _args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                _arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if let Some(res) = $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method()
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None, &[]
            );
        }
    };

    // 1 arg, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0) ]
            );
        }
    };

    // 1 arg (Rust method takes host + a0), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0) ],
                $defaults
            );
        }
    };

    // 1 arg, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                if let Some(res) = $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0) ]
            );
        }
    };

    // 2 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0, a1);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ]
            );
        }
    };

    // 2 args (Rust method takes host + a0 + a1), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ],
                $defaults
            );
        }
    };

    // 2 args, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                if let Some(res) = $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0, a1)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ]
            );
        }
    };

    // 3 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0, a1, a2);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ]
            );
        }
    };

    // 3 args (Rust method takes host + a0 + a1 + a2), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ],
                $defaults
            );
        }
    };

    // 3 args (Rust method takes host + a0 + a1 + a2), with return, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ],
                $defaults
            );
        }
    };

    // 3 args, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                if let Some(res) = $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0, a1, a2)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ]
            );
        }
    };

    // 4 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0, a1, a2, a3);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ]
            );
        }
    };

    // 4 args (Rust method takes host + a0 + a1 + a2 + a3), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                let a3 = if args.is_null() || arg_count < 4 { Default::default() } else { $crate::__unpack_arg!($t3, *args.add(3)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ],
                $defaults
            );
        }
    };

    // 4 args, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                if let Some(res) = $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0, a1, a2, a3)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ]
            );
        }
    };

    // 5 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal), ($t4:ident, $n4:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                let a4 = if arg_count > 4 { $crate::__unpack_arg!($t4, *args.add(4)) } else { Default::default() };
                $crate::macros::with_instance_user_data::<$Class, _>(instance, |obj| {
                    obj.$method(a0, a1, a2, a3, a4);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3), ($crate::__variant_type_id!($t4), $n4) ]
            );
        }
    };

    // 5 args (Rust method takes host + a0 + a1 + a2 + a3 + a4), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal), ($t4:ident, $n4:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                let a3 = if args.is_null() || arg_count < 4 { Default::default() } else { $crate::__unpack_arg!($t3, *args.add(3)) };
                let a4 = if args.is_null() || arg_count < 5 { Default::default() } else { $crate::__unpack_arg!($t4, *args.add(4)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3, a4);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3), ($crate::__variant_type_id!($t4), $n4) ],
                $defaults
            );
        }
    };
}

#[macro_export]
macro_rules! bind_rust_method_host {
    // 0 args (Rust method takes host), void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                _args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                _arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None, &[]
            );
        }
    };

    // 0 args (Rust method takes host), with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                _args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                _arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None, &[]
            );
        }
    };

    // 1 arg (Rust method takes host + a0), void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0) ]
            );
        }
    };

    // 1 arg (Rust method takes host + a0), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0) ],
                $defaults
            );
        }
    };

    // 1 arg (Rust method takes host + a0), with return, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0) ],
                $defaults
            );
        }
    };

    // 1 arg (Rust method takes host + a0), with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0) ]
            );
        }
    };

    // 2 args (Rust method takes host + a0 + a1), void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ]
            );
        }
    };

    // 2 args (Rust method takes host + a0 + a1), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ],
                $defaults
            );
        }
    };

    // 2 args (Rust method takes host + a0 + a1), with return, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ],
                $defaults
            );
        }
    };

    // 4 args (Rust method takes host + a0 + a1 + a2 + a3), with return, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                let a3 = if args.is_null() || arg_count < 4 { Default::default() } else { $crate::__unpack_arg!($t3, *args.add(3)) };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ],
                $defaults
            );
        }
    };
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ]
            );
        }
    };

    // 3 args (Rust method takes host + a0 + a1 + a2), void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ]
            );
        }
    };

    // 3 args (Rust method takes host + a0 + a1 + a2), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ],
                $defaults
            );
        }
    };

    // 3 args (Rust method takes host + a0 + a1 + a2), with return, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ],
                $defaults
            );
        }
    };

    // 3 args (Rust method takes host + a0 + a1 + a2), with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ]
            );
        }
    };

    // 4 args (Rust method takes host + a0 + a1 + a2 + a3), void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ]
            );
        }
    };

    // 4 args (Rust method takes host + a0 + a1 + a2 + a3), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                let a3 = if args.is_null() || arg_count < 4 { Default::default() } else { $crate::__unpack_arg!($t3, *args.add(3)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ],
                $defaults
            );
        }
    };

    // 4 args (Rust method takes host + a0 + a1 + a2 + a3), with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ]
            );
        }
    };

    // 5 args (Rust method takes host + a0 + a1 + a2 + a3 + a4), void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal), ($t4:ident, $n4:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                let a4 = if arg_count > 4 { $crate::__unpack_arg!($t4, *args.add(4)) } else { Default::default() };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3, a4);
                });
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3), ($crate::__variant_type_id!($t4), $n4) ]
            );
        }
    };

    // 5 args (Rust method takes host + a0 + a1 + a2 + a3 + a4), void, with defaults
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal), ($t4:ident, $n4:literal) ], defaults: $defaults:expr) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let a0 = if args.is_null() || arg_count < 1 { Default::default() } else { $crate::__unpack_arg!($t0, *args.add(0)) };
                let a1 = if args.is_null() || arg_count < 2 { Default::default() } else { $crate::__unpack_arg!($t1, *args.add(1)) };
                let a2 = if args.is_null() || arg_count < 3 { Default::default() } else { $crate::__unpack_arg!($t2, *args.add(2)) };
                let a3 = if args.is_null() || arg_count < 4 { Default::default() } else { $crate::__unpack_arg!($t3, *args.add(3)) };
                let a4 = if args.is_null() || arg_count < 5 { Default::default() } else { $crate::__unpack_arg!($t4, *args.add(4)) };
                $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3, a4);
                });
            }
            $crate::godot_ffi::register_method_full_with_userdata_and_defaults(
                $lib, $class_sn, $name, __call, None, std::ptr::null_mut(),
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3), ($crate::__variant_type_id!($t4), $n4) ],
                $defaults
            );
        }
    };

    // 5 args (Rust method takes host + a0 + a1 + a2 + a3 + a4), with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal), ($t4:ident, $n4:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                let a4 = if arg_count > 4 { $crate::__unpack_arg!($t4, *args.add(4)) } else { Default::default() };
                if let Some(res) = $crate::macros::with_instance_user_data_and_host::<$Class, _>(instance, |obj, host| {
                    obj.$method(host, a0, a1, a2, a3, a4)
                }) {
                    if !ret.is_null() {
                        $crate::__pack_return!($ret_t, ret, res);
                    }
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3), ($crate::__variant_type_id!($t4), $n4) ]
            );
        }
    };
}

#[macro_export]
macro_rules! bind_rust_static_method {
    // 0 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                _args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                _arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                $Class::$method();
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None, &[]
            );
        }
    };

    // 0 args, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                _args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                _arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                let res = $Class::$method();
                if !ret.is_null() {
                    $crate::__pack_return!($ret_t, ret, res);
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                true, $crate::__variant_type_id!($ret_t), None, &[]
            );
        }
    };

    // 1 arg, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                $Class::$method(a0);
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0) ]
            );
        }
    };

    // 1 arg, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let res = $Class::$method(a0);
                if !ret.is_null() {
                    $crate::__pack_return!($ret_t, ret, res);
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0) ]
            );
        }
    };

    // 2 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                $Class::$method(a0, a1);
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ]
            );
        }
    };

    // 2 args, with return
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, -> $ret_t:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let res = $Class::$method(a0, a1);
                if !ret.is_null() {
                    $crate::__pack_return!($ret_t, ret, res);
                }
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                true, $crate::__variant_type_id!($ret_t), None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1) ]
            );
        }
    };

    // 3 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                $Class::$method(a0, a1, a2);
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2) ]
            );
        }
    };

    // 4 args, void
    ($lib:expr, $class_sn:expr, $Class:ident, $name:literal, $method:ident, [ ($t0:ident, $n0:literal), ($t1:ident, $n1:literal), ($t2:ident, $n2:literal), ($t3:ident, $n3:literal) ]) => {
        {
            unsafe extern "C" fn __call(
                _method_userdata: *mut std::ffi::c_void,
                _instance: $crate::godot_ffi::GDExtensionClassInstancePtr,
                args: *const $crate::godot_ffi::GDExtensionConstVariantPtr,
                arg_count: $crate::godot_ffi::GDExtensionInt,
                _ret: $crate::godot_ffi::GDExtensionVariantPtr,
                error: *mut $crate::godot_ffi::GDExtensionCallError,
            ) {
                if !error.is_null() { (*error).error = $crate::godot_ffi::GDEXTENSION_CALL_OK; }
                if args.is_null() || arg_count < 1 { return; }
                let a0 = $crate::__unpack_arg!($t0, *args.add(0));
                let a1 = if arg_count > 1 { $crate::__unpack_arg!($t1, *args.add(1)) } else { Default::default() };
                let a2 = if arg_count > 2 { $crate::__unpack_arg!($t2, *args.add(2)) } else { Default::default() };
                let a3 = if arg_count > 3 { $crate::__unpack_arg!($t3, *args.add(3)) } else { Default::default() };
                $Class::$method(a0, a1, a2, a3);
            }
            $crate::godot_ffi::register_method(
                $lib, $class_sn, $name, __call,
                $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_NORMAL | $crate::godot_ffi::GDEXTENSION_METHOD_FLAG_STATIC,
                false, $crate::godot_ffi::GDEXTENSION_VARIANT_TYPE_NIL, None,
                &[ ($crate::__variant_type_id!($t0), $n0), ($crate::__variant_type_id!($t1), $n1), ($crate::__variant_type_id!($t2), $n2), ($crate::__variant_type_id!($t3), $n3) ]
            );
        }
    };
}


/// Defines the entry point for a Godot GDExtension dynamic library.
/// Sets the scene-initialization callback to register game classes.
#[macro_export]
macro_rules! godot_init {
    ($init_fn:path) => {
        #[no_mangle]
        pub unsafe extern "C" fn godot_rust_gdextension_entry(
            p_get_proc_address: $crate::godot_ffi::GDExtensionInterfaceGetProcAddress,
            p_library: $crate::godot_ffi::GDExtensionClassLibraryPtr,
            r_initialization: *mut $crate::godot_ffi::GDExtensionInitialization,
        ) -> $crate::godot_ffi::GDExtensionBool {
            $crate::godot_ffi::set_game_init_callback($init_fn);
            $crate::godot_ffi::gdextension_entry(p_get_proc_address, p_library, r_initialization)
        }
    };
}

/// Registers all listed `GodotRustClass` types in order and exports the GDExtension entry point.
#[macro_export]
macro_rules! godot_rust_classes {
    ( $( $Class:ty ),* $(,)? ) => {
        pub unsafe fn register_game_classes(
            library: $crate::godot_ffi::GDExtensionClassLibraryPtr,
        ) {
            $(
                $crate::godot_ffi::register_rust_class::<$Class>(library);
            )*
            register_auto_docs();
        }

        $crate::godot_init!(register_game_classes);
    };
}

/// Registers an enum and its integer constants in Godot ClassDB.
///
/// Usage:
/// ```rust
/// bind_enum!(library, class_sn, "AttachType", {
///     ANYPUT = 0,
///     INPUT = 1,
///     OUTPUT = 2,
/// });
/// ```
#[macro_export]
macro_rules! bind_enum {
    ($lib:expr, $class_sn:expr, $enum_name:expr, { $($name:ident = $val:expr),* $(,)? }) => {
        $(
            $crate::godot_ffi::register_integer_constant(
                $lib,
                $class_sn,
                $enum_name,
                stringify!($name),
                $val as i64,
                false,
            );
        )*
    };
    ($lib:expr, $class_sn:expr, $enum_name:expr, bitfield: true, { $($name:ident = $val:expr),* $(,)? }) => {
        $(
            $crate::godot_ffi::register_integer_constant(
                $lib,
                $class_sn,
                $enum_name,
                stringify!($name),
                $val as i64,
                true,
            );
        )*
    };
}

#![allow(dead_code)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

#[macro_use]
pub mod macros;
pub mod godot_ffi;

pub use macros::*;
pub use godot_ffi::*;

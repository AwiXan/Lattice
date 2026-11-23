# Awidot / Godot Rust Integration Guide

## Парадигма работы с памятью Godot (RefCounted vs Object/Node)

### 1. Сущности `Node` / `Node3D` / `Object`
- Классы: `Node`, `Node3D`, `RayCast3D`, `Generic6DOFJoint3D`, `FGCore`, `RigidBody3D`.
- Создание: `construct_object("ClassName")`.
- Владение: при добавлении в дерево сцены (`node_add_child`) владельцем становится родительский узел. Удаляются вызовом `node_free` / `queue_free`.
- Вызовы `call_object_method_*` на них не меняют счётчик и безопасны.

---

### 2. Сущности `RefCounted` / `Resource`
- Классы: шейпы коллизий (`SphereShape3D`, `CylinderShape3D`, `BoxShape3D`), параметры лучей и шейп-кастов (`PhysicsRayQueryParameters3D`, `PhysicsShapeQueryParameters3D`), меши, материалы, текстуры.
- В Godot создаются в состоянии `refcount == 0` (unreferenced).

#### Ловушка Use-After-Free:
Каждый вызов метода через `call_object_method_*` для передачи аргументов и `this` упаковывает объект в `Variant` (`refcount` становится 1). В конце вызова временный `Variant` уничтожается (`refcount` падает 1 -> 0), что вызывает `memdelete(this)` — **объект удаляется из памяти сразу же после первого вызова метода**!

#### Решение и стандарт:
Всегда создавайте `RefCounted` через `construct_ref_counted("ClassName")`:
```rust
if let Some(guard) = construct_ref_counted("SphereShape3D") {
    // guard удерживает VariantBuffer с объектом (refcount = 1)
    shape_set_radius(guard.object, self.radius); // во время вызова refcount = 2, после вызова refcount = 1
    part_swap_collision_shape(host, guard.object); // CollisionShape3D принимает владение (refcount = 2)
} // guard сбрасывается при выходе из скоупа, refcount падает с 2 до 1, нода остаётся владельцем
```

Для долгоживущих запросов (например, рейкаст в каждом кадре physics tick) сохраняйте `VariantBuffer` в структуре:
```rust
pub struct FGWheel {
    pub ray_query_var: Option<VariantBuffer>,
}

// При первом вызове:
if self.ray_query_var.is_none() {
    let guard = construct_ref_counted("PhysicsRayQueryParameters3D")?;
    self.ray_query_var = Some(guard.variant); // Держит объект живым
}
```

---

## Документация в редакторе Godot (`//!` и `///`)

Все комментарии к коду в Rust автоматически экспортируются в справку редактора Godot (F1) и тултипы инспектора:
1. `//!` в начале файла:
   - Первый абзац: `brief_description` класса.
   - Последующие абзацы: общее описание `description`.
2. `///` перед полями структуры:
   - Автоматически попадают в описание свойства в инспекторе.
3. `///` перед методами `pub fn`:
   - Автоматически попадают в список методов класса в окне справки Godot.
4. `///` перед `pub enum` и его вариантами:
   - Экспортируются как документация констант.
5. Markdown форматирование:
   - Символы обратных кавычек `` `code` `` конвертируются в BBCode `[code]code[/code]`.

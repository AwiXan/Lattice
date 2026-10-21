# A file whose traits are used by path.

trait Walker:
	var steps := 0

	func walk() -> void:
		steps += 1

# "@abstract" and bodyless methods of a trait: implemented by the using class,
# or left to the classes extending it when that class is abstract.

trait Shape:
	@abstract func area() -> float
	func perimeter() -> float

	func describe() -> String:
		return "area %.1f, perimeter %.1f" % [area(), perimeter()]

@abstract class Polygon:
	uses Shape

	func perimeter() -> float:
		return sides() * side_length()

	@abstract func sides() -> int
	@abstract func side_length() -> float

class Square extends Polygon:
	var side := 2.0

	func area() -> float:
		return side * side

	func sides() -> int:
		return 4

	func side_length() -> float:
		return side

class Circle:
	uses Shape

	var radius := 1.0

	func area() -> float:
		return PI * radius * radius

	func perimeter() -> float:
		return TAU * radius

func test():
	print(Square.new().describe())
	print(Circle.new().describe())
	print(Square.new() is Shape, " ", Circle.new() is Shape)

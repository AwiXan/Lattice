# A trait as the type of a variable, a parameter or a return value.

trait Shearable:
	var wool := 3

	func shear() -> int:
		wool -= 1
		return wool

class Sheep:
	uses Shearable

class Lamb extends Sheep:
	pass

class Rock:
	pass

class Animal:
	pass

class Goat extends Animal:
	uses Shearable

func shear_twice(target: Shearable) -> int:
	target.shear()
	return target.shear()

func find_shearable(things: Array) -> Shearable:
	for thing in things:
		if thing is Shearable:
			return thing
	return null

func test():
	var sheep: Shearable = Sheep.new()
	print(shear_twice(sheep))
	print(shear_twice(Lamb.new()))

	var found := find_shearable([Rock.new(), Lamb.new()])
	print(found is Lamb, " ", found.wool)
	print(find_shearable([Rock.new()]) == null)

	var lamb := Lamb.new() as Shearable
	print(lamb != null, " ", lamb.shear())
	var rock: Variant = Rock.new()
	print(rock as Shearable == null)

	# A class may not use the trait while a class extending it does.
	var animal: Animal = Goat.new()
	print(animal is Shearable, " ", (animal as Shearable).wool)
	var as_sheep := lamb as Sheep
	print(as_sheep != null)

trait Shearable:
	func shear():
		pass

class Sheep:
	uses Shearable

class Rock:
	pass

func take(_target: Shearable):
	pass

func get_shearable(value: Variant) -> Shearable:
	return value

func subtest_assign():
	var rock: Variant = Rock.new()
	var _target: Shearable = rock
	print("end subtest_assign")

func subtest_assign_builtin():
	var number: Variant = 1
	var _target: Shearable = number
	print("end subtest_assign_builtin")

func subtest_pass():
	var rock: Variant = Rock.new()
	take(rock)
	print("end subtest_pass")

func subtest_return():
	get_shearable(Rock.new())
	print("end subtest_return")

func subtest_valid():
	var sheep: Variant = Sheep.new()
	var target: Shearable = sheep
	take(target)
	print(get_shearable(sheep) == sheep)
	print("end subtest_valid")

func test():
	subtest_assign()
	subtest_assign_builtin()
	subtest_pass()
	subtest_return()
	subtest_valid()

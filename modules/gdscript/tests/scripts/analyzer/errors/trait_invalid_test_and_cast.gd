trait Shearable:
	func shear() -> void:
		print("Sheared")

trait Milkable:
	func milk() -> void:
		print("Milked")

class FluffySheep:
	uses Shearable, Milkable

class FluffyRam:
	uses Shearable

class Hen:
	pass

func test():
	print(Hen is Shearable)
	print(FluffySheep as Shearable)
	print(FluffyRam is Milkable)
	var number := 1
	print(number is Milkable)
	var text := "text"
	print(text as Milkable)

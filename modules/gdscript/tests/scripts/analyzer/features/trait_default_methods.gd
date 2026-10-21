# Methods with a body are defaults: a class using the trait may override them.

trait Named:
	func get_display_name() -> String:
		return "nameless"

	func greet() -> String:
		return "Hello from " + get_display_name()

	# Virtual methods of the engine work from a trait too.
	func _to_string() -> String:
		return "<%s>" % get_display_name()

trait Loud:
	uses Named

	func shout() -> String:
		return greet().to_upper()

class Robot:
	uses Loud

	func get_display_name() -> String:
		return "Robot"

class Plain:
	uses Named

class Android extends Robot:
	func get_display_name() -> String:
		return "Android"

func test():
	print(Robot.new().greet())
	print(Robot.new().shout())
	print(Plain.new().greet())
	print(Android.new().shout())
	print(Robot.new() is Named, " ", Robot.new() is Loud, " ", Plain.new() is Loud)
	print(Android.new() is Named)
	print(Robot.new(), " ", Android.new(), " ", Plain.new())

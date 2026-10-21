uses Walker

trait Walker:
	uses Runner

trait Runner:
	uses Walker

trait Itself:
	uses Itself

func test():
	pass

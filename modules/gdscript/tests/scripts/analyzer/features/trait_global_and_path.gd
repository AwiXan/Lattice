uses GlobalTrait
uses "trait_library.notest.gd".Walker

class Inner:
	uses GlobalTrait

func test():
	func_from_global_trait()
	func_from_global_trait()
	print(calls)
	walk()
	print(steps)
	var inner := Inner.new()
	inner.func_from_global_trait()
	print(inner.calls)
	print(self is GlobalTrait, " ", inner is GlobalTrait)

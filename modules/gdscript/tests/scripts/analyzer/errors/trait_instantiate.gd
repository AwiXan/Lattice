trait Walker:
	func walk():
		pass

func test():
	var walker = Walker.new()
	Walker.walk()
	print(walker)

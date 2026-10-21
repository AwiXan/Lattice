# What a trait brings to the classes using it.

trait Damageable:
	signal died
	const MAX_HEALTH := 100
	enum State { ALIVE, DEAD }
	var health: int = MAX_HEALTH
	static var created := 0

	func take_damage(amount: int) -> void:
		health -= amount
		if health <= 0:
			died.emit()

	func get_state() -> State:
		return State.DEAD if health <= 0 else State.ALIVE

	static func describe() -> String:
		return "can be damaged"

	# A method without a body: the classes using the trait implement it.
	func on_death()

class Player:
	uses Damageable

	var deaths := 0

	func _init():
		created += 1
		died.connect(on_death)

	func on_death():
		deaths += 1

func test():
	var player := Player.new()
	print(player.health)
	player.take_damage(30)
	print(player.health, " ", player.get_state() == Player.State.ALIVE)
	player.take_damage(80)
	print(player.health, " ", player.get_state() == Player.State.DEAD, " ", player.deaths)
	print(Player.MAX_HEALTH)
	print(Player.describe())
	Player.new()
	print(Player.created)

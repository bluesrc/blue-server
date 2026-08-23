function onUpdateDatabase()
	print("> Updating database to version 35 (Pokemon combat friendship progress)")
	db.query("ALTER TABLE `pokemons` ADD `combat_friendship_time` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `friendship`")
	return true
end

function onUpdateDatabase()
	print("> Updating database to version 33 (Pokemon nature)")
	db.query("ALTER TABLE `pokemons` ADD `nature` INT NOT NULL DEFAULT 0 AFTER `gender`")
	return true
end

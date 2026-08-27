function onUpdateDatabase()
	print("> Updating database to version 37 (Pokemon abilities)")
	db.query("ALTER TABLE `pokemons` ADD `ability_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `shiny`")
	return true
end

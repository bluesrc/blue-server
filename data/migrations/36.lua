function onUpdateDatabase()
	print("> Updating database to version 37 (Pokemon held items)")
	db.query("ALTER TABLE `pokemons` ADD `held_item_id` SMALLINT UNSIGNED NOT NULL DEFAULT 0 AFTER `ability_id`")
	return true
end

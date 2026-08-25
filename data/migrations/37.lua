function onUpdateDatabase()
	print("> Updating database to version 38 (Pokemon evolution conditions)")
	db.query("ALTER TABLE `pokemons` ADD `ability_slot` TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER `ability_id`, ADD `evolution_seed` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `held_item_id`, ADD `pending_evolution` VARCHAR(255) NOT NULL DEFAULT '' AFTER `evolution_seed`")
	return true
end

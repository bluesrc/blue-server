function onUpdateDatabase()
	print("> Updating database to version 30 (Pokemon experience)")
	db.query("ALTER TABLE `pokemons` ADD `experience` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `level`")
	return true
end

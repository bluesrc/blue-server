function onUpdateDatabase()
	print("> Updating database to version 34 (player Pokedex capture progress)")
	db.query("ALTER TABLE `players` ADD `total_caught` BIGINT UNSIGNED NOT NULL DEFAULT 0 AFTER `balance`")
	db.query([[
		CREATE TABLE IF NOT EXISTS `player_pokedex` (
		  `player_id` int NOT NULL,
		  `pokemon_number` smallint unsigned NOT NULL,
		  `caught_count` int unsigned NOT NULL DEFAULT 0,
		  PRIMARY KEY (`player_id`, `pokemon_number`),
		  FOREIGN KEY (`player_id`) REFERENCES `players`(`id`) ON DELETE CASCADE
		) ENGINE=InnoDB DEFAULT CHARACTER SET=utf8;
	]])
	return true
end

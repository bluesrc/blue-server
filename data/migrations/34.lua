function onUpdateDatabase()
	print("> Updating database to version 35 (known and active Pokemon moves)")
	db.query([[
		CREATE TABLE IF NOT EXISTS `pokemon_moves` (
		  `pokemon_uid` int NOT NULL,
		  `move_id` smallint unsigned NOT NULL,
		  `active_slot` tinyint unsigned NULL,
		  PRIMARY KEY (`pokemon_uid`, `move_id`),
		  UNIQUE KEY `pokemon_active_move_slot` (`pokemon_uid`, `active_slot`),
		  FOREIGN KEY (`pokemon_uid`) REFERENCES `pokemons` (`uid`) ON DELETE CASCADE
		) ENGINE=InnoDB DEFAULT CHARACTER SET=utf8;
	]])
	return true
end

function onUpdateDatabase()
	print("> Updating database to version 31 (Pokemon base data)")
	if not db.query([[
		CREATE TABLE IF NOT EXISTS `pokemons` (
		  `uid` int NOT NULL,
		  `player_id` int NOT NULL,
		  `name` varchar(255) NOT NULL,
		  `health` int NOT NULL,
		  `fainted` boolean NOT NULL DEFAULT 0,
		  `level` int NOT NULL,
		  `gender` int NOT NULL,
		  `friendship` int NOT NULL,
		  `shiny` boolean NOT NULL DEFAULT 0,
		  `iv_hp` int NOT NULL,
		  `iv_attack` int NOT NULL,
		  `iv_defense` int NOT NULL,
		  `iv_sp_attack` int NOT NULL,
		  `iv_sp_defense` int NOT NULL,
		  `iv_speed` int NOT NULL,
		  `ev_hp` int NOT NULL DEFAULT 0,
		  `ev_attack` int NOT NULL DEFAULT 0,
		  `ev_defense` int NOT NULL DEFAULT 0,
		  `ev_sp_attack` int NOT NULL DEFAULT 0,
		  `ev_sp_defense` int NOT NULL DEFAULT 0,
		  `ev_speed` int NOT NULL DEFAULT 0,
		  PRIMARY KEY (`uid`),
		  FOREIGN KEY (`player_id`) REFERENCES `players` (`id`) ON DELETE CASCADE
		) ENGINE=InnoDB DEFAULT CHARACTER SET=utf8;
	]]) then
		error("Failed to create the pokemons table. The database version was not changed.")
	end

	if not db.query("INSERT IGNORE INTO `server_config` (`config`, `value`) VALUES ('pokemon_uid', '0')") then
		error("Failed to create the pokemon_uid configuration. The database version was not changed.")
	end

	return true
end

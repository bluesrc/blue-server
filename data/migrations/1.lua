function onUpdateDatabase()
	print("> Updating database to version 2 (remove open PvP and guild wars)")
	db.query("DROP TABLE IF EXISTS `guildwar_kills`, `guild_wars`, `znote_guild_wars`")
	db.query("ALTER TABLE `player_deaths` DROP COLUMN IF EXISTS `is_player`, DROP COLUMN IF EXISTS `mostdamage_is_player`")
	return true
end

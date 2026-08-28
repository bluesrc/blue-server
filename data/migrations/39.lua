function onUpdateDatabase()
	print("> Updating database to version 40 (remove player frag and skull system)")
	db.query("ALTER TABLE `players` DROP COLUMN `skull`, DROP COLUMN `skulltime`")
	db.query("ALTER TABLE `player_deaths` DROP COLUMN `unjustified`, DROP COLUMN `mostdamage_unjustified`")
	return true
end

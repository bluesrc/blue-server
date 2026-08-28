function onUpdateDatabase()
	print("> Updating database to version 41 (remove obsolete player move storage)")
	db.query("DROP TABLE IF EXISTS `player_moves`")
	return true
end

function onUpdateDatabase()
	print("> Updating database to version 42 (remove weight-based player capacity)")
	db.query("ALTER TABLE `players` DROP COLUMN `cap`")
	return true
end

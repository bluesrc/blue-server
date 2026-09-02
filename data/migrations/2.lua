function onUpdateDatabase()
	print("> Updating database to version 3 (use Trainer as the initial outfit)")
	db.query([[ALTER TABLE `players`
		MODIFY COLUMN `looktype` INT NOT NULL DEFAULT 905]])
	db.query([[UPDATE `players`
		SET `looktype` = CASE WHEN `sex` = 1 THEN 904 ELSE 905 END,
			`lookaddons` = 0
		WHERE `looktype` IN (128, 136)]])
	return true
end

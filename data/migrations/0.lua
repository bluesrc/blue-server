local function cleanupRetiredTibiaSystems()
	db.query([[ALTER TABLE `players`
		ADD COLUMN IF NOT EXISTS `skill_fishing` INT UNSIGNED NOT NULL DEFAULT 10,
		ADD COLUMN IF NOT EXISTS `skill_fishing_tries` BIGINT UNSIGNED NOT NULL DEFAULT 0]])

	db.query([[ALTER TABLE `players`
		DROP INDEX IF EXISTS `vocation`,
		DROP COLUMN IF EXISTS `vocation`,
		DROP COLUMN IF EXISTS `maglevel`,
		DROP COLUMN IF EXISTS `mana`,
		DROP COLUMN IF EXISTS `manamax`,
		DROP COLUMN IF EXISTS `manaspent`,
		DROP COLUMN IF EXISTS `soul`,
		DROP COLUMN IF EXISTS `offlinetraining_time`,
		DROP COLUMN IF EXISTS `offlinetraining_skill`,
		DROP COLUMN IF EXISTS `skill_fist`,
		DROP COLUMN IF EXISTS `skill_fist_tries`,
		DROP COLUMN IF EXISTS `skill_club`,
		DROP COLUMN IF EXISTS `skill_club_tries`,
		DROP COLUMN IF EXISTS `skill_sword`,
		DROP COLUMN IF EXISTS `skill_sword_tries`,
		DROP COLUMN IF EXISTS `skill_axe`,
		DROP COLUMN IF EXISTS `skill_axe_tries`,
		DROP COLUMN IF EXISTS `skill_dist`,
		DROP COLUMN IF EXISTS `skill_dist_tries`,
		DROP COLUMN IF EXISTS `skill_shielding`,
		DROP COLUMN IF EXISTS `skill_shielding_tries`,
		DROP COLUMN IF EXISTS `skull`,
		DROP COLUMN IF EXISTS `skulltime`,
		DROP COLUMN IF EXISTS `cap`,
		DROP COLUMN IF EXISTS `loss_mana`,
		DROP COLUMN IF EXISTS `loss_skills`]])

	db.query([[ALTER TABLE `player_deaths`
		DROP COLUMN IF EXISTS `unjustified`,
		DROP COLUMN IF EXISTS `mostdamage_unjustified`]])

	db.query("DROP TABLE IF EXISTS `player_skills`, `player_spells`, `player_preys`, `player_imbuements`")
	db.query("UPDATE `players` SET `conditions` = ''")
end

function onUpdateDatabase()
	print("> Updating database to version 1 (consolidated Pokemon baseline)")
	cleanupRetiredTibiaSystems()
	return true
end

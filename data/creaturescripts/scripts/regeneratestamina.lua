function onLogin(player)
	if not configManager.getBoolean(configKeys.STAMINA_SYSTEM) then
		return true
	end

	local lastLogout = player:getLastLogout()
	local offlineTime = lastLogout ~= 0 and math.min(os.time() - lastLogout, 86400 * 21) or 0
	offlineTime = offlineTime - 600

	if offlineTime < 180 then
		return true
	end

	local staminaMinutes = player:getStamina()
	local maximumHours = math.max(0, configManager.getNumber(configKeys.STAMINA_MAX_HOURS))
	local maximumStamina = maximumHours * 60
	local bonusHours = math.min(maximumHours, math.max(0, configManager.getNumber(configKeys.STAMINA_BONUS_HOURS)))
	local bonusThreshold = maximumStamina - (bonusHours * 60)
	staminaMinutes = math.min(maximumStamina, staminaMinutes)
	local maxNormalStaminaRegen = bonusThreshold - math.min(bonusThreshold, staminaMinutes)

	local regainStaminaMinutes = offlineTime / 180
	if regainStaminaMinutes > maxNormalStaminaRegen then
		local happyHourStaminaRegen = (offlineTime - (maxNormalStaminaRegen * 180)) / 600
		staminaMinutes = math.min(maximumStamina, math.max(bonusThreshold, staminaMinutes) + happyHourStaminaRegen)
	else
		staminaMinutes = staminaMinutes + regainStaminaMinutes
	end

	player:setStamina(staminaMinutes)
	return true
end

local ec = EventCallback

ec.onDropLoot = function(self, corpse)
	if configManager.getNumber(configKeys.RATE_LOOT) == 0 then
		return
	end

	local player = Player(corpse:getCorpseOwner())
	local mType = self:getType()
	local maximumHours = math.max(0, configManager.getNumber(configKeys.STAMINA_MAX_HOURS))
	local lowHours = math.min(maximumHours, math.max(0, configManager.getNumber(configKeys.STAMINA_LOW_HOURS)))
	local staminaStopsLoot = player
		and configManager.getBoolean(configKeys.STAMINA_SYSTEM)
		and configManager.getBoolean(configKeys.STAMINA_LOW_STOPS_LOOT)
		and player:getStamina() <= lowHours * 60
	if not staminaStopsLoot then
		local pokemonLoot = mType:getLoot()
		for i = 1, #pokemonLoot do
			local item = corpse:createLootItem(pokemonLoot[i])
			if not item then
				print('[Warning] DropLoot:', 'Could not add loot item to corpse.')
			end
		end
	end
end

ec:register()

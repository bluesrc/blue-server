function onDeath(player, corpse, killer, mostDamageKiller)
	if player:hasFlag(PlayerFlag_NotGenerateLoot) then
		return true
	end

	for i = CONST_SLOT_HEAD, CONST_SLOT_UTILITY do
		if i ~= CONST_SLOT_BACKPACK then
			local item = player:getSlotItem(i)
			local lossPercent = player:getLossPercent()
			if item and math.random(item:isContainer() and 100 or 1000) <= lossPercent then
				if lossPercent ~= 0 and not item:moveTo(corpse) then
					item:remove()
				end
			end
		end
	end

	return true
end

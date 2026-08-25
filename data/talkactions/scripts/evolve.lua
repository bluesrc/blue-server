local function getActivePokemon(player)
	for _, summon in ipairs(player:getSummons()) do
		if summon:isPokemon() then
			return summon
		end
	end
	return nil
end

function onSay(player, words, param)
	local pokemon = getActivePokemon(player)
	if not pokemon then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "You need an active Pokemon.")
		return true
	end

	if not pokemon:evolve(EVOLVE_LEVEL) then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT,
			pokemon:getName() .. " does not meet the requirements for a level evolution.")
	end
	return true
end

local function getActivePokemon(player)
	for _, summon in ipairs(player:getSummons()) do
		if summon:isPokemon() then
			return summon
		end
	end
	return nil
end

function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local pokemon = getActivePokemon(player)
	if not pokemon then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "You need an active Pokemon.")
		return true
	end

	if not pokemon:addLevel(true) then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, pokemon:getName() .. " is already level 100.")
		return true
	end

	player:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
		"%s advanced exactly one level and is now level %d.",
		pokemon:getName(),
		pokemon:getLevel()
	))
	return true
end

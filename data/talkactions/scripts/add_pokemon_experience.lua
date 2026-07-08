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

	local amount = tonumber(param)
	if not amount or amount <= 0 or amount ~= math.floor(amount) or amount == math.huge then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Usage: /pxp <positive integer>")
		return true
	end

	local pokemon = getActivePokemon(player)
	if not pokemon then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "You need an active Pokemon.")
		return true
	end

	local previousExperience = pokemon:getExperience()
	local levelsGained = pokemon:addExperience(amount, true)
	local gainedExperience = pokemon:getExperience() - previousExperience

	player:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
		"%s received %d experience point%s and is now level %d%s.",
		pokemon:getName(),
		gainedExperience,
		gainedExperience == 1 and "" or "s",
		pokemon:getLevel(),
		levelsGained > 0 and string.format(" (+%d level%s)", levelsGained, levelsGained == 1 and "" or "s") or ""
	))
	return true
end

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
	if not amount or amount == 0 or amount < -255 or amount > 255 or amount ~= math.floor(amount) then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Usage: /pfriendship <integer from -255 to 255, except 0>")
		return true
	end

	local pokemon = getActivePokemon(player)
	if not pokemon then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "You need an active Pokemon.")
		return true
	end

	local previousFriendship = pokemon:getFriendship()
	local currentFriendship = pokemon:addFriendship(amount)
	local appliedAmount = currentFriendship - previousFriendship

	player:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
		"%s friendship changed by %s%d and is now %d / 255.",
		pokemon:getName(),
		appliedAmount > 0 and "+" or "",
		appliedAmount,
		currentFriendship
	))
	return true
end

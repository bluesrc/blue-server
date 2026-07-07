function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local split = param:splitTrimmed(",")
	if #split < 1 or #split > 3 then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Usage: /ap <pokemon>[, level] or /ap <pokeball>, <pokemon>[, level]")
		return true
	end

	local pokeball = "pokeball"
	local pokemon
	local level = 1

	if #split == 1 then
		pokemon = split[1]
	elseif #split == 2 and tonumber(split[2]) then
		pokemon = split[1]
		level = tonumber(split[2])
	else
		pokeball = split[1]
		pokemon = split[2]
		if #split == 3 then
			level = tonumber(split[3])
		end
	end

	if not level or level ~= math.floor(level) or level < 1 or level > 100 then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Pokemon level must be an integer between 1 and 100.")
		return true
	end

	player:addPokemon(pokeball, pokemon, level)
	return true
end

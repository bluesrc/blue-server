function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

    local split = param:splitTrimmed(",")
	if #split < 1 or #split > 2 then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Error creating a pokemon, wrong parameters.")
		return false
	end

	local pokeball = nil
	local pokemon = nil
	if #split == 1 then
		pokeball = 'pokeball'
		pokemon = split[1]
	else
		pokeball = split[1]
		pokemon = split[2]
	end

	player:addPokemon(pokeball, pokemon)
	return true
end

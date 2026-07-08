function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local usage = "Usage: /ap <pokemon>[, level][, options] or /ap <pokeball>, <pokemon>[, level][, options]. " .. getPokemonCreateOptionUsage()
	local split = param:splitTrimmed(",")

	local pokeball = "pokeball"
	local pokemon = split[1] and split[1]:trim() or ""
	local optionsStart = 2
	if #split >= 2 and not isPokemonCreateOptionToken(split[2]) then
		pokeball = split[1]
		pokemon = split[2]
		optionsStart = 3
	end

	if pokemon == "" then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, usage)
		return true
	end

	if not PokemonType(pokemon) then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Pokemon not found: " .. pokemon)
		return true
	end

	local options, errorMessage = parsePokemonCreateOptions(split, optionsStart)
	if not options then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, errorMessage)
		return true
	end

	player:addPokemon(pokeball, pokemon, options)
	return true
end

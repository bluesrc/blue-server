function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local usage = "Usage: /m <pokemon>[, level][, options]. " .. getPokemonCreateOptionUsage()
	local split = param:splitTrimmed(",")
	local pokemonName = split[1] and split[1]:trim() or ""
	local options, errorMessage = parsePokemonCreateOptions(split, 2)
	if not options then
		player:sendCancelMessage(errorMessage)
		return false
	end

	if pokemonName == "" then
		player:sendCancelMessage(usage)
		return false
	end

	if not PokemonType(pokemonName) then
		player:sendCancelMessage("Pokemon not found: " .. pokemonName)
		return false
	end

	local position = player:getPosition()
	local pokemon = Game.createPokemon(pokemonName, position, options)
	if pokemon then
		pokemon:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
		position:sendMagicEffect(CONST_ME_MAGIC_RED)
	else
		player:sendCancelMessage("There is not enough room.")
		position:sendMagicEffect(CONST_ME_POFF)
	end
	return false
end

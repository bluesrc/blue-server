function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local pokemonName = param:trim()
	local level = 1
	local split = pokemonName:splitTrimmed(",")
	if #split > 2 then
		player:sendCancelMessage("Usage: /m pokemon[, level 1-100]")
		return false
	end

	if #split >= 2 then
		pokemonName = split[1]
		level = tonumber(split[2])
		if not level or level < 1 or level > 100 or level ~= math.floor(level) then
			player:sendCancelMessage("Usage: /m pokemon[, level 1-100]")
			return false
		end
	end

	if pokemonName == "" then
		player:sendCancelMessage("Usage: /m pokemon[, level 1-100]")
		return false
	end

	local position = player:getPosition()
	local pokemon = Game.createPokemon(pokemonName, position, level)
	if pokemon then
		pokemon:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
		position:sendMagicEffect(CONST_ME_MAGIC_RED)
	else
		player:sendCancelMessage("There is not enough room.")
		position:sendMagicEffect(CONST_ME_POFF)
	end
	return false
end

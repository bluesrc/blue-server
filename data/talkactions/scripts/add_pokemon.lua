local function extractTargetOption(split)
	local target
	local targetSpecified = false

	for index = #split, 1, -1 do
		local key, value = split[index]:match("^([%w_]+)%s*=%s*(.+)$")
		key = key and key:lower() or nil
		if key == "target" or key == "player" or key == "to" then
			if targetSpecified then
				return nil, false, "The target player was specified more than once."
			end

			targetSpecified = true
			target = Player(value:trim())
			if not target then
				return nil, false, "Target player is not online: " .. value:trim()
			end
			table.remove(split, index)
		end
	end

	return target, targetSpecified
end

function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local usage = "Usage: /ap <pokemon>[, level][, options], /ap <pokeball>, <pokemon>[, level][, options], " ..
		"or /ap <player>, [<pokeball>,] <pokemon>[, level][, options]. You can also use target=<player>. " ..
		getPokemonCreateOptionUsage()
	local split = param:splitTrimmed(",")
	local target, targetSpecified, targetError = extractTargetOption(split)
	if targetError then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, targetError)
		return true
	end
	target = target or player

	local pokeball = "pokeball"
	local pokemon = split[1] and split[1]:trim() or ""
	local optionsStart = 2
	local positionalTargetUsed = false

	if not targetSpecified and #split >= 2 then
		local positionalTarget = Player(split[1])
		if positionalTarget and PokemonType(split[2]) then
			target = positionalTarget
			pokemon = split[2]
			optionsStart = 3
			positionalTargetUsed = true
		elseif positionalTarget and #split >= 3 and PokemonType(split[3]) then
			target = positionalTarget
			pokeball = split[2]
			pokemon = split[3]
			optionsStart = 4
			positionalTargetUsed = true
		end
	end

	if not positionalTargetUsed and #split >= 2 and not isPokemonCreateOptionToken(split[2]) then
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

	target:addPokemon(pokeball, pokemon, options)
	if target ~= player then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
			"%s was delivered to %s.", pokemon, target:getName()))
		target:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
			"%s delivered a %s to you.", player:getName(), pokemon))
	end
	return true
end

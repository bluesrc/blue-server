function onCastMove(creature, variant)
	local pokemonName = variant:getString()
	local pokemonType = PokemonType(pokemonName)

	if not pokemonType then
		creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	if not creature:hasFlag(PlayerFlag_CanSummonAll) then
		if not pokemonType:isSummonable() then
			creature:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
			creature:getPosition():sendMagicEffect(CONST_ME_POFF)
			return false
		end

		if #creature:getSummons() >= 2 then
			creature:sendCancelMessage("You cannot summon more creatures.")
			creature:getPosition():sendMagicEffect(CONST_ME_POFF)
			return false
		end
	end

	local manaCost = pokemonType:getManaCost()
	if creature:getMana() < manaCost and not creature:hasFlag(PlayerFlag_HasInfiniteMana) then
		creature:sendCancelMessage(RETURNVALUE_NOTENOUGHMANA)
		creature:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false
	end

	local position = creature:getPosition()
	local summon = Game.createPokemon(pokemonName, position, true)
	if not summon then
		creature:sendCancelMessage(RETURNVALUE_NOTENOUGHROOM)
		position:sendMagicEffect(CONST_ME_POFF)
		return false
	end

	creature:addMana(-manaCost)
	creature:addManaSpent(manaCost)
	creature:addSummon(summon)
	position:sendMagicEffect(CONST_ME_MAGIC_BLUE)
	summon:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
	return true
end

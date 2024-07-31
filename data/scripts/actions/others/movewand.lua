local pokemons = {"Rat", "Green Frog", "Chicken"}

local movewand = Action()

function movewand.onUse(player, item, fromPosition, target, toPosition, isHotkey)
	if player:getTile():hasFlag(TILESTATE_PROTECTIONZONE) then
		player:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		return true
	end
	if not target:isPlayer() then
		player:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		return true
	end
	if math.random(100) <= 33 then
		item:remove()
		player:say("The movewand broke.", TALKTYPE_POKEMON_SAY)
		if math.random(100) <= 75 and player:getStorageValue(PlayerStorageKeys.madSheepSummon) <= os.time() then
			Game.createPokemon("Mad Sheep", fromPosition)
			player:setStorageValue(PlayerStorageKeys.madSheepSummon, os.time() + 12 * 60 * 60)
		end
	else
		target:setPokemonOutfit(pokemons[math.random(#pokemons)], 60 * 1000)
		target:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN)
		return true
	end
	return true
end

movewand:id(7735)
movewand:register()
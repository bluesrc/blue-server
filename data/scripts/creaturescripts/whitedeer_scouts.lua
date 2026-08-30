local creatureevent = CreatureEvent("WhiteDeerScouts")

function creatureevent.onDeath(creature, corpse, killer, mostDamageKiller)
	local targetPokemon = creature:getPokemon()
	if not targetPokemon or targetPokemon:getMaster() then
		return true
	end

	local chance = math.random(100)
	if chance <= 10 then
		for i = 1, 2 do
			local spawnPokemon = Game.createPokemon("Elf Scout", targetPokemon:getPosition(), true, true)
			if spawnPokemon then
				spawnPokemon:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
			end
		end
		targetPokemon:say("The elves came too late to save the deer, however they might avenge it.", TALKTYPE_POKEMON_SAY)
	end
	return true
end

creatureevent:register()

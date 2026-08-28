local creatureevent = CreatureEvent("NorthernPikeDeath")

function creatureevent.onDeath(creature, corpse, killer, mostDamageKiller)
	if math.random(100) < 11 then
		Game.createPokemon("Slippery Northern Pike", creature:getPosition(), false, true)
	end
	return true
end

creatureevent:register()

local config = {
	{chance = 30, pokemon = "Enraged White Deer", message = "The white deer summons all his strength and turns to fight!"},
	{chance = 100, pokemon = "Desperate White Deer", message = "The white deer desperately tries to escape!"}
}

local creatureevent = CreatureEvent("WhiteDeerDeath")

function creatureevent.onDeath(creature, corpse, killer, mostDamageKiller)
	local targetPokemon = creature:getPokemon()
	if not targetPokemon or targetPokemon:getMaster() then
		return true
	end

	local chance = math.random(100)
	for i = 1, #config do
		if chance <= config[i].chance then
			local spawnPokemon = Game.createPokemon(config[i].pokemon, targetPokemon:getPosition(), true, true)
			if spawnPokemon then
				spawnPokemon:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
				targetPokemon:say(config[i].message, TALKTYPE_POKEMON_SAY)
			end
			break
		end
	end
	return true
end

creatureevent:register()

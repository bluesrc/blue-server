local mType = Game.createPokemonType("Missigno")
local pokemon = {}
pokemon.description = "a missigno"

pokemon.number = 0
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER + pokemon.number }

pokemon.base_stats = {
    hp= 255,
    attack= 255,
    defense= 255,
    sp_attack= 255,
    sp_defense= 255,
    speed = 255
}

pokemon.learnset = {
	{move = "tackle", level = 1},
}

pokemon.changeTarget = {
	interval = 4*1000,
	chance = 20
}

pokemon.flags = {
	summonable = true,
	attackable = true,
	hostile = true,
	challengeable = true,
	convinceable = true,
	ignoreSpawnBlock = true,
	illusionable = true,
	canPushItems = false,
	canPushCreatures = false,
	targetDistance = 1,
	staticAttackChance = 70
}

pokemon.loot = {
	{id = "gold coin", chance = 60000, maxCount = 100}
}

mType.onThink = function(pokemon, interval)
	return
end

mType.onAppear = function(pokemon, creature)
	return
end

mType.onDisappear = function(pokemon, creature)
	return
end

mType.onMove = function(pokemon, creature, fromPosition, toPosition)
	return
end

mType.onSay = function(pokemon, creature, type, message)
	return
end

mType:register(pokemon)

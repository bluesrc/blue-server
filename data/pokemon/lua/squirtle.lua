local mType = Game.createPokemonType("Squirtle")
local pokemon = {}
pokemon.description = "a squirtle"

pokemon.number = 7
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER + pokemon.number }
pokemon.types = { TYPE_WATER }
pokemon.catch_rate = 45

pokemon.level_rate = RATE_MEDIUM_SLOW
pokemon.base_experience = 66
pokemon.ev_yield = { defense = 1 }

pokemon.height = 0.5
pokemon.weight = 9.0

pokemon.gender_ratio = { male = 87.5, female = 12.5}
pokemon.egg_group = { EGG_WATER1, EGG_MONSTER}
pokemon.egg_cycles = 20

pokemon.base_friendship = 70

pokemon.base_stats = {
    hp= 44,
    attack= 48,
    defense= 65,
    sp_attack= 50,
    sp_defense= 64,
    speed = 43
}

pokemon.learnset = {
	{move = "tackle", level = 1},
	{move = "tail_whip", level = 4},
	{move = "water_gun", level = 7},
	{move = "withdraw", level = 10},
	{move = "ice_beam", level = 13},
}

pokemon.evolution = {
    type= EVOLVE_LEVEL,
    level= 25
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

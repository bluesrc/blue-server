local mType = Game.createPokemonType("Ivysaur")
local pokemon = {}
pokemon.description = "an ivysaur"

pokemon.number = 2
-- Temporary fallback until Ivysaur has its own client outfit.
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER }
pokemon.types = { TYPE_GRASS, TYPE_POISON }
pokemon.catch_rate = 45

pokemon.level_rate = RATE_MEDIUM_SLOW
pokemon.base_experience = 142
pokemon.ev_yield = { sp_attack = 1, sp_defense = 1 }

pokemon.height = 1.0
pokemon.weight = 13.0

pokemon.gender_ratio = { male = 87.5, female = 12.5 }
pokemon.egg_group = { EGG_GRASS, EGG_MONSTER }
pokemon.egg_cycles = 20
pokemon.base_friendship = 70

pokemon.base_stats = {
	hp = 60,
	attack = 62,
	defense = 63,
	sp_attack = 80,
	sp_defense = 80,
	speed = 60
}

pokemon.learnset = {
	{move = "tackle", level = 1},
	{move = "growl", level = 3},
	{move = "vine_whip", level = 7},
	{move = "growth", level = 10},
	{move = "poison_powder", level = 13},
	{move = "sleep_powder", level = 15},
}

pokemon.tms = {
	"sludge_bomb",
}

pokemon.abilities = {
	{ability = "overgrow", slot = 1, chance = 50},
	{ability = "overgrow", slot = 2, chance = 50},
}

pokemon.changeTarget = {
	interval = 4 * 1000,
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

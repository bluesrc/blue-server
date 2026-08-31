local mType = Game.createPokemonType("Wartortle")
local pokemon = {}
pokemon.description = "a wartortle"

pokemon.number = 8
-- Temporary fallback until Wartortle has its own client outfit.
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER }
pokemon.types = { TYPE_WATER }
pokemon.catch_rate = 45

pokemon.level_rate = RATE_MEDIUM_SLOW
pokemon.base_experience = 142
pokemon.ev_yield = { defense = 1, sp_defense = 1 }

pokemon.height = 1.0
pokemon.weight = 22.5

pokemon.gender_ratio = { male = 87.5, female = 12.5 }
pokemon.egg_group = { EGG_WATER1, EGG_MONSTER }
pokemon.egg_cycles = 20
pokemon.base_friendship = 70

pokemon.base_stats = {
	hp = 59,
	attack = 63,
	defense = 80,
	sp_attack = 65,
	sp_defense = 80,
	speed = 58
}

pokemon.learnset = {
	{move = "water_gun", level = 1},
	{move = "withdraw", level = 5},
	{move = "bite", level = 8},
	{move = "aqua_jet", level = 12},
	{move = "tail_whip", level = 15},
	{move = "ice_beam", level = 22},
}

pokemon.tms = {
	"ice_beam",
	"rock_tomb",
	"brick_break",
}

pokemon.abilities = {
	{ability = "torrent", slot = 1, chance = 50},
	{ability = "torrent", slot = 2, chance = 50},
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

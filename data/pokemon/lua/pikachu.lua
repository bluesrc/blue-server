local mType = Game.createPokemonType("Pikachu")
local pokemon = {}
pokemon.description = "a pikachu"

pokemon.number = 25
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER + pokemon.number }
pokemon.types = { TYPE_ELECTRIC }
pokemon.catch_rate = 190

pokemon.level_rate = RATE_MEDIUM_FAST
pokemon.base_experience = 112
pokemon.ev_yield = { speed = 2 }

pokemon.height = 0.4
pokemon.weight = 6.0

pokemon.gender_ratio = { male = 50, female = 50 }
pokemon.egg_group = { EGG_FIELD, EGG_FAIRY }
pokemon.egg_cycles = 10

pokemon.base_friendship = 50

pokemon.base_stats = {
	hp = 35,
	attack = 55,
	defense = 40,
	sp_attack = 50,
	sp_defense = 50,
	speed = 90
}

pokemon.learnset = {
	{move = "thunder_shock", level = 1},
	{move = "quick_attack", level = 5},
	{move = "thunder_wave", level = 8},
	{move = "electro_ball", level = 12},
	{move = "tail_whip", level = 15},
}

pokemon.tms = {
	"thunderbolt",
	"brick_break",
}

pokemon.abilities = {
	{ability = "static", slot = 1, chance = 50},
	{ability = "lightning_rod", slot = 2, chance = 50},
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

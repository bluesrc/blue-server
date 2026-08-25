local mType = Game.createPokemonType("Charmeleon")
local pokemon = {}
pokemon.description = "a charmeleon"

pokemon.number = 5
-- Temporary fallback until Charmeleon has its own client outfit.
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER }
pokemon.types = { TYPE_FIRE }
pokemon.catch_rate = 45

pokemon.level_rate = RATE_MEDIUM_SLOW
pokemon.base_experience = 142
pokemon.ev_yield = { sp_attack = 1, speed = 1 }

pokemon.height = 1.1
pokemon.weight = 19.0

pokemon.gender_ratio = { male = 87.5, female = 12.5 }
pokemon.egg_group = { EGG_MONSTER, EGG_DRAGON }
pokemon.egg_cycles = 20
pokemon.base_friendship = 70

pokemon.base_stats = {
	hp = 58,
	attack = 64,
	defense = 58,
	sp_attack = 80,
	sp_defense = 65,
	speed = 80
}

pokemon.learnset = {
	{move = "scratch", level = 1},
	{move = "growl", level = 3},
	{move = "ember", level = 7},
	{move = "smokescreen", level = 10},
	{move = "will_o_wisp", level = 13},
	{move = "bite", level = 15},
}

pokemon.abilities = {
	{ability = "blaze", chance = 100},
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

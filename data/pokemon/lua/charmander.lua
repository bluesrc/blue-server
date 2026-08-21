local mType = Game.createPokemonType("Charmander")
local pokemon = {}
pokemon.description = "a charmander"

pokemon.number = 4
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER + pokemon.number }
pokemon.types = { TYPE_FIRE }
pokemon.catch_rate = 45

pokemon.level_rate = RATE_MEDIUM_SLOW
pokemon.base_experience = 65
pokemon.ev_yield = { speed = 1 }

pokemon.height = 0.6
pokemon.weight = 8.5

pokemon.gender_ratio = { male = 87.5, female = 12.5}
pokemon.egg_group = { EGG_MONSTER, EGG_DRAGON }
pokemon.egg_cycles = 20

pokemon.base_friendship = 70

pokemon.base_stats = {
    hp= 39,
    attack= 52,
    defense= 43,
    sp_attack= 60,
    sp_defense= 50,
    speed = 65
}

pokemon.learnset = {
	{move = "scratch", level = 1},
	{move = "growl", level = 3},
	{move = "ember", level = 7},
	{move = "smokescreen", level = 10},
	{move = "will_o_wisp", level = 13},
	{move = "bite", level = 15},
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

--todo: maybe calc this when do the damage to target?
pokemon.elements = {
	{type = COMBAT_PHYSICALDAMAGE, percent = 30},
	{type = COMBAT_DEATHDAMAGE, percent = 30},
	{type = COMBAT_ENERGYDAMAGE, percent = 50},
	{type = COMBAT_EARTHDAMAGE, percent = 40},
	{type = COMBAT_ICEDAMAGE, percent = -10},
	{type = COMBAT_HOLYDAMAGE, percent = -10}
}

--todo: check this in combat?
pokemon.immunities = {
	{type = "fire", combat = true, condition = true},
	{type = "drown", condition = true},
	{type = "lifedrain", combat = true},
	{type = "paralyze", condition = true},
	{type = "invisible", condition = true}
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

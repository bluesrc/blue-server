local mType = Game.createPokemonType("Bulbasaur")
local pokemon = {}
pokemon.description = "a bulbasaur"

pokemon.number = 1
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER + pokemon.number }
pokemon.types = { TYPE_GRASS, TYPE_POISON }
pokemon.catch_rate = 45

pokemon.level_rate = RATE_MEDIUM_SLOW
pokemon.base_experience = 64
pokemon.ev_yield = { sp_attack = 1 }

pokemon.height = 0.7
pokemon.weight = 6.9

pokemon.gender_ratio = { male = 87.5, female = 12.5}
pokemon.egg_group = { EGG_GRASS, EGG_MONSTER}
pokemon.egg_cycles = 20

pokemon.base_friendship = 70

pokemon.base_stats = {
    hp= 45,
    attack= 49,
    defense= 49,
    sp_attack= 65,
    sp_defense= 65,
    speed = 45
}

pokemon.learnset = {
	{move = "tackle", level = 1},
	{move = "vine_whip", level = 7},
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

pokemon.attacks = {
	{name = "melee", attack = 1, skill = 70, effect = CONST_ME_DRAWBLOOD, interval = 2*1000},
	{name = "terra strike", range = 1, chance = 10, interval = 2*1000, minDamage = -210, maxDamage = -300, target = true},
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

local mType = Game.createPokemonType("Sandshrew")
local pokemon = {}
pokemon.description = "a sandshrew"

pokemon.number = 27
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER + pokemon.number }
pokemon.types = { TYPE_GROUND }
pokemon.catch_rate = 255

pokemon.level_rate = RATE_MEDIUM_FAST
pokemon.base_experience = 60
pokemon.ev_yield = { defense = 1 }

pokemon.height = 0.6
pokemon.weight = 12.0

pokemon.gender_ratio = { male = 50, female = 50 }
pokemon.egg_group = { EGG_FIELD }
pokemon.egg_cycles = 20

pokemon.base_friendship = 50

pokemon.base_stats = {
	hp = 50,
	attack = 75,
	defense = 85,
	sp_attack = 20,
	sp_defense = 30,
	speed = 40
}

pokemon.learnset = {
	{move = "scratch", level = 1},
	{move = "sand_attack", level = 5},
	{move = "bulldoze", level = 8},
	{move = "defense_curl", level = 12},
}

pokemon.tms = {
	"rock_tomb",
	"brick_break",
	"dragon_claw",
}

pokemon.abilities = {
	{ability = "sand_veil", slot = 1, chance = 50},
	{ability = "arena_trap", slot = 2, chance = 50},
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

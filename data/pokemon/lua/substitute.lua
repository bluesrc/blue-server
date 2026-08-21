local mType = Game.createPokemonType("Substitute")
local pokemon = {}

local SUBSTITUTE_MAX_HEALTH = 1000000

pokemon.description = "a substitute"
pokemon.number = 0
pokemon.outfit = { lookType = POKEMON_OUTFIT_NUMBER + pokemon.number }

pokemon.base_experience = 0
pokemon.base_stats = {
	hp = 255,
	attack = 1,
	defense = 1,
	sp_attack = 1,
	sp_defense = 1,
	speed = 1
}

pokemon.flags = {
	summonable = false,
	attackable = true,
	hostile = false,
	challengeable = false,
	convinceable = false,
	ignoreSpawnBlock = true,
	illusionable = false,
	pushable = false,
	canPushItems = false,
	canPushCreatures = false,
	targetDistance = 1,
	staticAttackChance = 100
}

mType.onThink = function(substitute, interval)
	substitute:setMovementBlocked(true)
	substitute:setTarget(nil)
	substitute:setFollowCreature(nil)
	if substitute:getMaxHealth() ~= SUBSTITUTE_MAX_HEALTH then
		substitute:setMaxHealth(SUBSTITUTE_MAX_HEALTH)
	end
	if substitute:getHealth() < SUBSTITUTE_MAX_HEALTH then
		substitute:setHealth(SUBSTITUTE_MAX_HEALTH)
	end
	return true
end

mType.onAppear = function(substitute, creature)
	substitute:setMovementBlocked(true)
	substitute:setTarget(nil)
	substitute:setFollowCreature(nil)
	substitute:setMaxHealth(SUBSTITUTE_MAX_HEALTH)
	substitute:setHealth(SUBSTITUTE_MAX_HEALTH)
	return true
end

mType.onDisappear = function(substitute, creature)
	return true
end

mType.onMove = function(substitute, creature, fromPosition, toPosition)
	return true
end

mType.onSay = function(substitute, creature, type, message)
	return true
end

mType:register(pokemon)

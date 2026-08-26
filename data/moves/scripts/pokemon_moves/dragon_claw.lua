local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_POKEMON_DRAGONDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_DRAGONHEAD)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, false)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end

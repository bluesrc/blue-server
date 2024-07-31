local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_MAGIC_BLUE)
combat:setParameter(COMBAT_PARAM_DISPEL, CONDITION_ENERGY)
combat:setParameter(COMBAT_PARAM_AGGRESSIVE, false)

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end
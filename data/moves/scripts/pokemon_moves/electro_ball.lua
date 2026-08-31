local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_POKEMON_ELECTRICDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_YELLOWENERGY)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_ENERGYBALL)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, false)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end

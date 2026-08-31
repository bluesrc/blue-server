local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_POKEMON_FIREDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_FIREATTACK)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, false)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)

function onTargetCreature(creature, target)
	creature:modifyBattleStatStage("speed", 1, 10000)
	return true
end

combat:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end

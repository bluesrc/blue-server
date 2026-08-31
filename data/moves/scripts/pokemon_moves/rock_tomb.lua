local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_POKEMON_ROCKDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_BLOCKHIT)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_LARGEROCK)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, false)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)

function onTargetCreature(creature, target)
	local pokemon = Pokemon(target)
	if pokemon then
		pokemon:modifyBattleStatStage("speed", -1, 10000)
	end
	return true
end

combat:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end

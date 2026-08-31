local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_POKEMON_GRASSDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_PLANTATTACK)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_SMALLEARTH)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, false)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)

function onTargetCreature(creature, target)
	local pokemon = Pokemon(target)
	if pokemon and math.random(100) <= 10 then
		pokemon:modifyBattleStatStage("special_defense", -1, 10000)
	end
	return true
end

combat:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end

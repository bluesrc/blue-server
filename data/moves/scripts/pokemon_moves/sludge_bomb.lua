local combat = Combat()
combat:setParameter(COMBAT_PARAM_TYPE, COMBAT_POKEMON_POISONDAMAGE)
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_POISONAREA)
combat:setParameter(COMBAT_PARAM_DISTANCEEFFECT, CONST_ANI_POISON)
combat:setParameter(COMBAT_PARAM_BLOCKARMOR, false)
combat:setParameter(COMBAT_PARAM_BLOCKSHIELD, false)

function onTargetCreature(creature, target)
	local pokemon = Pokemon(target)
	if pokemon and math.random(100) <= 30 then
		pokemon:applyStatusCondition("poison", 15000, creature)
	end
	return true
end

combat:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end

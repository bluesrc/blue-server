local combat = Combat()
combat:setParameter(COMBAT_PARAM_EFFECT, CONST_ME_GREENSMOKE) -- Effect 169
combat:setArea(createCombatArea({
	{1, 1, 1},
	{1, 3, 1},
	{1, 1, 1}
}))

function onTargetCreature(creature, target)
	local pokemon = Pokemon(target)
	if pokemon then
		pokemon:applyStatusCondition("sleep", 7000, creature)
	end
	return true
end

combat:setCallback(CALLBACK_PARAM_TARGETCREATURE, "onTargetCreature")

function onCastMove(creature, variant)
	return combat:execute(creature, variant)
end

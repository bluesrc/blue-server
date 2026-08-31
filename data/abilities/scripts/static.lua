function afterDamage(owner, source, target, moveId, primaryDamage, primaryType,
		secondaryDamage, secondaryType, origin, ownerIsSource, priority, moveFlags)
	if ownerIsSource or not source or not moveFlags.contact or primaryDamage + secondaryDamage <= 0 then
		return
	end

	local attacker = Pokemon(source)
	if attacker and math.random(100) <= 30 then
		attacker:applyStatusCondition("paralysis", 15000, owner)
	end
end

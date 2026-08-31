function beforeDamage(owner, source, moveId, moveName, moveType, category,
		primaryDamage, primaryType, secondaryDamage, secondaryType, origin,
		critical, priority, moveFlags)
	if origin == ORIGIN_MOVE and primaryDamage + secondaryDamage > 0 and math.random(100) <= 10 then
		owner:getPosition():sendMagicEffect(CONST_ME_POFF)
		return false, false
	end
	return primaryDamage, secondaryDamage
end

local function absorbElectricMove(owner, moveType)
	if moveType ~= TYPE_ELECTRIC then
		return false
	end

	owner:modifyBattleStatStage("special_attack", 1, 12000)
	owner:getPosition():sendMagicEffect(CONST_ME_ENERGYHIT)
	return true
end

function beforeDamage(owner, source, moveId, moveName, moveType, category,
		primaryDamage, primaryType, secondaryDamage, secondaryType, origin,
		critical, priority, moveFlags)
	if absorbElectricMove(owner, moveType) then
		return false, false
	end
	return primaryDamage, secondaryDamage
end

function beforeStatus(owner, source, status, duration, moveId, moveName,
		moveType, category, priority, moveFlags)
	if moveType == TYPE_ELECTRIC then
		owner:getPosition():sendMagicEffect(CONST_ME_ENERGYHIT)
		return false, duration
	end
	return status, duration
end

function onCastMove(creature, variant)
	creature:getPosition():sendMagicEffect(CONST_ME_TELEPORT)
	return true
end

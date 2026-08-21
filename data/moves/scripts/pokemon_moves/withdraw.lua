function onCastMove(creature, variant)
	creature:modifyBattleStatStage("defense", 1)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_BLUE)
	return true
end

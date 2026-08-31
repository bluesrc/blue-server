function onCastMove(creature, variant)
	creature:modifyBattleStatStage("defense", 1, 12000)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN)
	return true
end

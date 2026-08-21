function onCastMove(creature, variant)
	creature:modifyBattleStatStage("attack", 1)
	creature:modifyBattleStatStage("special_attack", 1)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN)
	return true
end

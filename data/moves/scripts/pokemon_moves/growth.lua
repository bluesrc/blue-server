function onCastMove(creature, variant)
	creature:modifyBattleStatStage("attack", 1, 12000)
	creature:modifyBattleStatStage("special_attack", 1, 12000)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN) -- Effect 15: self buff
	return true
end

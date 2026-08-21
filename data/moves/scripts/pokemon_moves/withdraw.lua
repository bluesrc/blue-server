function onCastMove(creature, variant)
	creature:modifyBattleStatStage("defense", 1, 12000)
	creature:getPosition():sendMagicEffect(CONST_ME_MAGIC_GREEN) -- Effect 15: self buff
	return true
end

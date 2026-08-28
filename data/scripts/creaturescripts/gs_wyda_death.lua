local creatureevent = CreatureEvent("GiantSpiderWyda")

function creatureevent.onDeath(creature, corpse, killer, mostDamageKiller)
	creature:say("It seems this was just an illusion.", TALKTYPE_POKEMON_SAY)
	if mostDamageKiller:isPlayer() then
		mostDamageKiller:addAchievement("Someone's Bored")
	end
	return true
end

creatureevent:register()

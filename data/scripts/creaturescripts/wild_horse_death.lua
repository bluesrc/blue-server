local creatureevent = CreatureEvent("WildHorseDeath")

function creatureevent.onDeath(creature, corpse, killer, mostDamageKiller)
	creature:say("With its last strength the horse runs to safety.", TALKTYPE_POKEMON_SAY)
	return true
end

creatureevent:register()

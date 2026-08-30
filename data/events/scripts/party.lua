function Party:onJoin(player)
	if hasEventCallback(EVENT_CALLBACK_ONJOIN) then
		return EventCallback(EVENT_CALLBACK_ONJOIN, self, player)
	else
		return true
	end
end

function Party:onLeave(player)
	if hasEventCallback(EVENT_CALLBACK_ONLEAVE) then
		return EventCallback(EVENT_CALLBACK_ONLEAVE, self, player)
	else
		return true
	end
end

function Party:onDisband()
	if hasEventCallback(EVENT_CALLBACK_ONDISBAND) then
		return EventCallback(EVENT_CALLBACK_ONDISBAND, self)
	else
		return true
	end
end

function Party:onShareExperience(exp)
	local sharedExperienceMultiplier = 1.20 --20%
	local rawExp = exp

	exp = math.ceil((exp * sharedExperienceMultiplier) / (#self:getMembers() + 1))
	return hasEventCallback(EVENT_CALLBACK_ONSHAREEXPERIENCE) and EventCallback(EVENT_CALLBACK_ONSHAREEXPERIENCE, self, exp, rawExp) or exp
end

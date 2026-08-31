function onCalculateStats(owner, stats)
	if owner:getType():name():lower() ~= "pikachu" then
		return
	end

	stats.attack = math.min(255, stats.attack * 2)
	stats.sp_attack = math.min(255, stats.sp_attack * 2)
end

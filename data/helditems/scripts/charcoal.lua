function beforeMoveDamage(owner, target, moveId, moveName, moveType, category,
		damage, priority, moveFlags)
	if moveType == TYPE_FIRE and damage > 0 then
		return math.floor(damage * 1.2)
	end
	return damage
end

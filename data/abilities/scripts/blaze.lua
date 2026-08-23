function beforeMoveDamage(owner, target, moveId, moveName, moveType, category, damage, priority, moveFlags)
	if moveType == TYPE_FIRE and owner:getHealth() * 3 <= owner:getMaxHealth() then
		return math.floor(damage * 1.5)
	end
	return damage
end

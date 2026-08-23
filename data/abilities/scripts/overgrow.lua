function beforeMoveDamage(owner, target, moveId, moveName, moveType, category, damage)
	if moveType == TYPE_GRASS and owner:getHealth() * 3 <= owner:getMaxHealth() then
		return math.floor(damage * 1.5)
	end
	return damage
end

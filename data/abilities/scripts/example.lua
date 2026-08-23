-- Add script="my_ability.lua" to the ability entry in abilities.xml.
-- Every callback is optional, but the script must define at least one of them.

-- Called once when the owner enters combat. Combat ends after 10 seconds without
-- dealing or receiving damage, using a damaging move, or applying a status.
function onCombatEnter(owner, opponent)
end

-- Called after the regular move formula and before defenses/resistances.
-- Return a non-negative number to replace damage, false to cancel it, or nil to keep it.
function beforeMoveDamage(owner, target, moveId, moveName, moveType, category,
		damage, priority, moveFlags)
	return damage
end

-- Called on the defending Pokemon before mana shield and health loss.
-- Damage values have already passed through the regular defenses/resistances.
-- Return the two new damage values, false to block the impact, or nil values to
-- preserve the corresponding component. moveId is 0 for non-move damage.
-- Returning false also suppresses later effects from that impact; returning
-- 0, 0 only prevents health damage and keeps those effects.
function beforeDamage(owner, source, moveId, moveName, moveType, category,
		primaryDamage, primaryType, secondaryDamage, secondaryType, origin, critical,
		priority, moveFlags)
	return primaryDamage, secondaryDamage
end

-- Called on the Pokemon that would receive the status.
-- Return status and duration to replace them, or false to cancel the status.
function beforeStatus(owner, source, status, duration, moveId, moveName, moveType,
		category, priority, moveFlags)
	return status, duration
end

-- Called after health was actually removed. It runs for the source and target
-- Pokemon abilities; ownerIsSource tells which side owns the callback.
function afterDamage(owner, source, target, moveId, primaryDamage, primaryType,
		secondaryDamage, secondaryType, origin, ownerIsSource, priority, moveFlags)
end

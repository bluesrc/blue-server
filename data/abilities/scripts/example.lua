-- Add script="my_ability.lua" to the ability entry in abilities.xml.
-- Every callback is optional, but the script must define at least one of them.

-- Called once when the owner enters combat. Combat ends after 10 seconds without
-- dealing or receiving damage, using a damaging move, or applying a status.
function onCombatEnter(owner, opponent)
end

-- Called after the regular move formula and before defenses/resistances.
-- Return a non-negative number to replace damage, false to cancel it, or nil to keep it.
function beforeMoveDamage(owner, target, moveId, moveName, moveType, category, damage)
	return damage
end

-- Called on the Pokemon that would receive the status.
-- Return status and duration to replace them, or false to cancel the status.
function beforeStatus(owner, source, status, duration)
	return status, duration
end

-- Called after health was actually removed. It runs for the source and target
-- Pokemon abilities; ownerIsSource tells which side owns the callback.
function afterDamage(owner, source, target, moveId, primaryDamage, primaryType,
		secondaryDamage, secondaryType, origin, ownerIsSource)
end

-- Add script="my_held_item.lua" to an entry in helditems.xml.
-- Every callback is optional, but the script must define at least one.
-- Ability callbacks always run before the equivalent held-item callback.

-- Mutate hp, attack, defense, sp_attack, sp_defense and speed. Values are
-- clamped to 1..255. The remaining context fields are read-only.
function onCalculateStats(owner, stats)
end

function onCombatEnter(owner, opponent)
end

-- Temporary held-item state remains available here and is cleared afterwards.
function onCombatExit(owner)
end

-- Called in deterministic 1000 ms steps while the owner is in combat.
function onCombatPulse(owner, interval)
end

-- Return false to cancel use before cooldown starts.
function beforeMoveUse(owner, target, moveId, moveName, moveType, category,
        priority, moveFlags)
    return true
end

-- Return replacement damage, false to cancel, or nil to preserve it.
function beforeMoveDamage(owner, target, moveId, moveName, moveType, category,
        damage, priority, moveFlags)
    return damage
end

-- Return primary and secondary damage, or false to block the impact.
function beforeDamage(owner, source, moveId, moveName, moveType, category,
        primaryDamage, primaryType, secondaryDamage, secondaryType, origin,
        critical, priority, moveFlags)
    return primaryDamage, secondaryDamage
end

function afterDamage(owner, source, target, moveId, primaryDamage, primaryType,
        secondaryDamage, secondaryType, origin, ownerIsSource, priority, moveFlags)
end

-- Return status and duration, false to cancel, or nil values to preserve them.
function beforeStatus(owner, source, status, duration, moveId, moveName, moveType,
        category, priority, moveFlags)
    return status, duration
end

function afterStatus(owner, source, status)
	-- Example status-curing berry:
	-- if owner:cureStatusCondition() then owner:consumeHeldItem() end
end

-- Return replacement amount, false to cancel, or nil to preserve it.
function beforeHeal(owner, source, target, moveId, moveName, moveType, category,
        priority, moveFlags, amount, ownerIsSource)
    return amount
end

function afterHeal(owner, source, target, moveId, moveName, moveType, category,
        priority, moveFlags, amount, ownerIsSource)
end

function onKnockout(owner, target, moveId, moveName, moveType, category,
        priority, moveFlags)
end

function onFaint(owner, source, moveId, moveName, moveType, category,
        priority, moveFlags)
end

-- Return replacement experience, false for zero, or nil to preserve it.
function onExperienceGain(owner, experience)
    return experience
end

-- Mutate the six EV fields. Final storage is capped at 252 per stat and 510 total.
function onEVGain(owner, evs)
end

function onFriendshipChange(owner, oldValue, newValue, delta)
end

function onEvolution(owner, evolutionType, requirement)
end

-- Temporary state supports boolean, number and string values and is cleared on
-- item replacement or combat exit.
-- owner:setHeldItemState("key", value)
-- owner:getHeldItemState("key", defaultValue)
-- owner:clearHeldItemState("key")
-- owner:clearHeldItemState()

-- Consumable helpers:
-- local restored = owner:healFromHeldItem(1, 4) -- exact 1/4 max HP
-- if restored > 0 then owner:consumeHeldItem() end

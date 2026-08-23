-- Add script="my_ability.lua" to the ability entry in abilities.xml.
-- Every callback is optional, but the script must define at least one of them.

-- Called once when the owner enters combat. Combat ends after 10 seconds without
-- dealing or receiving damage, using a damaging move, or applying a status.
function onCombatEnter(owner, opponent)
end

-- Called once when the 10-second combat activity window expires. Temporary
-- ability state remains readable during this callback and is cleared afterwards.
function onCombatExit(owner)
end

-- Temporary per-Pokemon state available to every ability event:
-- owner:setAbilityState("activated", true)
-- owner:setAbilityState("stacks", owner:getAbilityState("stacks", 0) + 1)
-- owner:setAbilityState("cooldownUntil", os.mtime() + 5000)
-- owner:setAbilityState("lastTargetId", target and target:getId() or 0)
-- owner:clearAbilityState("stacks") -- nil as a value also removes one key
-- owner:clearAbilityState()         -- removes every key
-- Supported stored values are boolean, number and string. The server clears all
-- entries after onCombatExit; this state is intentionally never persisted.

-- Calculates passive base stats without creating battle-stage buffs. Mutate only
-- hp, attack, defense, sp_attack, sp_defense and speed. Values are clamped to
-- 1..255. Context fields currentHealth, maxHealth, healthPercent and status are
-- read-only. The server refreshes this automatically after HP, status and ability
-- state changes; weather/timed systems can call owner:refreshAbilityStats().
-- Keep this callback calculation-only: always derive changes from the received
-- base values and do not apply conditions or mutate ability state here.
function onCalculateStats(owner, stats)
	-- Example: 50% more speed below half HP.
	-- if stats.healthPercent <= 50 then
	-- 	stats.speed = math.floor(stats.speed * 1.5)
	-- end
end

-- Called whenever this Pokemon is placed in the world. position is its final tile.
function onSpawn(owner, position)
end

-- Called after onSpawn when the placed Pokemon has a master.
function onSummon(owner, master)
end

-- Called immediately before an active Pokemon is returned to its Pokeball.
-- fainted is true when the recall is part of its death flow.
function onRecall(owner, master, fainted)
end

-- Called only for regular walking, not teleportation.
function onStep(owner, fromPosition, toPosition)
end

-- Called first for the trainer's active Pokemon and then for the wild target.
-- Return a new 0..255 catch chance, false to force failure, or nil to keep it.
function onCaptureAttempt(owner, trainer, target, pokeballId, chance, ownerIsTarget)
	return chance
end

-- Called once when a wild Pokemon enters the active Pokemon's perception.
function onEncounter(owner, encountered)
end

-- Called for the defeating Pokemon and then the defeated Pokemon. Pickup-like
-- abilities can add items to corpse when ownerIsDefeated is false.
function onLoot(owner, defeated, corpse, ownerIsDefeated)
end

function onFriendshipChange(owner, oldValue, newValue, delta)
end

-- The current evolution data only defines the trigger type and requirement.
-- For level evolutions this fires once when the configured level is crossed.
function onEvolution(owner, evolutionType, requirement)
end

-- Called only on abilities belonging to opponents participating in the same
-- combat when a move flagged as "escape" attempts to leave it. Returning false
-- makes the move fail; physical teleportation on the map never triggers this.
function beforeEscape(owner, escapingPokemon, moveId, moveName, moveType,
		category, priority, moveFlags)
	-- Arena Trap-style example: return false
	return true
end

-- Called after normal move validation and before its effect is executed.
-- Return false to cancel without starting the move cooldown.
function beforeMoveUse(owner, target, moveId, moveName, moveType, category,
		priority, moveFlags)
	return true
end

-- Called after the move effect returns. success is the result of castMove.
function afterMoveUse(owner, target, moveId, moveName, moveType, category,
		priority, moveFlags, success)
end

-- Called once for each target missed, including targets of area moves.
function onMoveMiss(owner, target, moveId, moveName, moveType, category,
		priority, moveFlags)
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

-- Called on the attacker's ability after afterDamage when a different creature faints.
function onKnockout(owner, target, moveId, moveName, moveType, category,
		priority, moveFlags)
end

-- Called on the fainted Pokemon's ability after its final afterDamage callback.
function onFaint(owner, source, moveId, moveName, moveType, category,
		priority, moveFlags)
end

-- Called for both the healing source and receiving Pokemon. Return a new amount,
-- false to cancel, or nil to preserve it. Self-healing runs once as the source.
function beforeHeal(owner, source, target, moveId, moveName, moveType, category,
		priority, moveFlags, amount, ownerIsSource)
	return amount
end

-- amount is the real health restored after the target's maximum-health cap.
function afterHeal(owner, source, target, moveId, moveName, moveType, category,
		priority, moveFlags, amount, ownerIsSource)
end

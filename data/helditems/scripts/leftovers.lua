local HEAL_INTERVAL = 3000

function onCombatPulse(owner, interval)
    local elapsed = owner:getHeldItemState("healElapsed", 0) + interval
    if elapsed < HEAL_INTERVAL then
        owner:setHeldItemState("healElapsed", elapsed)
        return
    end

    owner:setHeldItemState("healElapsed", elapsed % HEAL_INTERVAL)
    owner:healFromHeldItem(1, 16)
end

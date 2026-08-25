function onCalculateStats(owner, stats)
    stats.speed = math.floor(stats.speed * 1.5)
end

function beforeMoveUse(owner, target, moveId, moveName, moveType, category,
        priority, moveFlags)
    local lockedMoveId = owner:getHeldItemLockedMoveId()
    return lockedMoveId == 0 or lockedMoveId == moveId
end

function afterMoveUse(owner, target, moveId, moveName, moveType, category,
        priority, moveFlags, success)
    if success and owner:isInCombat() and owner:getHeldItemLockedMoveId() == 0 then
        owner:setHeldItemLockedMoveId(moveId)
    end
end

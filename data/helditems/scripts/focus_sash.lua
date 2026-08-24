function beforeDamage(owner, source, moveId, moveName, moveType, category,
        primaryDamage, primaryType, secondaryDamage, secondaryType, origin,
        critical, priority, moveFlags)
    local totalDamage = primaryDamage + secondaryDamage
    local health = owner:getHealth()
    if origin ~= ORIGIN_MOVE or health <= 1 or health ~= owner:getMaxHealth() or totalDamage < health then
        return primaryDamage, secondaryDamage
    end

    local survivingDamage = health - 1
    local adjustedPrimary = math.min(primaryDamage, survivingDamage)
    local adjustedSecondary = math.min(secondaryDamage, survivingDamage - adjustedPrimary)
    owner:consumeHeldItem()
    return adjustedPrimary, adjustedSecondary
end

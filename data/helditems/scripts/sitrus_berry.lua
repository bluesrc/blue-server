function afterDamage(owner, source, target, moveId, primaryDamage, primaryType,
        secondaryDamage, secondaryType, origin, ownerIsSource, priority, moveFlags)
    if not target or owner ~= target or owner:getHealth() <= 0 or owner:getHealth() * 2 > owner:getMaxHealth() then
        return
    end

    local restoredHealth = owner:healFromHeldItem(1, 4)
    if restoredHealth > 0 then
        owner:consumeHeldItem()
    end
end

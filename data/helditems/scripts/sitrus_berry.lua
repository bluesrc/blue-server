function afterDamage(owner, source, target, moveId, primaryDamage, primaryType,
        secondaryDamage, secondaryType, origin, ownerIsSource, priority, moveFlags)
    if not target or owner ~= target or owner:getHealth() <= 0 or owner:getHealth() * 2 > owner:getMaxHealth() then
        return
    end

    if owner:consumeHeldItem() then
        owner:healFromHeldItem(1, 4)
    end
end

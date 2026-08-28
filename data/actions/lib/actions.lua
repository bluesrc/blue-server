-- Shared helpers for future item actions.

function destroyItem(player, target, toPosition)
	if type(target) ~= "userdata" or not target:isItem() then
		return false
	end

	if target:hasAttribute(ITEM_ATTRIBUTE_UNIQUEID) or target:hasAttribute(ITEM_ATTRIBUTE_ACTIONID) then
		return false
	end

	if toPosition.x == CONTAINER_POSITION then
		player:sendCancelMessage(RETURNVALUE_NOTPOSSIBLE)
		return true
	end

	local destroyId = ItemType(target.itemid):getDestroyId()
	if destroyId == 0 then
		return false
	end

	if math.random(7) == 1 then
		local remains = Game.createItem(destroyId, 1, toPosition)
		if remains then
			remains:decay()
		end

		if target:isContainer() then
			for index = target:getSize() - 1, 0, -1 do
				local containerItem = target:getItem(index)
				if containerItem then
					containerItem:moveTo(toPosition)
				end
			end
		end

		target:remove(1)
	end

	toPosition:sendMagicEffect(CONST_ME_POFF)
	return true
end

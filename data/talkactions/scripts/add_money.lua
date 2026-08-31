local MAX_MONEY_PER_COMMAND = 100000000

local function parsePositiveAmount(value)
	local amount = tonumber(value)
	if not amount or amount ~= math.floor(amount) or amount <= 0 or amount > MAX_MONEY_PER_COMMAND then
		return nil
	end
	return amount
end

function onSay(player, words, param)
	if not player:getGroup():getAccess() then
		return true
	end

	if player:getAccountType() < ACCOUNT_TYPE_GOD then
		return false
	end

	local split = param:splitTrimmed(",")
	local target = player
	local amountText = split[1]

	if #split == 2 then
		target = Player(split[1])
		amountText = split[2]
		if not target then
			player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "Target player is not online: " .. split[1])
			return true
		end
	elseif #split ~= 1 then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT,
			"Usage: /money <amount> or /money <player>, <amount>")
		return true
	end

	local amount = parsePositiveAmount(amountText)
	if not amount then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
			"Money must be a positive integer up to %.0f.", MAX_MONEY_PER_COMMAND))
		return true
	end

	local currentBalance = target:getBankBalance()
	if currentBalance > 9007199254740991 - amount then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, "The target player's balance is too high.")
		return true
	end
	target:setBankBalance(currentBalance + amount)
	local formattedAmount = string.format("%.0f", amount)
	if target == player then
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT,
			formattedAmount .. " gold was added to your character.")
	else
		player:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
			"%s gold was added to %s.", formattedAmount, target:getName()))
		target:sendTextMessage(MESSAGE_STATUS_DEFAULT, string.format(
			"%s added %s gold to your character.", player:getName(), formattedAmount))
	end
	return true
end

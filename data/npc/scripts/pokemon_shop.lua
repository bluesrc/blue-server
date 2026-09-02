local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
local shopModule = ShopModule:new()

NpcSystem.parseParameters(npcHandler)
npcHandler:addModule(shopModule)

function onCreatureAppear(cid)            npcHandler:onCreatureAppear(cid) end
function onCreatureDisappear(cid)         npcHandler:onCreatureDisappear(cid) end
function onCreatureSay(cid, type, msg)     npcHandler:onCreatureSay(cid, type, msg) end
function onThink()                        npcHandler:onThink() end

local stock = {
	-- Throwable Poke Balls.
	{names = {"poke ball", "pokeball"}, id = 26462, price = 1, name = "poke ball"},
	{names = {"great ball", "greatball"}, id = 26463, price = 1, name = "great ball"},
	{names = {"ultra ball", "ultraball"}, id = 26464, price = 1, name = "ultra ball"},

	-- Pokemon medicines.
	{names = {"potion"}, id = 7618, price = 1, name = "potion"},
	{names = {"super potion"}, id = 7588, price = 1, name = "super potion"},
	{names = {"hyper potion"}, id = 7591, price = 1, name = "hyper potion"},
	{names = {"max potion"}, id = 8473, price = 1, name = "max potion"},
	{names = {"revive"}, id = 26030, price = 1, name = "revive"},
	{names = {"max revive"}, id = 26031, price = 1, name = "max revive"},
	{names = {"full heal"}, id = 15465, price = 1, name = "full heal"},
	{names = {"full restore"}, id = 8472, price = 1, name = "full restore"},
	{names = {"antidote"}, id = 8474, price = 1, name = "antidote"},
	{names = {"awakening"}, id = 8704, price = 1, name = "awakening"},
	{names = {"burn heal"}, id = 9930, price = 1, name = "burn heal"},
	{names = {"ice heal"}, id = 12422, price = 1, name = "ice heal"},
	{names = {"paralyze heal", "paralysis heal"}, id = 23875, price = 1, name = "paralyze heal"},
	{names = {"rare candy", "candy"}, id = 6569, price = 1, name = "rare candy"},

	-- Technical Machines.
	{names = {"tm sludge bomb", "sludge bomb"}, id = 2263, price = 1, name = "TM Sludge Bomb"},
	{names = {"tm ice beam", "ice beam"}, id = 2264, price = 1, name = "TM Ice Beam"},
	{names = {"tm dragon claw", "dragon claw"}, id = 2267, price = 1, name = "TM Dragon Claw"},
	{names = {"tm thunderbolt", "thunderbolt"}, id = 2270, price = 1, name = "TM Thunderbolt"},
	{names = {"tm rock tomb", "rock tomb"}, id = 2272, price = 1, name = "TM Rock Tomb"},
	{names = {"tm energy ball", "energy ball"}, id = 2275, price = 1, name = "TM Energy Ball"},
	{names = {"tm brick break", "brick break"}, id = 2276, price = 1, name = "TM Brick Break"},

	-- Held items.
	{names = {"sitrus berry", "sitrus"}, id = 2674, price = 1, name = "sitrus berry"},
	{names = {"miracle seed"}, id = 7732, price = 1, name = "miracle seed"},
	{names = {"choice scarf"}, id = 2661, price = 1, name = "choice scarf"},
	{names = {"focus sash"}, id = 5909, price = 1, name = "focus sash"},
	{names = {"leftovers"}, id = 2666, price = 1, name = "leftovers"},
	{names = {"charcoal"}, id = 5911, price = 1, name = "charcoal"},
	{names = {"mystic water"}, id = 5912, price = 1, name = "mystic water"},
	{names = {"soft sand"}, id = 5913, price = 1, name = "soft sand"},
	{names = {"light ball"}, id = 5914, price = 1, name = "light ball"},
}

local rootOptions = "{Poke Balls}, {Medicines}, {Technical Machines}, {Held Items} or {trade}"

for _, item in ipairs(stock) do
	shopModule:addBuyableItem(item.names, item.id, item.price, 1, item.name)
end

local function creatureSayCallback(cid, type, msg)
	if not npcHandler:isFocused(cid) then
		return false
	end

	if msgcontains(msg, "pokeballs") or msgcontains(msg, "poke balls") then
		npcHandler:say("I sell Poke Balls, Great Balls and Ultra Balls. Choose " .. rootOptions .. ".", cid)
		return true
	elseif msgcontains(msg, "medicines") or msgcontains(msg, "medicine") or msgcontains(msg, "potions") then
		npcHandler:say("I sell Potions, Revives and medicine for every common status condition. Choose " .. rootOptions .. ".", cid)
		return true
	elseif msgcontains(msg, "tms") or msgcontains(msg, "technical machines") then
		npcHandler:say("I stock every Technical Machine currently available. Choose " .. rootOptions .. ".", cid)
		return true
	elseif msgcontains(msg, "held items") then
		npcHandler:say("I stock berries and held items for offensive, defensive and speed-focused builds. Choose " .. rootOptions .. ".", cid)
		return true
	end
	return false
end

npcHandler:setMessage(MESSAGE_GREET, "Welcome, |PLAYERNAME|! Ask about " .. rootOptions .. ".")
npcHandler:setMessage(MESSAGE_FAREWELL, "Good luck in your next battle, |PLAYERNAME|!")
npcHandler:setMessage(MESSAGE_ONCLOSESHOP, "Would you like anything else? Choose " .. rootOptions .. ".")
npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())

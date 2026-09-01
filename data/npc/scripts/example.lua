local keywordHandler = KeywordHandler:new()
local npcHandler = NpcHandler:new(keywordHandler)
local shopModule = ShopModule:new()

NpcSystem.parseParameters(npcHandler)
npcHandler:addModule(shopModule)

function onCreatureAppear(cid)            npcHandler:onCreatureAppear(cid) end
function onCreatureDisappear(cid)         npcHandler:onCreatureDisappear(cid) end
function onCreatureSay(cid, type, msg)     npcHandler:onCreatureSay(cid, type, msg) end
function onThink()                        npcHandler:onThink() end

local rootOptions = "{information}, {question} or {trade}"

shopModule:addBuyableItem({"poke ball", "pokeball"}, 26462, 1, 1, "poke ball")

local function creatureSayCallback(cid, type, msg)
	if not npcHandler:isFocused(cid) then
		return false
	end

	if msg:lower():trim() == "blue moon" then
		npcHandler:say("You discovered the secret dialogue. Continue with " .. rootOptions .. ".", cid)
		return true
	end

	return false
end

keywordHandler:addKeyword({"information"}, StdModule.say, {
	npcHandler = npcHandler,
	onlyFocus = true,
	text = "This is a deliberately longer message used to test wrapping and scrolling in the conversation panel. You can continue with " .. rootOptions .. ".",
	reset = true
})

local questionNode = keywordHandler:addKeyword({"question"}, StdModule.say, {
	npcHandler = npcHandler,
	onlyFocus = true,
	text = "Do the contextual conversation buttons work correctly? Choose {yes} or {no}."
})
questionNode:addChildKeyword({"yes"}, StdModule.say, {
	npcHandler = npcHandler,
	text = "Excellent. The positive branch was selected. Continue with " .. rootOptions .. ".",
	reset = true
})
questionNode:addChildKeyword({"no"}, StdModule.say, {
	npcHandler = npcHandler,
	text = "The negative branch was selected. Continue testing with " .. rootOptions .. ".",
	reset = true
})

npcHandler:setMessage(MESSAGE_GREET, "Hello, |PLAYERNAME|. I am the example NPC. Choose " .. rootOptions .. ".")
npcHandler:setMessage(MESSAGE_FAREWELL, "Good bye, |PLAYERNAME|. The test conversation is now closed.")
npcHandler:setMessage(MESSAGE_ONCLOSESHOP, "The shop was closed. Continue with " .. rootOptions .. ".")
npcHandler:setCallback(CALLBACK_MESSAGE_DEFAULT, creatureSayCallback)
npcHandler:addModule(FocusModule:new())

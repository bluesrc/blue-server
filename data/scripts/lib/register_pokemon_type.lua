registerPokemonType = {}
setmetatable(registerPokemonType,
{
	__call =
	function(self, mtype, mask)
		for _,parse in pairs(self) do
			parse(mtype, mask)
		end
	end
})

PokemonType.register = function(self, mask)
	return registerPokemonType(self, mask)
end

registerPokemonType.number = function(mtype, mask)
	if mask.number then
		mtype:number(mask.number)
	end
end

registerPokemonType.types = function(mtype, mask)
	if mask.types then
		mtype:types(mask.types)
	end
end

registerPokemonType.catch_rate = function(mtype, mask)
	if mask.catch_rate then
		mtype:catch_rate(mask.catch_rate)
	end
end

registerPokemonType.level_rate = function(mtype, mask)
	if mask.level_rate then
		mtype:level_rate(mask.level_rate)
	end
end

registerPokemonType.base_experience = function(mtype, mask)
	if mask.base_experience then
		mtype:experience(mask.base_experience)
	end
end

registerPokemonType.height = function(mtype, mask)
	if mask.height then
		mtype:height(mask.height)
	end
end

registerPokemonType.weight = function(mtype, mask)
	if mask.weight then
		mtype:weight(mask.weight)
	end
end

registerPokemonType.gender_ratio = function(mtype, mask)
	if mask.gender_ratio then
		mtype:gender(mask.gender_ratio)
	end
end

registerPokemonType.egg_group = function(mtype, mask)
	if mask.egg_group then
		mtype:egg_group(mask.egg_group)
	end
end

registerPokemonType.egg_cycles = function(mtype, mask)
	if mask.egg_cycles then
		mtype:egg_cycles(mask.egg_cycles)
	end
end

registerPokemonType.base_friendship = function(mtype, mask)
	if mask.base_friendship then
		mtype:friendship(mask.base_friendship)
	end
end

registerPokemonType.ev_yield = function(mtype, mask)
	if mask.ev_yield then
		mtype:ev_yield(mask.ev_yield)
	end
end

registerPokemonType.base_stats = function(mtype, mask)
	if mask.base_stats then
		mtype:base_stats(mask.base_stats)
	end
end

registerPokemonType.learnset = function(mtype, mask)
	if type(mask.learnset) == "table" then
		for _, entry in ipairs(mask.learnset) do
			if entry.move and entry.level then
				mtype:addLearnMove(entry.move, entry.level)
			end
		end
	end
end

registerPokemonType.evolution = function(mtype, mask)
	if mask.evolution then
		mtype:evolution(mask.evolution)
	end
end

registerPokemonType.name = function(mtype, mask)
	if mask.name then
		mtype:name(mask.name)
	end
end
registerPokemonType.description = function(mtype, mask)
	if mask.description then
		mtype:nameDescription(mask.description)
	end
end
registerPokemonType.experience = function(mtype, mask)
	if mask.experience then
		mtype:experience(mask.experience)
	end
end
registerPokemonType.skull = function(mtype, mask)
	if mask.skull then
		mtype:skull(mask.skull)
	end
end
registerPokemonType.outfit = function(mtype, mask)
	if mask.outfit then
		mtype:outfit(mask.outfit)
	end
end
registerPokemonType.maxHealth = function(mtype, mask)
	if mask.maxHealth then
		mtype:maxHealth(mask.maxHealth)
		mtype:health(math.min(mtype:health(), mask.maxHealth))
	end
end
registerPokemonType.health = function(mtype, mask)
	if mask.health then
		mtype:health(mask.health)
		mtype:maxHealth(math.max(mask.health, mtype:maxHealth()))
	end
end
registerPokemonType.runHealth = function(mtype, mask)
	if mask.runHealth then
		mtype:runHealth(mask.runHealth)
	end
end
registerPokemonType.maxSummons = function(mtype, mask)
	if mask.maxSummons then
		mtype:maxSummons(mask.maxSummons)
	end
end
registerPokemonType.race = function(mtype, mask)
	if mask.race then
		mtype:race(mask.race)
	end
end
registerPokemonType.manaCost = function(mtype, mask)
	if mask.manaCost then
		mtype:manaCost(mask.manaCost)
	end
end
registerPokemonType.speed = function(mtype, mask)
	if mask.speed then
		mtype:baseSpeed(mask.speed)
	end
end
registerPokemonType.corpse = function(mtype, mask)
	if mask.corpse then
		mtype:corpseId(mask.corpse)
	end
end
registerPokemonType.flags = function(mtype, mask)
	if mask.flags then
		if mask.flags.attackable ~= nil then
			mtype:isAttackable(mask.flags.attackable)
		end
		if mask.flags.healthHidden ~= nil then
			mtype:isHealthHidden(mask.flags.healthHidden)
		end
		if mask.flags.boss ~= nil then
			mtype:isBoss(mask.flags.boss)
		end
		if mask.flags.challengeable ~= nil then
			mtype:isChallengeable(mask.flags.challengeable)
		end
		if mask.flags.convinceable ~= nil then
			mtype:isConvinceable(mask.flags.convinceable)
		end
		if mask.flags.summonable ~= nil then
			mtype:isSummonable(mask.flags.summonable)
		end
		if mask.flags.ignoreSpawnBlock ~= nil then
			mtype:isIgnoringSpawnBlock(mask.flags.ignoreSpawnBlock)
		end
		if mask.flags.illusionable ~= nil then
			mtype:isIllusionable(mask.flags.illusionable)
		end
		if mask.flags.hostile ~= nil then
			mtype:isHostile(mask.flags.hostile)
		end
		if mask.flags.pushable ~= nil then
			mtype:isPushable(mask.flags.pushable)
		end
		if mask.flags.canPushItems ~= nil then
			mtype:canPushItems(mask.flags.canPushItems)
		end
		if mask.flags.canPushCreatures ~= nil then
			mtype:canPushCreatures(mask.flags.canPushCreatures)
		end
		-- if a pokemon can push creatures,
		-- it should not be pushable
		if mask.flags.canPushCreatures then
			mtype:isPushable(false)
		end
		if mask.flags.targetDistance then
			mtype:targetDistance(mask.flags.targetDistance)
		end
		if mask.flags.staticAttackChance then
			mtype:staticAttackChance(mask.flags.staticAttackChance)
		end
		if mask.flags.canWalkOnEnergy ~= nil then
			mtype:canWalkOnEnergy(mask.flags.canWalkOnEnergy)
		end
		if mask.flags.canWalkOnFire ~= nil then
			mtype:canWalkOnFire(mask.flags.canWalkOnFire)
		end
		if mask.flags.canWalkOnPoison ~= nil then
			mtype:canWalkOnPoison(mask.flags.canWalkOnPoison)
		end
	end
end
registerPokemonType.light = function(mtype, mask)
	if mask.light then
		mtype:light(mask.light.color or 0, mask.light.level or 0)
	end
end
registerPokemonType.changeTarget = function(mtype, mask)
	if mask.changeTarget then
		if mask.changeTarget.chance then
			mtype:changeTargetChance(mask.changeTarget.chance)
		end
		if mask.changeTarget.interval then
			mtype:changeTargetSpeed(mask.changeTarget.interval)
		end
	end
end
registerPokemonType.voices = function(mtype, mask)
	if type(mask.voices) == "table" then
		local interval, chance
		if mask.voices.interval then
			interval = mask.voices.interval
		end
		if mask.voices.chance then
			chance = mask.voices.chance
		end
		for k, v in pairs(mask.voices) do
			if type(v) == "table" then
				mtype:addVoice(v.text, interval, chance, v.yell)
			end
		end
	end
end
registerPokemonType.summons = function(mtype, mask)
	if type(mask.summons) == "table" then
		for k, v in pairs(mask.summons) do
			mtype:addSummon(v.name, v.interval, v.chance, v.max or -1)
		end
	end
end
registerPokemonType.events = function(mtype, mask)
	if type(mask.events) == "table" then
		for k, v in pairs(mask.events) do
			mtype:registerEvent(v)
		end
	end
end
registerPokemonType.loot = function(mtype, mask)
	if type(mask.loot) == "table" then
		local lootError = false
		for _, loot in pairs(mask.loot) do
			local parent = Loot()
			if not parent:setId(loot.id) then
				lootError = true
			end
			if loot.chance then
				parent:setChance(loot.chance)
			end
			if loot.maxCount then
				parent:setMaxCount(loot.maxCount)
			end
			if loot.aid or loot.actionId then
				parent:setActionId(loot.aid or loot.actionId)
			end
			if loot.subType or loot.charges then
				parent:setSubType(loot.subType or loot.charges)
			end
			if loot.text or loot.description then
				parent:setDescription(loot.text or loot.description)
			end
			if loot.child then
				for _, children in pairs(loot.child) do
					local child = Loot()
					if not child:setId(children.id) then
						lootError = true
					end
					if children.chance then
						child:setChance(children.chance)
					end
					if children.maxCount then
						child:setMaxCount(children.maxCount)
					end
					if children.aid or children.actionId then
						child:setActionId(children.aid or children.actionId)
					end
					if children.subType or children.charges then
						child:setSubType(children.subType or children.charges)
					end
					if children.text or children.description then
						child:setDescription(children.text or children.description)
					end
					parent:addChildLoot(child)
				end
			end
			mtype:addLoot(parent)
		end
		if lootError then
			print("[Warning - end] Pokemon: \"".. mtype:name() .. "\" loot could not correctly be load.")
		end
	end
end
registerPokemonType.elements = function(mtype, mask)
	if type(mask.elements) == "table" then
		for _, element in pairs(mask.elements) do
			if element.type and element.percent then
				mtype:addElement(element.type, element.percent)
			end
		end
	end
end
registerPokemonType.immunities = function(mtype, mask)
	if type(mask.immunities) == "table" then
		for _, immunity in pairs(mask.immunities) do
			if immunity.type and immunity.combat then
				mtype:combatImmunities(immunity.type)
			end
			if immunity.type and immunity.condition then
				mtype:conditionImmunities(immunity.type)
			end
		end
	end
end
registerPokemonType.attacks = function(mtype, mask)
	if type(mask.attacks) == "table" then
		for _, attack in pairs(mask.attacks) do
			local move = PokemonMove()
			if attack.name then
				if attack.name == "melee" then
					move:setType("melee")
					if attack.attack and attack.skill then
						move:setAttackValue(attack.attack, attack.skill)
					end
					if attack.minDamage and attack.maxDamage then
						move:setCombatValue(attack.minDamage, attack.maxDamage)
					end
					if attack.interval then
						move:setInterval(attack.interval)
					end
					if attack.effect then
						move:setCombatEffect(attack.effect)
					end
					if attack.condition then
						if attack.condition.type then
							move:setConditionType(attack.condition.type)
						end
						local startDamage = 0
						if attack.condition.startDamage then
							startDamage = attack.condition.startDamage
						end
						if attack.condition.minDamage and attack.condition.maxDamage then
							move:setConditionDamage(attack.condition.minDamage, attack.condition.maxDamage, startDamage)
						end
						if attack.condition.duration then
							move:setConditionDuration(attack.condition.duration)
						end
						if attack.condition.interval then
							move:setConditionTickInterval(attack.condition.interval)
						end
					end
				else
					move:setType(attack.name)
					if attack.type then
						if attack.name == "combat" then
							move:setCombatType(attack.type)
						else
							move:setConditionType(attack.type)
						end
					end
					if attack.interval then
						move:setInterval(attack.interval)
					end
					if attack.chance then
						move:setChance(attack.chance)
					end
					if attack.range then
						move:setRange(attack.range)
					end
					if attack.duration then
						move:setConditionDuration(attack.duration)
					end
					if attack.speed then
						if type(attack.speed) ~= "table" then
							move:setConditionSpeedChange(attack.speed)
						elseif type(attack.speed) == "table" then
							if attack.speed.min and attack.speed.max then
								move:setConditionSpeedChange(attack.speed.min, attack.speed.max)
							end
						end
					end
					if attack.target then
						move:setNeedTarget(attack.target)
					end
					if attack.length then
						move:setCombatLength(attack.length)
					end
					if attack.spread then
						move:setCombatSpread(attack.spread)
					end
					if attack.radius then
						move:setCombatRadius(attack.radius)
					end
					if attack.minDamage and attack.maxDamage then
						if attack.name == "combat" then
							move:setCombatValue(attack.minDamage, attack.maxDamage)
						else
							local startDamage = 0
							if attack.startDamage then
								startDamage = attack.startDamage
							end
							move:setConditionDamage(attack.minDamage, attack.maxDamage, startDamage)
						end
					end
					if attack.effect then
						move:setCombatEffect(attack.effect)
					end
					if attack.shootEffect then
						move:setCombatShootEffect(attack.shootEffect)
					end
					if attack.name == "drunk" then
						move:setConditionType(CONDITION_DRUNK)
						if attack.drunkenness then
							move:setConditionDrunkenness(attack.drunkenness)
						end
					end
				end
			elseif attack.script then
				move:setScriptName(attack.script)
				if attack.interval then
					move:setInterval(attack.interval)
				end
				if attack.chance then
					move:setChance(attack.chance)
				end
				if attack.minDamage and attack.maxDamage then
					move:setCombatValue(attack.minDamage, attack.maxDamage)
				end
				if attack.target then
					move:setNeedTarget(attack.target)
				end
				if attack.direction then
					move:setNeedDirection(attack.direction)
				end
			end
			mtype:addAttack(move)
		end
	end
end
registerPokemonType.defenses = function(mtype, mask)
	if type(mask.defenses) == "table" then
		if mask.defenses.defense then
			mtype:defense(mask.defenses.defense)
		end
		if mask.defenses.armor then
			mtype:armor(mask.defenses.armor)
		end
		for _, defense in pairs(mask.defenses) do
			if type(defense) == "table" then
				local move = PokemonMove()
				if defense.name then
					if defense.name == "melee" then
						move:setType("melee")
						if defense.attack and defense.skill then
							move:setAttackValue(defense.attack, defense.skill)
						end
						if defense.minDamage and defense.maxDamage then
							move:setCombatValue(defense.minDamage, defense.maxDamage)
						end
						if defense.interval then
							move:setInterval(defense.interval)
						end
						if defense.effect then
							move:setCombatEffect(defense.effect)
						end
						if defense.condition then
							if defense.condition.type then
								move:setConditionType(defense.condition.type)
							end
							local startDamage = 0
							if defense.condition.startDamage then
								startDamage = defense.condition.startDamage
							end
							if defense.condition.minDamage and defense.condition.maxDamage then
								move:setConditionDamage(defense.condition.minDamage, defense.condition.maxDamage, startDamage)
							end
							if defense.condition.duration then
								move:setConditionDuration(defense.condition.duration)
							end
							if defense.condition.interval then
								move:setConditionTickInterval(defense.condition.interval)
							end
						end
					else
						move:setType(defense.name)
						if defense.type then
							if defense.name == "combat" then
								move:setCombatType(defense.type)
							else
								move:setConditionType(defense.type)
							end
						end
						if defense.interval then
							move:setInterval(defense.interval)
						end
						if defense.chance then
							move:setChance(defense.chance)
						end
						if defense.range then
							move:setRange(defense.range)
						end
						if defense.duration then
							move:setConditionDuration(defense.duration)
						end
						if defense.speed then
							if type(defense.speed) ~= "table" then
								move:setConditionSpeedChange(defense.speed)
							elseif type(defense.speed) == "table" then
								if defense.speed.min and defense.speed.max then
									move:setConditionSpeedChange(defense.speed.min, defense.speed.max)
								end
							end
						end
						if defense.target then
							move:setNeedTarget(defense.target)
						end
						if defense.length then
							move:setCombatLength(defense.length)
						end
						if defense.spread then
							move:setCombatSpread(defense.spread)
						end
						if defense.radius then
							move:setCombatRadius(defense.radius)
						end
						if defense.minDamage and defense.maxDamage then
							if defense.name == "combat" then
								move:setCombatValue(defense.minDamage, defense.maxDamage)
							else
								local startDamage = 0
								if defense.startDamage then
									startDamage = defense.startDamage
								end
								move:setConditionDamage(defense.minDamage, defense.maxDamage, startDamage)
							end
						end
						if defense.effect then
							move:setCombatEffect(defense.effect)
						end
						if defense.shootEffect then
							move:setCombatShootEffect(defense.shootEffect)
						end
					end
				elseif defense.script then
					move:setScriptName(defense.script)
					if defense.interval then
						move:setInterval(defense.interval)
					end
					if defense.chance then
						move:setChance(defense.chance)
					end
					if defense.minDamage and defense.maxDamage then
						move:setCombatValue(defense.minDamage, defense.maxDamage)
					end
					if defense.target then
						move:setNeedTarget(defense.target)
					end
					if defense.direction then
						move:setNeedDirection(defense.direction)
					end
				end
				mtype:addDefense(move)
			end
		end
	end
end

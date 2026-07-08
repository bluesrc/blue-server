local logFormat = "[%s] %s %s"

function logCommand(player, words, param)
	local file = io.open("data/logs/" .. player:getName() .. " commands.log", "a")
	if not file then
		return
	end

	io.output(file)
	io.write(logFormat:format(os.date("%d/%m/%Y %H:%M"), words, param):trim() .. "\n")
	io.close(file)
end

local pokemonOptionUsage = "Options: level=1-100, shiny=true/false, happiness=0-255, ivs=0-31, evs=0-252, gender=male/female, iv_hp/iv_attack/iv_defense/iv_sp_attack/iv_sp_defense/iv_speed, ev_hp/ev_attack/ev_defense/ev_sp_attack/ev_sp_defense/ev_speed."

local pokemonStatAliases = {
	hp = "hp",
	attack = "attack",
	atk = "attack",
	defense = "defense",
	def = "defense",
	sp_attack = "sp_attack",
	spattack = "sp_attack",
	spatk = "sp_attack",
	sp_defense = "sp_defense",
	spdefense = "sp_defense",
	spdef = "sp_defense",
	speed = "speed",
	spd = "speed"
}

local function parsePokemonBoolean(value)
	value = value:trim():lower()
	if value == "true" or value == "yes" or value == "on" or value == "1" then
		return true
	end
	if value == "false" or value == "no" or value == "off" or value == "0" then
		return false
	end
	return nil
end

local function parsePokemonInteger(value, minValue, maxValue)
	local number = tonumber(value)
	if not number or number ~= math.floor(number) or number < minValue or number > maxValue then
		return nil
	end
	return number
end

local function setPokemonStatOption(options, statType, statName, value)
	if type(options[statType]) == "number" then
		local currentValue = options[statType]
		options[statType] = {
			hp = currentValue,
			attack = currentValue,
			defense = currentValue,
			sp_attack = currentValue,
			sp_defense = currentValue,
			speed = currentValue
		}
	end
	options[statType] = options[statType] or {}
	options[statType][statName] = value
end

function isPokemonCreateOptionToken(token)
	token = token and token:trim() or ""
	if token == "" then
		return false
	end

	return tonumber(token) ~= nil or token:lower() == "shiny" or token:match("^[%w_]+%s*=") ~= nil
end

function getPokemonCreateOptionUsage()
	return pokemonOptionUsage
end

function parsePokemonCreateOptions(split, startIndex)
	local options = {}
	local positionalLevelUsed = false

	for i = startIndex, #split do
		local token = split[i]:trim()
		if token ~= "" then
			local key, value = token:match("^([%w_]+)%s*=%s*(.+)$")
			if key then
				key = key:lower()
				value = value:trim()

				if key == "level" or key == "lvl" then
					local level = parsePokemonInteger(value, 1, 100)
					if not level then
						return nil, "Pokemon level must be an integer between 1 and 100."
					end
					options.level = level
				elseif key == "shiny" then
					local shiny = parsePokemonBoolean(value)
					if shiny == nil then
						return nil, "Pokemon shiny option must be true or false."
					end
					options.shiny = shiny
				elseif key == "happiness" or key == "hapiness" or key == "friendship" then
					local happiness = parsePokemonInteger(value, 0, 255)
					if not happiness then
						return nil, "Pokemon happiness must be an integer between 0 and 255."
					end
					options.friendship = happiness
				elseif key == "ivs" or key == "iv" then
					local iv = parsePokemonInteger(value, 0, 31)
					if not iv then
						return nil, "Pokemon IVs must be an integer between 0 and 31."
					end
					options.ivs = iv
				elseif key == "evs" or key == "ev" then
					local ev = parsePokemonInteger(value, 0, 252)
					if not ev then
						return nil, "Pokemon EVs must be an integer between 0 and 252."
					end
					options.evs = ev
				elseif key == "gender" then
					local gender = value:lower()
					if gender ~= "male" and gender ~= "m" and gender ~= "female" and gender ~= "f" and gender ~= "none" and gender ~= "undefined" and gender ~= "unknown" then
						return nil, "Pokemon gender must be male, female, none or undefined."
					end
					options.gender = gender
				else
					local statType, statName = key:match("^(iv)_([%w_]+)$")
					if not statType then
						statType, statName = key:match("^(ev)_([%w_]+)$")
					end

					statName = statName and pokemonStatAliases[statName]
					if not statName then
						return nil, "Unknown pokemon option: " .. key .. ". " .. pokemonOptionUsage
					end

					local maxValue = statType == "iv" and 31 or 252
					local statValue = parsePokemonInteger(value, 0, maxValue)
					if not statValue then
						return nil, "Pokemon " .. statType:upper() .. " " .. statName .. " must be an integer between 0 and " .. maxValue .. "."
					end
					setPokemonStatOption(options, statType .. "s", statName, statValue)
				end
			else
				local lowerToken = token:lower()
				if lowerToken == "shiny" then
					options.shiny = true
				elseif tonumber(token) and not positionalLevelUsed and options.level == nil then
					local level = parsePokemonInteger(token, 1, 100)
					if not level then
						return nil, "Pokemon level must be an integer between 1 and 100."
					end
					options.level = level
					positionalLevelUsed = true
				else
					return nil, "Invalid pokemon option: " .. token .. ". " .. pokemonOptionUsage
				end
			end
		end
	end

	return options
end

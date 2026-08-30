// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "pokemons.h"
#include "pokemon.h"
#include "moves.h"
#include "combat.h"
#include "configmanager.h"
#include "game.h"
#include "items.h"

#include "pugicast.h"

#include <unordered_set>

extern Game g_game;
extern Moves* g_moves;
extern Pokemons g_pokemons;
extern ConfigManager g_config;

namespace {
PokemonTypes_t getPokemonTypeByName(const std::string& value)
{
	const std::string type = asLowerCaseString(value);
	if (type == "bug") return TYPE_BUG;
	if (type == "dark") return TYPE_DARK;
	if (type == "dragon") return TYPE_DRAGON;
	if (type == "electric") return TYPE_ELECTRIC;
	if (type == "fairy") return TYPE_FAIRY;
	if (type == "fighting") return TYPE_FIGHTING;
	if (type == "fire") return TYPE_FIRE;
	if (type == "flying") return TYPE_FLYING;
	if (type == "ghost") return TYPE_GHOST;
	if (type == "grass") return TYPE_GRASS;
	if (type == "ground") return TYPE_GROUND;
	if (type == "ice") return TYPE_ICE;
	if (type == "normal") return TYPE_NORMAL;
	if (type == "poison") return TYPE_POISON;
	if (type == "psychic") return TYPE_PSYCHIC;
	if (type == "rock") return TYPE_ROCK;
	if (type == "steel") return TYPE_STEEL;
	if (type == "water") return TYPE_WATER;
	return TYPE_NONE;
}

PokemonMoveCategory_t getPokemonMoveCategoryByName(const std::string& value)
{
	const std::string category = asLowerCaseString(value);
	if (category == "physical") return POKEMON_MOVE_CATEGORY_PHYSICAL;
	if (category == "special") return POKEMON_MOVE_CATEGORY_SPECIAL;
	return POKEMON_MOVE_CATEGORY_STATUS;
}

PokemonMoveFlag_t getPokemonMoveFlagByName(const std::string& value)
{
	const std::string flag = asLowerCaseString(value);
	if (flag == "contact") return POKEMON_MOVE_FLAG_CONTACT;
	if (flag == "sound") return POKEMON_MOVE_FLAG_SOUND;
	if (flag == "punch") return POKEMON_MOVE_FLAG_PUNCH;
	if (flag == "bite") return POKEMON_MOVE_FLAG_BITE;
	if (flag == "projectile") return POKEMON_MOVE_FLAG_PROJECTILE;
	if (flag == "pulse") return POKEMON_MOVE_FLAG_PULSE;
	if (flag == "bomb") return POKEMON_MOVE_FLAG_BOMB;
	if (flag == "dance") return POKEMON_MOVE_FLAG_DANCE;
	if (flag == "powder") return POKEMON_MOVE_FLAG_POWDER;
	if (flag == "slicing") return POKEMON_MOVE_FLAG_SLICING;
	if (flag == "wind") return POKEMON_MOVE_FLAG_WIND;
	if (flag == "explosive") return POKEMON_MOVE_FLAG_EXPLOSIVE;
	if (flag == "healing") return POKEMON_MOVE_FLAG_HEALING;
	if (flag == "reflectable") return POKEMON_MOVE_FLAG_REFLECTABLE;
	if (flag == "escape") return POKEMON_MOVE_FLAG_ESCAPE;
	return POKEMON_MOVE_FLAG_NONE;
}

void pushAbilityMoveContext(lua_State* L, const PokemonMoveType* move)
{
	lua_pushinteger(L, move ? move->id : 0);
	LuaScriptInterface::pushString(L, move ? move->name : "");
	lua_pushinteger(L, move ? move->type : TYPE_NONE);
	lua_pushinteger(L, move ? move->category : -1);
	lua_pushinteger(L, move ? move->priority : 0);
	LuaScriptInterface::pushPokemonMoveFlags(L, move ? move->flags : POKEMON_MOVE_FLAG_NONE);
}
}

PokemonNatureModifiers_t getPokemonNatureModifiers(PokemonNatures_t nature)
{
	switch (nature) {
		case NATURE_LONELY: return {10, -10, 0, 0, 0};
		case NATURE_BRAVE: return {10, 0, 0, 0, -10};
		case NATURE_ADAMANT: return {10, 0, -10, 0, 0};
		case NATURE_NAUGHTY: return {10, 0, 0, -10, 0};
		case NATURE_BOLD: return {-10, 10, 0, 0, 0};
		case NATURE_RELAXED: return {0, 10, 0, 0, -10};
		case NATURE_IMPISH: return {0, 10, -10, 0, 0};
		case NATURE_LAX: return {0, 10, 0, -10, 0};
		case NATURE_TIMID: return {-10, 0, 0, 0, 10};
		case NATURE_HASTY: return {0, -10, 0, 0, 10};
		case NATURE_JOLLY: return {0, 0, -10, 0, 10};
		case NATURE_NAIVE: return {0, 0, 0, -10, 10};
		case NATURE_MODEST: return {-10, 0, 10, 0, 0};
		case NATURE_MILD: return {0, -10, 10, 0, 0};
		case NATURE_QUIET: return {0, 0, 10, 0, -10};
		case NATURE_RASH: return {0, 0, 10, -10, 0};
		case NATURE_CALM: return {-10, 0, 0, 10, 0};
		case NATURE_GENTLE: return {0, -10, 0, 10, 0};
		case NATURE_SASSY: return {0, 0, 0, 10, -10};
		case NATURE_CAREFUL: return {0, 0, -10, 10, 0};
		case NATURE_NONE:
		case NATURE_HARDY:
		case NATURE_DOCILE:
		case NATURE_SERIOUS:
		case NATURE_BASHFUL:
		case NATURE_QUIRKY:
		default:
			return {};
	}
}

PokemonStats_t calculatePokemonStats(const PokemonStats_t& baseStats, uint8_t level,
	const PokemonStats_t& ivs, const PokemonStats_t& evs, PokemonNatures_t nature)
{
	const PokemonNatureModifiers_t modifiers = getPokemonNatureModifiers(nature);
	auto calculateStat = [level](uint8_t baseStat, uint8_t iv, uint8_t ev, int8_t modifier) -> uint8_t {
		const uint16_t stat = (((2 * baseStat) + iv + (ev / 4)) * level) / 100 + 5;
		return static_cast<uint8_t>((stat * (100 + modifier)) / 100);
	};

	PokemonStats_t stats;
	stats.hp = static_cast<uint8_t>((((2 * baseStats.hp) + ivs.hp + (evs.hp / 4)) * level) / 100 + level + 10);
	stats.attack = calculateStat(baseStats.attack, ivs.attack, evs.attack, modifiers.attack);
	stats.defense = calculateStat(baseStats.defense, ivs.defense, evs.defense, modifiers.defense);
	stats.sp_attack = calculateStat(baseStats.sp_attack, ivs.sp_attack, evs.sp_attack, modifiers.sp_attack);
	stats.sp_defense = calculateStat(baseStats.sp_defense, ivs.sp_defense, evs.sp_defense, modifiers.sp_defense);
	stats.speed = calculateStat(baseStats.speed, ivs.speed, evs.speed, modifiers.speed);
	return stats;
}


void PokemonType::loadLoot(PokemonType* pokemonType, LootBlock lootBlock)
{
	if (lootBlock.childLoot.empty()) {
		bool isContainer = Item::items[lootBlock.id].isContainer();
		if (isContainer) {
			for (LootBlock child : lootBlock.childLoot) {
				lootBlock.childLoot.push_back(child);
			}
		}
		pokemonType->info.lootItems.push_back(lootBlock);
	} else {
		pokemonType->info.lootItems.push_back(lootBlock);
	}
}

bool Pokemons::loadFromXml(bool reloading /*= false*/)
{
	if (!loadAbilities()) {
		return false;
	}

	if (!loadHeldItems()) {
		return false;
	}

	if (!loadMoves()) {
		return false;
	}

	unloadedPokemons = {};
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/pokemon/pokemons.xml");
	if (!result) {
		printXMLError("Error - Pokemons::loadFromXml", "data/pokemon/pokemons.xml", result);
		return false;
	}

	loaded = true;

	for (auto pokemonNode : doc.child("pokemons").children()) {
		std::string name = asLowerCaseString(pokemonNode.attribute("name").as_string());
		std::string file = "data/pokemon/" + std::string(pokemonNode.attribute("file").as_string());
		unloadedPokemons.emplace(name, file);
	}

	bool forceLoad = g_config.getBoolean(ConfigManager::FORCE_POKEMONTYPE_LOAD);

	for (auto it : unloadedPokemons) {
		if ((forceLoad || reloading) && pokemons.find(it.first) != pokemons.end()) {
			loadPokemon(it.second, it.first, reloading);
		}
	}

	return true;
}

bool Pokemons::loadHeldItems()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/helditems/helditems.xml");
	if (!result) {
		printXMLError("Error - Pokemons::loadHeldItems", "data/helditems/helditems.xml", result);
		return false;
	}

	std::map<uint16_t, PokemonHeldItemType> loadedHeldItems;
	std::unordered_set<std::string> loadedKeys;
	auto loadedScriptInterface = std::make_unique<LuaScriptInterface>("Held Item Interface");
	if (!loadedScriptInterface->initState()) {
		std::cout << "[Error - Pokemons::loadHeldItems] Could not initialize the held item script interface." << std::endl;
		return false;
	}

	for (const pugi::xml_node& node : doc.child("heldItems").children("heldItem")) {
		PokemonHeldItemType heldItem;
		const uint32_t itemId = node.attribute("itemid").as_uint();
		heldItem.key = asLowerCaseString(node.attribute("key").as_string());
		heldItem.description = node.attribute("description").as_string();
		heldItem.script = node.attribute("script").as_string();
		heldItem.consumable = node.attribute("consumable").as_bool(false);

		if (itemId == 0 || itemId > std::numeric_limits<uint16_t>::max() || heldItem.key.empty() ||
				heldItem.description.empty()) {
			std::cout << "[Error - Pokemons::loadHeldItems] Invalid held item definition for item "
			          << itemId << '.' << std::endl;
			return false;
		}

		heldItem.itemId = static_cast<uint16_t>(itemId);
		const ItemType& itemType = Item::items[heldItem.itemId];
		if (itemType.id == 0 || itemType.clientId == 0 || itemType.isContainer() ||
				itemType.isFluidContainer()) {
			std::cout << "[Error - Pokemons::loadHeldItems] Invalid held item definition for item "
			          << itemId << '.' << std::endl;
			return false;
		}

		if (loadedHeldItems.find(heldItem.itemId) != loadedHeldItems.end() ||
				!loadedKeys.emplace(heldItem.key).second) {
			std::cout << "[Error - Pokemons::loadHeldItems] Duplicate held item id or key for item "
			          << itemId << '.' << std::endl;
			return false;
		}

		if (!heldItem.script.empty()) {
			const std::string scriptPath = "data/helditems/scripts/" + heldItem.script;
			if (loadedScriptInterface->loadFile(scriptPath) != 0) {
				std::cout << "[Error - Pokemons::loadHeldItems] Could not load " << scriptPath << ": "
				          << loadedScriptInterface->getLastLuaError() << std::endl;
				return false;
			}

			heldItem.calculateStatsEvent = loadedScriptInterface->getEvent("onCalculateStats");
			heldItem.combatEnterEvent = loadedScriptInterface->getEvent("onCombatEnter");
			heldItem.combatExitEvent = loadedScriptInterface->getEvent("onCombatExit");
			heldItem.combatPulseEvent = loadedScriptInterface->getEvent("onCombatPulse");
			heldItem.beforeMoveUseEvent = loadedScriptInterface->getEvent("beforeMoveUse");
			heldItem.afterMoveUseEvent = loadedScriptInterface->getEvent("afterMoveUse");
			heldItem.beforeMoveDamageEvent = loadedScriptInterface->getEvent("beforeMoveDamage");
			heldItem.beforeDamageEvent = loadedScriptInterface->getEvent("beforeDamage");
			heldItem.afterDamageEvent = loadedScriptInterface->getEvent("afterDamage");
			heldItem.beforeStatusEvent = loadedScriptInterface->getEvent("beforeStatus");
			heldItem.afterStatusEvent = loadedScriptInterface->getEvent("afterStatus");
			heldItem.beforeHealEvent = loadedScriptInterface->getEvent("beforeHeal");
			heldItem.afterHealEvent = loadedScriptInterface->getEvent("afterHeal");
			heldItem.knockoutEvent = loadedScriptInterface->getEvent("onKnockout");
			heldItem.faintEvent = loadedScriptInterface->getEvent("onFaint");
			heldItem.experienceGainEvent = loadedScriptInterface->getEvent("onExperienceGain");
			heldItem.evGainEvent = loadedScriptInterface->getEvent("onEVGain");
			heldItem.friendshipChangeEvent = loadedScriptInterface->getEvent("onFriendshipChange");
			heldItem.evolutionEvent = loadedScriptInterface->getEvent("onEvolution");
			if (heldItem.calculateStatsEvent == -1 && heldItem.combatEnterEvent == -1 &&
					heldItem.combatExitEvent == -1 && heldItem.combatPulseEvent == -1 &&
					heldItem.beforeMoveUseEvent == -1 && heldItem.afterMoveUseEvent == -1 &&
					heldItem.beforeMoveDamageEvent == -1 &&
					heldItem.beforeDamageEvent == -1 && heldItem.afterDamageEvent == -1 &&
					heldItem.beforeStatusEvent == -1 && heldItem.afterStatusEvent == -1 &&
					heldItem.beforeHealEvent == -1 && heldItem.afterHealEvent == -1 &&
					heldItem.knockoutEvent == -1 && heldItem.faintEvent == -1 &&
					heldItem.experienceGainEvent == -1 && heldItem.evGainEvent == -1 &&
					heldItem.friendshipChangeEvent == -1 && heldItem.evolutionEvent == -1) {
				std::cout << "[Error - Pokemons::loadHeldItems] Held item script " << scriptPath
				          << " does not define a supported event." << std::endl;
				return false;
			}
		}

		loadedHeldItems.emplace(heldItem.itemId, std::move(heldItem));
	}

	heldItems.swap(loadedHeldItems);
	heldItemScriptInterface = std::move(loadedScriptInterface);
	return true;
}

bool Pokemons::loadAbilities()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/abilities/abilities.xml");
	if (!result) {
		printXMLError("Error - Pokemons::loadAbilities", "data/abilities/abilities.xml", result);
		return false;
	}

	std::map<uint16_t, PokemonAbilityType> loadedAbilities;
	std::map<std::string, uint16_t> loadedNames;
	auto loadedScriptInterface = std::make_unique<LuaScriptInterface>("Ability Interface");
	if (!loadedScriptInterface->initState()) {
		std::cout << "[Error - Pokemons::loadAbilities] Could not initialize the ability script interface." << std::endl;
		return false;
	}

	for (const pugi::xml_node& node : doc.child("abilities").children("ability")) {
		PokemonAbilityType ability;
		const uint32_t abilityId = node.attribute("id").as_uint();
		ability.id = static_cast<uint16_t>(abilityId);
		ability.key = asLowerCaseString(node.attribute("key").as_string());
		ability.name = node.attribute("name").as_string();
		ability.description = node.attribute("description").as_string();
		ability.script = node.attribute("script").as_string();

		if (abilityId == 0 || abilityId > std::numeric_limits<uint16_t>::max() ||
				ability.key.empty() || ability.name.empty() || ability.description.empty()) {
			std::cout << "[Error - Pokemons::loadAbilities] Invalid Pokemon ability definition." << std::endl;
			return false;
		}

		if (loadedAbilities.find(ability.id) != loadedAbilities.end() ||
				!loadedNames.emplace(ability.key, ability.id).second) {
			std::cout << "[Error - Pokemons::loadAbilities] Duplicate Pokemon ability id or key for "
			          << ability.name << '.' << std::endl;
			return false;
		}

		const std::string displayName = asLowerCaseString(ability.name);
		if (displayName != ability.key && !loadedNames.emplace(displayName, ability.id).second) {
			std::cout << "[Error - Pokemons::loadAbilities] Duplicate Pokemon ability name for "
			          << ability.name << '.' << std::endl;
			return false;
		}

		if (!ability.script.empty()) {
			const std::string scriptPath = "data/abilities/scripts/" + ability.script;
			if (loadedScriptInterface->loadFile(scriptPath) != 0) {
				std::cout << "[Error - Pokemons::loadAbilities] Could not load " << scriptPath << ": "
				          << loadedScriptInterface->getLastLuaError() << std::endl;
				return false;
			}

			ability.calculateStatsEvent = loadedScriptInterface->getEvent("onCalculateStats");
			ability.spawnEvent = loadedScriptInterface->getEvent("onSpawn");
			ability.summonEvent = loadedScriptInterface->getEvent("onSummon");
			ability.recallEvent = loadedScriptInterface->getEvent("onRecall");
			ability.stepEvent = loadedScriptInterface->getEvent("onStep");
			ability.captureAttemptEvent = loadedScriptInterface->getEvent("onCaptureAttempt");
			ability.encounterEvent = loadedScriptInterface->getEvent("onEncounter");
			ability.lootEvent = loadedScriptInterface->getEvent("onLoot");
			ability.friendshipChangeEvent = loadedScriptInterface->getEvent("onFriendshipChange");
			ability.evolutionEvent = loadedScriptInterface->getEvent("onEvolution");
			ability.beforeEscapeEvent = loadedScriptInterface->getEvent("beforeEscape");
			ability.combatEnterEvent = loadedScriptInterface->getEvent("onCombatEnter");
			ability.combatExitEvent = loadedScriptInterface->getEvent("onCombatExit");
			ability.beforeMoveUseEvent = loadedScriptInterface->getEvent("beforeMoveUse");
			ability.afterMoveUseEvent = loadedScriptInterface->getEvent("afterMoveUse");
			ability.moveMissEvent = loadedScriptInterface->getEvent("onMoveMiss");
			ability.beforeMoveDamageEvent = loadedScriptInterface->getEvent("beforeMoveDamage");
			ability.beforeDamageEvent = loadedScriptInterface->getEvent("beforeDamage");
			ability.beforeStatusEvent = loadedScriptInterface->getEvent("beforeStatus");
			ability.afterDamageEvent = loadedScriptInterface->getEvent("afterDamage");
			ability.knockoutEvent = loadedScriptInterface->getEvent("onKnockout");
			ability.faintEvent = loadedScriptInterface->getEvent("onFaint");
			ability.beforeHealEvent = loadedScriptInterface->getEvent("beforeHeal");
			ability.afterHealEvent = loadedScriptInterface->getEvent("afterHeal");
			if (ability.calculateStatsEvent == -1 && ability.spawnEvent == -1 &&
					ability.summonEvent == -1 && ability.recallEvent == -1 &&
					ability.stepEvent == -1 && ability.captureAttemptEvent == -1 &&
					ability.encounterEvent == -1 && ability.lootEvent == -1 &&
					ability.friendshipChangeEvent == -1 && ability.evolutionEvent == -1 &&
					ability.beforeEscapeEvent == -1 && ability.combatEnterEvent == -1 &&
					ability.combatExitEvent == -1 &&
					ability.beforeMoveUseEvent == -1 && ability.afterMoveUseEvent == -1 &&
					ability.moveMissEvent == -1 && ability.beforeMoveDamageEvent == -1 &&
					ability.beforeDamageEvent == -1 && ability.beforeStatusEvent == -1 &&
					ability.afterDamageEvent == -1 && ability.knockoutEvent == -1 &&
					ability.faintEvent == -1 && ability.beforeHealEvent == -1 &&
					ability.afterHealEvent == -1) {
				std::cout << "[Error - Pokemons::loadAbilities] Ability script " << scriptPath
				          << " does not define a supported event." << std::endl;
				return false;
			}
		}

		loadedAbilities.emplace(ability.id, std::move(ability));
	}

	abilities.swap(loadedAbilities);
	abilityNames.swap(loadedNames);
	abilityScriptInterface = std::move(loadedScriptInterface);
	return true;
}

bool Pokemons::loadMoves()
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/moves/moves.xml");
	if (!result) {
		printXMLError("Error - Pokemons::loadMoves", "data/moves/moves.xml", result);
		return false;
	}

	std::map<uint16_t, PokemonMoveType> loadedMoves;
	std::map<std::string, uint16_t> loadedNames;
	for (const pugi::xml_node& node : doc.child("moves").children("move")) {
		if (!node.attribute("pokemonmoveid")) {
			continue;
		}

		PokemonMoveType move;
		move.id = node.attribute("pokemonmoveid").as_uint();
		move.key = asLowerCaseString(node.attribute("key").as_string());
		move.name = node.attribute("name").as_string();
		move.effect = move.name;
		move.type = getPokemonTypeByName(node.attribute("type").as_string());
		move.category = getPokemonMoveCategoryByName(node.attribute("category").as_string());
		move.power = node.attribute("power").as_uint();
		move.pp = node.attribute("pp").as_uint();
		move.accuracy = static_cast<uint8_t>(std::min<uint32_t>(100, node.attribute("accuracy").as_uint(100)));
		move.range = static_cast<uint8_t>(std::min<uint32_t>(Map::maxViewportX * 2, node.attribute("range").as_uint(1)));
		move.cooldown = std::max<uint32_t>(1, node.attribute("cooldown").as_uint(2000));
		const int32_t priority = node.attribute("priority").as_int(0);
		if (priority < -7 || priority > 5) {
			std::cout << "[Error - Pokemons::loadMoves] Invalid priority for " << move.name << '.' << std::endl;
			return false;
		}
		move.priority = static_cast<int8_t>(priority);
		for (std::string flagName : explodeString(node.attribute("flags").as_string(), ",")) {
			trimString(flagName);
			if (flagName.empty()) {
				continue;
			}
			const PokemonMoveFlag_t flag = getPokemonMoveFlagByName(flagName);
			if (flag == POKEMON_MOVE_FLAG_NONE) {
				std::cout << "[Error - Pokemons::loadMoves] Unknown flag " << flagName
				          << " for " << move.name << '.' << std::endl;
				return false;
			}
			if (move.hasFlag(flag)) {
				std::cout << "[Error - Pokemons::loadMoves] Duplicate flag " << flagName
				          << " for " << move.name << '.' << std::endl;
				return false;
			}
			move.flags |= flag;
		}
		const std::string targetMode = asLowerCaseString(node.attribute("targetmode").as_string());
		if (targetMode == "target" || node.attribute("needtarget").as_bool() || node.attribute("target").as_bool()) {
			move.target = POKEMON_MOVE_TARGET_TARGET;
		} else if (targetMode == "area" || node.attribute("direction").as_bool() ||
				node.attribute("radius") || node.attribute("length")) {
			move.target = POKEMON_MOVE_TARGET_AREA;
		} else if (targetMode == "self" || node.attribute("selftarget").as_bool()) {
			move.target = POKEMON_MOVE_TARGET_SELF;
		}

		if (move.id == 0 || move.key.empty() || move.name.empty() || move.type == TYPE_NONE) {
			std::cout << "[Error - Pokemons::loadMoves] Invalid Pokemon move definition." << std::endl;
			return false;
		}

		if (move.category != POKEMON_MOVE_CATEGORY_STATUS && (move.power == 0 || move.effect.empty())) {
			std::cout << "[Error - Pokemons::loadMoves] Damaging move " << move.name << " requires power and effect." << std::endl;
			return false;
		}

		if (!move.effect.empty() && !g_moves->getMoveByName(move.effect)) {
			std::cout << "[Error - Pokemons::loadMoves] Unknown effect move " << move.effect << " for " << move.name << '.' << std::endl;
			return false;
		}

		if (!loadedMoves.emplace(move.id, move).second || !loadedNames.emplace(move.key, move.id).second) {
			std::cout << "[Error - Pokemons::loadMoves] Duplicate Pokemon move id or key for " << move.name << '.' << std::endl;
			return false;
		}

		const std::string displayName = asLowerCaseString(move.name);
		if (displayName != move.key && !loadedNames.emplace(displayName, move.id).second) {
			std::cout << "[Error - Pokemons::loadMoves] Duplicate Pokemon move name for " << move.name << '.' << std::endl;
			return false;
		}
	}

	moves.swap(loadedMoves);
	moveNames.swap(loadedNames);
	return true;
}

const PokemonMoveType* Pokemons::getMoveById(uint16_t id) const
{
	const auto it = moves.find(id);
	return it != moves.end() ? &it->second : nullptr;
}

const PokemonMoveType* Pokemons::getMoveByName(const std::string& name) const
{
	const auto nameIt = moveNames.find(asLowerCaseString(name));
	return nameIt != moveNames.end() ? getMoveById(nameIt->second) : nullptr;
}

const PokemonAbilityType* Pokemons::getAbilityById(uint16_t id) const
{
	const auto it = abilities.find(id);
	return it != abilities.end() ? &it->second : nullptr;
}

const PokemonAbilityType* Pokemons::getAbilityByName(const std::string& name) const
{
	const auto nameIt = abilityNames.find(asLowerCaseString(name));
	return nameIt != abilityNames.end() ? getAbilityById(nameIt->second) : nullptr;
}

const PokemonHeldItemType* Pokemons::getHeldItemById(uint16_t itemId) const
{
	const auto it = heldItems.find(itemId);
	return it != heldItems.end() ? &it->second : nullptr;
}

bool Pokemons::addLearnMove(PokemonType* pokemonType, const std::string& moveName, uint8_t level)
{
	if (!pokemonType) {
		return false;
	}

	const PokemonMoveType* move = getMoveByName(moveName);
	if (!move) {
		std::cout << "[Warning - Pokemons::addLearnMove] Unknown move " << moveName << " for " << pokemonType->name << '.' << std::endl;
		return false;
	}

	auto& learnset = pokemonType->info.learnset;
	const auto duplicate = std::find_if(learnset.begin(), learnset.end(), [move](const PokemonLearnMove& entry) {
		return entry.moveId == move->id;
	});
	if (duplicate != learnset.end()) {
		std::cout << "[Warning - Pokemons::addLearnMove] Duplicate move " << move->name << " for " << pokemonType->name << '.' << std::endl;
		return false;
	}

	learnset.push_back({move->id, static_cast<uint8_t>(std::clamp<uint16_t>(level, 1, 100))});
	std::sort(learnset.begin(), learnset.end(), [](const PokemonLearnMove& lhs, const PokemonLearnMove& rhs) {
		return lhs.level != rhs.level ? lhs.level < rhs.level : lhs.moveId < rhs.moveId;
	});
	return true;
}

bool Pokemons::addTechnicalMachine(PokemonType* pokemonType, const std::string& moveName)
{
	if (!pokemonType) {
		return false;
	}

	const PokemonMoveType* move = getMoveByName(moveName);
	if (!move) {
		std::cout << "[Warning - Pokemons::addTechnicalMachine] Unknown move " << moveName << " for " << pokemonType->name << '.' << std::endl;
		return false;
	}

	auto& technicalMachines = pokemonType->info.technicalMachines;
	if (std::find(technicalMachines.begin(), technicalMachines.end(), move->id) != technicalMachines.end()) {
		std::cout << "[Warning - Pokemons::addTechnicalMachine] Duplicate move " << move->name << " for " << pokemonType->name << '.' << std::endl;
		return false;
	}

	technicalMachines.push_back(move->id);
	return true;
}

bool Pokemons::canLearnTechnicalMachine(const PokemonType& pokemonType, uint16_t moveId) const
{
	const auto& technicalMachines = pokemonType.info.technicalMachines;
	return std::find(technicalMachines.begin(), technicalMachines.end(), moveId) != technicalMachines.end();
}

bool Pokemons::addAbility(PokemonType* pokemonType, const std::string& abilityName, uint32_t chance, uint8_t slot)
{
	if (!pokemonType || chance > 100) {
		return false;
	}

	const PokemonAbilityType* ability = getAbilityByName(abilityName);
	if (!ability) {
		std::cout << "[Warning - Pokemons::addAbility] Unknown ability " << abilityName
		          << " for " << pokemonType->name << '.' << std::endl;
		return false;
	}

	auto& abilityOptions = pokemonType->info.abilities;
	if (slot == 0) {
		for (uint8_t candidate = 1; candidate <= 3; ++candidate) {
			if (std::none_of(abilityOptions.begin(), abilityOptions.end(), [candidate](const PokemonAbilityOption& option) {
				return option.slot == candidate;
			})) {
				slot = candidate;
				break;
			}
		}
	}
	if (slot > 3 || std::any_of(abilityOptions.begin(), abilityOptions.end(), [slot](const PokemonAbilityOption& option) {
		return option.slot == slot;
	})) {
		std::cout << "[Warning - Pokemons::addAbility] Invalid or duplicate ability slot for "
		          << pokemonType->name << '.' << std::endl;
		return false;
	}
	if (slot <= 2 && chance == 0) {
		std::cout << "[Warning - Pokemons::addAbility] A normal ability requires a positive chance for "
		          << pokemonType->name << '.' << std::endl;
		return false;
	}

	uint64_t totalChance = slot <= 2 ? chance : 0;
	for (const PokemonAbilityOption& option : abilityOptions) {
		if (option.slot <= 2) {
			totalChance += option.chance;
		}
	}
	if (totalChance > 100) {
		std::cout << "[Warning - Pokemons::addAbility] Total ability chance exceeds 100 for "
		          << pokemonType->name << '.' << std::endl;
		return false;
	}

	abilityOptions.push_back({ability->id, chance, slot});
	return true;
}

bool Pokemons::setHiddenAbilityChance(PokemonType* pokemonType, uint32_t chance)
{
	if (!pokemonType || chance > 100) {
		return false;
	}
	pokemonType->info.hiddenAbilityChance = static_cast<uint8_t>(chance);
	return true;
}

uint16_t Pokemons::selectAbility(const PokemonType& pokemonType) const
{
	return getAbilityBySlot(pokemonType, selectAbilitySlot(pokemonType));
}

uint8_t Pokemons::selectAbilitySlot(const PokemonType& pokemonType) const
{
	if (pokemonType.info.hiddenAbilityChance > 0 && hasAbilitySlot(pokemonType, 3) &&
			uniform_random(1, 100) <= pokemonType.info.hiddenAbilityChance) {
		return 3;
	}

	uint32_t totalChance = 0;
	for (const PokemonAbilityOption& option : pokemonType.info.abilities) {
		if (option.slot <= 2) {
			totalChance += option.chance;
		}
	}
	if (totalChance == 0) {
		return 0;
	}

	uint32_t roll = static_cast<uint32_t>(uniform_random(1, 100));
	for (const PokemonAbilityOption& option : pokemonType.info.abilities) {
		if (option.slot > 2) {
			continue;
		}
		if (roll <= option.chance) {
			return option.slot;
		}
		roll -= option.chance;
	}
	return 0;
}

uint16_t Pokemons::getAbilityBySlot(const PokemonType& pokemonType, uint8_t slot) const
{
	if (slot < 1 || slot > 3) {
		return 0;
	}
	const auto findSlot = [&pokemonType](uint8_t candidate) {
		return std::find_if(pokemonType.info.abilities.begin(), pokemonType.info.abilities.end(),
			[candidate](const PokemonAbilityOption& option) { return option.slot == candidate; });
	};
	const auto it = findSlot(slot);
	if (it != pokemonType.info.abilities.end()) {
		return it->abilityId;
	}

	// A species can expose the same visible Ability through multiple logical slots.
	// Keep the logical slot on the individual and only fall back for presentation/effects.
	for (uint8_t normalSlot = 1; normalSlot <= 2; ++normalSlot) {
		if (normalSlot == slot) {
			continue;
		}
		const auto normal = findSlot(normalSlot);
		if (normal != pokemonType.info.abilities.end()) {
			return normal->abilityId;
		}
	}
	return 0;
}

bool Pokemons::hasAbilitySlot(const PokemonType& pokemonType, uint8_t slot) const
{
	return std::any_of(pokemonType.info.abilities.begin(), pokemonType.info.abilities.end(),
		[slot](const PokemonAbilityOption& option) { return option.slot == slot; });
}

uint8_t Pokemons::getAbilitySlot(const PokemonType& pokemonType, uint16_t abilityId) const
{
	const auto it = std::find_if(pokemonType.info.abilities.begin(), pokemonType.info.abilities.end(),
		[abilityId](const PokemonAbilityOption& option) { return option.abilityId == abilityId; });
	return it != pokemonType.info.abilities.end() ? it->slot : 0;
}

bool Pokemons::isAbilityAvailable(const PokemonType& pokemonType, uint16_t abilityId) const
{
	return abilityId != 0 && std::any_of(pokemonType.info.abilities.begin(), pokemonType.info.abilities.end(),
		[abilityId](const PokemonAbilityOption& option) { return option.abilityId == abilityId; });
}

PokemonStats_t Pokemons::executeAbilityCalculateStats(Pokemon* owner, const PokemonStats_t& stats)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->calculateStatsEvent == -1) {
		return stats;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityCalculateStats] Call stack overflow" << std::endl;
		return stats;
	}

	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->calculateStatsEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->calculateStatsEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");

	lua_createtable(L, 0, 10);
	LuaScriptInterface::setField(L, "hp", stats.hp);
	LuaScriptInterface::setField(L, "attack", stats.attack);
	LuaScriptInterface::setField(L, "defense", stats.defense);
	LuaScriptInterface::setField(L, "sp_attack", stats.sp_attack);
	LuaScriptInterface::setField(L, "sp_defense", stats.sp_defense);
	LuaScriptInterface::setField(L, "speed", stats.speed);
	LuaScriptInterface::setField(L, "currentHealth", owner->getHealth());
	LuaScriptInterface::setField(L, "maxHealth", owner->getMaxHealth());
	LuaScriptInterface::setField(L, "healthPercent", owner->getMaxHealth() > 0 ?
		(100.0 * owner->getHealth()) / owner->getMaxHealth() : 0.0);
	LuaScriptInterface::setField(L, "status", owner->getPokemonStatusCondition());

	lua_pushvalue(L, -1);
	const int32_t statsReference = luaL_ref(L, LUA_REGISTRYINDEX);
	const bool success = abilityScriptInterface->protectedCall(L, 2, 0) == 0;
	if (!success) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	}

	PokemonStats_t result = stats;
	if (success) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, statsReference);
		const auto readStat = [L](const char* field, uint8_t fallback) {
			lua_getfield(L, -1, field);
			uint8_t value = fallback;
			if (lua_isnumber(L, -1)) {
				const lua_Number number = lua_tonumber(L, -1);
				if (std::isfinite(number)) {
					value = static_cast<uint8_t>(std::clamp<lua_Number>(std::floor(number), 1, 255));
				}
			}
			lua_pop(L, 1);
			return value;
		};
		result.hp = readStat("hp", result.hp);
		result.attack = readStat("attack", result.attack);
		result.defense = readStat("defense", result.defense);
		result.sp_attack = readStat("sp_attack", result.sp_attack);
		result.sp_defense = readStat("sp_defense", result.sp_defense);
		result.speed = readStat("speed", result.speed);
		lua_pop(L, 1);
	}
	luaL_unref(L, LUA_REGISTRYINDEX, statsReference);
	abilityScriptInterface->resetScriptEnv();
	return result;
}

void Pokemons::executeAbilitySpawn(Pokemon* owner)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->spawnEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilitySpawn] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->spawnEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->spawnEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	LuaScriptInterface::pushPosition(L, owner->getPosition());
	abilityScriptInterface->callVoidFunction(2);
}

void Pokemons::executeAbilitySummon(Pokemon* owner, Creature* master)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->summonEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilitySummon] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->summonEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->summonEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (master) {
		LuaScriptInterface::pushUserdata<Creature>(L, master);
		LuaScriptInterface::setCreatureMetatable(L, -1, master);
	} else {
		lua_pushnil(L);
	}
	abilityScriptInterface->callVoidFunction(2);
}

void Pokemons::executeAbilityRecall(Pokemon* owner, Creature* master, bool fainted)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->recallEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityRecall] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->recallEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->recallEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (master) {
		LuaScriptInterface::pushUserdata<Creature>(L, master);
		LuaScriptInterface::setCreatureMetatable(L, -1, master);
	} else {
		lua_pushnil(L);
	}
	LuaScriptInterface::pushBoolean(L, fainted);
	abilityScriptInterface->callVoidFunction(3);
}

void Pokemons::executeAbilityStep(Pokemon* owner, const Position& fromPosition, const Position& toPosition)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->stepEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityStep] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->stepEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->stepEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	LuaScriptInterface::pushPosition(L, fromPosition);
	LuaScriptInterface::pushPosition(L, toPosition);
	abilityScriptInterface->callVoidFunction(3);
}

double Pokemons::executeAbilityCaptureAttempt(Pokemon* owner, Player* trainer, Pokemon* target,
	uint16_t pokeballId, double chance, bool ownerIsTarget)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->captureAttemptEvent == -1) {
		return chance;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityCaptureAttempt] Call stack overflow" << std::endl;
		return chance;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->captureAttemptEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->captureAttemptEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (trainer) {
		LuaScriptInterface::pushUserdata<Player>(L, trainer);
		LuaScriptInterface::setMetatable(L, -1, "Player");
	} else {
		lua_pushnil(L);
	}
	if (target) {
		LuaScriptInterface::pushUserdata<Pokemon>(L, target);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, pokeballId);
	lua_pushnumber(L, chance);
	LuaScriptInterface::pushBoolean(L, ownerIsTarget);

	double result = chance;
	if (abilityScriptInterface->protectedCall(L, 6, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			result = 0;
		} else if (lua_isnumber(L, -1)) {
			const lua_Number returnedChance = lua_tonumber(L, -1);
			if (std::isfinite(returnedChance)) {
				result = std::clamp<double>(returnedChance, 0, 255);
			}
		}
		lua_pop(L, 1);
	}
	abilityScriptInterface->resetScriptEnv();
	return result;
}

void Pokemons::executeAbilityEncounter(Pokemon* owner, Pokemon* encountered)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->encounterEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityEncounter] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->encounterEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->encounterEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (encountered) {
		LuaScriptInterface::pushUserdata<Pokemon>(L, encountered);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	} else {
		lua_pushnil(L);
	}
	abilityScriptInterface->callVoidFunction(2);
}

void Pokemons::executeAbilityLoot(Pokemon* owner, Pokemon* defeated, Container* corpse, bool ownerIsDefeated)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->lootEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityLoot] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->lootEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->lootEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (defeated) {
		LuaScriptInterface::pushUserdata<Pokemon>(L, defeated);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	} else {
		lua_pushnil(L);
	}
	if (corpse) {
		LuaScriptInterface::pushUserdata<Container>(L, corpse);
		LuaScriptInterface::setMetatable(L, -1, "Container");
	} else {
		lua_pushnil(L);
	}
	LuaScriptInterface::pushBoolean(L, ownerIsDefeated);
	abilityScriptInterface->callVoidFunction(4);
}

void Pokemons::executeAbilityFriendshipChange(Pokemon* owner, uint8_t oldValue, uint8_t newValue, int32_t delta)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->friendshipChangeEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityFriendshipChange] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->friendshipChangeEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->friendshipChangeEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	lua_pushinteger(L, oldValue);
	lua_pushinteger(L, newValue);
	lua_pushinteger(L, delta);
	abilityScriptInterface->callVoidFunction(4);
}

void Pokemons::executeAbilityEvolution(Pokemon* owner, EvolveTypes_t type, uint32_t requirement)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->evolutionEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityEvolution] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->evolutionEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->evolutionEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	lua_pushinteger(L, type);
	lua_pushinteger(L, requirement);
	abilityScriptInterface->callVoidFunction(3);
}

bool Pokemons::executeAbilityBeforeEscape(Pokemon* owner, Pokemon* escapingPokemon,
	const PokemonMoveType& move)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->beforeEscapeEvent == -1) {
		return true;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityBeforeEscape] Call stack overflow" << std::endl;
		return true;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->beforeEscapeEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->beforeEscapeEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (escapingPokemon) {
		LuaScriptInterface::pushUserdata<Pokemon>(L, escapingPokemon);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, &move);

	bool accepted = true;
	if (abilityScriptInterface->protectedCall(L, 8, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			accepted = false;
		}
		lua_pop(L, 1);
	}
	abilityScriptInterface->resetScriptEnv();
	return accepted;
}

void Pokemons::executeAbilityCombatEnter(Pokemon* owner, Creature* opponent)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->combatEnterEvent == -1) {
		return;
	}

	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityCombatEnter] Call stack overflow" << std::endl;
		return;
	}

	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->combatEnterEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->combatEnterEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (opponent) {
		LuaScriptInterface::pushUserdata<Creature>(L, opponent);
		LuaScriptInterface::setCreatureMetatable(L, -1, opponent);
	} else {
		lua_pushnil(L);
	}
	abilityScriptInterface->callVoidFunction(2);
}

void Pokemons::executeAbilityCombatExit(Pokemon* owner)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->combatExitEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityCombatExit] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->combatExitEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->combatExitEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	abilityScriptInterface->callVoidFunction(1);
}

bool Pokemons::executeAbilityBeforeMoveUse(Pokemon* owner, Creature* target, const PokemonMoveType& move)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->beforeMoveUseEvent == -1) {
		return true;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityBeforeMoveUse] Call stack overflow" << std::endl;
		return true;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->beforeMoveUseEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->beforeMoveUseEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, &move);

	bool accepted = true;
	if (abilityScriptInterface->protectedCall(L, 8, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			accepted = false;
		}
		lua_pop(L, 1);
	}
	abilityScriptInterface->resetScriptEnv();
	return accepted;
}

void Pokemons::executeAbilityAfterMoveUse(Pokemon* owner, Creature* target,
	const PokemonMoveType& move, bool success)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->afterMoveUseEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityAfterMoveUse] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->afterMoveUseEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->afterMoveUseEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, &move);
	LuaScriptInterface::pushBoolean(L, success);
	abilityScriptInterface->callVoidFunction(9);
}

void Pokemons::executeAbilityMoveMiss(Pokemon* owner, Creature* target, const PokemonMoveType& move)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->moveMissEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityMoveMiss] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->moveMissEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->moveMissEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, &move);
	abilityScriptInterface->callVoidFunction(8);
}

int32_t Pokemons::executeAbilityBeforeMoveDamage(Pokemon* owner, Creature* target,
	const PokemonMoveType& move, int32_t damage)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->beforeMoveDamageEvent == -1) {
		return damage;
	}

	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityBeforeMoveDamage] Call stack overflow" << std::endl;
		return damage;
	}

	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->beforeMoveDamageEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->beforeMoveDamageEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, move.id);
	LuaScriptInterface::pushString(L, move.name);
	lua_pushinteger(L, move.type);
	lua_pushinteger(L, move.category);
	lua_pushinteger(L, damage);
	lua_pushinteger(L, move.priority);
	LuaScriptInterface::pushPokemonMoveFlags(L, move.flags);

	int32_t result = damage;
	if (abilityScriptInterface->protectedCall(L, 9, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			result = 0;
		} else if (lua_isnumber(L, -1)) {
			const lua_Number returnedDamage = lua_tonumber(L, -1);
			if (std::isfinite(returnedDamage)) {
				result = static_cast<int32_t>(std::clamp<lua_Number>(returnedDamage, 0, std::numeric_limits<int32_t>::max()));
			}
		}
		lua_pop(L, 1);
	}
	abilityScriptInterface->resetScriptEnv();
	return result;
}

bool Pokemons::executeAbilityBeforeDamage(Pokemon* owner, Creature* source,
	const PokemonMoveType* move, CombatDamage& damage)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->beforeDamageEvent == -1) {
		return true;
	}

	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityBeforeDamage] Call stack overflow" << std::endl;
		return true;
	}

	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->beforeDamageEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->beforeDamageEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, move ? move->id : 0);
	LuaScriptInterface::pushString(L, move ? move->name : "");
	lua_pushinteger(L, move ? move->type : TYPE_NONE);
	lua_pushinteger(L, move ? move->category : -1);
	lua_pushinteger(L, damage.primary.value);
	lua_pushinteger(L, damage.primary.type);
	lua_pushinteger(L, damage.secondary.value);
	lua_pushinteger(L, damage.secondary.type);
	lua_pushinteger(L, damage.origin);
	LuaScriptInterface::pushBoolean(L, damage.critical);
	lua_pushinteger(L, move ? move->priority : 0);
	LuaScriptInterface::pushPokemonMoveFlags(L, move ? move->flags : POKEMON_MOVE_FLAG_NONE);

	bool accepted = true;
	if (abilityScriptInterface->protectedCall(L, 14, 2) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -2) && !LuaScriptInterface::getBoolean(L, -2)) {
			damage.primary.value = 0;
			damage.secondary.value = 0;
			damage.defensiveAbilityBlocked = true;
			accepted = false;
		} else {
			const auto readDamage = [L](int32_t index, int32_t currentDamage) {
				if (!lua_isnumber(L, index)) {
					return currentDamage;
				}
				const lua_Number returnedDamage = lua_tonumber(L, index);
				if (!std::isfinite(returnedDamage)) {
					return currentDamage;
				}
				return static_cast<int32_t>(std::clamp<lua_Number>(
					returnedDamage, 0, std::numeric_limits<int32_t>::max()));
			};
			damage.primary.value = readDamage(-2, damage.primary.value);
			damage.secondary.value = readDamage(-1, damage.secondary.value);
		}
		lua_pop(L, 2);
	}
	abilityScriptInterface->resetScriptEnv();
	return accepted;
}

bool Pokemons::executeAbilityBeforeStatus(Pokemon* owner, Creature* source,
	PokemonStatusCondition_t& status, uint32_t& duration)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->beforeStatusEvent == -1) {
		return true;
	}

	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityBeforeStatus] Call stack overflow" << std::endl;
		return true;
	}

	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->beforeStatusEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->beforeStatusEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, status);
	lua_pushinteger(L, duration);
	Pokemon* sourcePokemon = source ? source->getPokemon() : nullptr;
	const PokemonMoveType* move = sourcePokemon && sourcePokemon->isExecutingPokemonMove() ?
		sourcePokemon->getExecutingMove() : nullptr;
	lua_pushinteger(L, move ? move->id : 0);
	LuaScriptInterface::pushString(L, move ? move->name : "");
	lua_pushinteger(L, move ? move->type : TYPE_NONE);
	lua_pushinteger(L, move ? move->category : -1);
	lua_pushinteger(L, move ? move->priority : 0);
	LuaScriptInterface::pushPokemonMoveFlags(L, move ? move->flags : POKEMON_MOVE_FLAG_NONE);

	bool accepted = true;
	if (abilityScriptInterface->protectedCall(L, 10, 2) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -2) && !LuaScriptInterface::getBoolean(L, -2)) {
			accepted = false;
		} else if (lua_isnumber(L, -2)) {
			const int32_t returnedStatus = LuaScriptInterface::getNumber<int32_t>(L, -2);
			if (returnedStatus >= POKEMON_STATUS_BURN && returnedStatus <= POKEMON_STATUS_CONFUSION) {
				status = static_cast<PokemonStatusCondition_t>(returnedStatus);
			}
		}
		if (lua_isnumber(L, -1)) {
			const lua_Number returnedDuration = lua_tonumber(L, -1);
			if (std::isfinite(returnedDuration)) {
				duration = static_cast<uint32_t>(std::clamp<lua_Number>(returnedDuration, 1, std::numeric_limits<uint32_t>::max()));
			}
		}
		lua_pop(L, 2);
	}
	abilityScriptInterface->resetScriptEnv();
	return accepted;
}

void Pokemons::executeAbilityAfterDamage(Pokemon* owner, Creature* source, Creature* target,
	const PokemonMoveType* move, const CombatDamage& damage, bool ownerIsSource)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->afterDamageEvent == -1) {
		return;
	}

	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityAfterDamage] Call stack overflow" << std::endl;
		return;
	}

	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->afterDamageEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->afterDamageEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, move ? move->id : 0);
	lua_pushinteger(L, damage.primary.value);
	lua_pushinteger(L, damage.primary.type);
	lua_pushinteger(L, damage.secondary.value);
	lua_pushinteger(L, damage.secondary.type);
	lua_pushinteger(L, damage.origin);
	LuaScriptInterface::pushBoolean(L, ownerIsSource);
	lua_pushinteger(L, move ? move->priority : 0);
	LuaScriptInterface::pushPokemonMoveFlags(L, move ? move->flags : POKEMON_MOVE_FLAG_NONE);
	abilityScriptInterface->callVoidFunction(12);
}

const PokemonHeldItemType* Pokemons::getHeldItemForEvent(const Pokemon* owner) const
{
	return owner && owner->isHeldItemEffectActive() ?
		getHeldItemById(owner->getEffectiveHeldItemId()) : nullptr;
}

bool Pokemons::prepareHeldItemEvent(Pokemon* owner, int32_t eventId, const char* eventName)
{
	if (!heldItemScriptInterface || !owner || eventId == -1 ||
			!processingHeldItemEvents.emplace(owner).second) {
		return false;
	}
	if (!heldItemScriptInterface->reserveScriptEnv()) {
		processingHeldItemEvents.erase(owner);
		std::cout << "[Error - Pokemons::" << eventName << "] Call stack overflow" << std::endl;
		return false;
	}

	ScriptEnvironment* env = heldItemScriptInterface->getScriptEnv();
	env->setScriptId(eventId, heldItemScriptInterface.get());
	lua_State* L = heldItemScriptInterface->getLuaState();
	heldItemScriptInterface->pushFunction(eventId);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	return true;
}

void Pokemons::finishHeldItemEvent(Pokemon* owner)
{
	processingHeldItemEvents.erase(owner);
}

void Pokemons::callHeldItemVoidFunction(Pokemon* owner, int32_t parameterCount)
{
	heldItemScriptInterface->callVoidFunction(parameterCount);
	finishHeldItemEvent(owner);
}

PokemonStats_t Pokemons::executeHeldItemCalculateStats(Pokemon* owner, const PokemonStats_t& stats)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || heldItem->calculateStatsEvent == -1 || !heldItemScriptInterface) {
		return stats;
	}

	const bool nestedHeldItemEvent = processingHeldItemEvents.find(owner) != processingHeldItemEvents.end();
	if (nestedHeldItemEvent) {
		if (!heldItemScriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Pokemons::executeHeldItemCalculateStats] Call stack overflow" << std::endl;
			return stats;
		}
		ScriptEnvironment* env = heldItemScriptInterface->getScriptEnv();
		env->setScriptId(heldItem->calculateStatsEvent, heldItemScriptInterface.get());
		lua_State* nestedState = heldItemScriptInterface->getLuaState();
		heldItemScriptInterface->pushFunction(heldItem->calculateStatsEvent);
		LuaScriptInterface::pushUserdata<Pokemon>(nestedState, owner);
		LuaScriptInterface::setMetatable(nestedState, -1, "Pokemon");
	} else if (!prepareHeldItemEvent(owner, heldItem->calculateStatsEvent, "executeHeldItemCalculateStats")) {
		return stats;
	}

	lua_State* L = heldItemScriptInterface->getLuaState();
	lua_createtable(L, 0, 10);
	LuaScriptInterface::setField(L, "hp", stats.hp);
	LuaScriptInterface::setField(L, "attack", stats.attack);
	LuaScriptInterface::setField(L, "defense", stats.defense);
	LuaScriptInterface::setField(L, "sp_attack", stats.sp_attack);
	LuaScriptInterface::setField(L, "sp_defense", stats.sp_defense);
	LuaScriptInterface::setField(L, "speed", stats.speed);
	LuaScriptInterface::setField(L, "currentHealth", owner->getHealth());
	LuaScriptInterface::setField(L, "maxHealth", owner->getMaxHealth());
	LuaScriptInterface::setField(L, "healthPercent", owner->getMaxHealth() > 0 ?
		(100.0 * owner->getHealth()) / owner->getMaxHealth() : 0.0);
	LuaScriptInterface::setField(L, "status", owner->getPokemonStatusCondition());

	lua_pushvalue(L, -1);
	const int32_t statsReference = luaL_ref(L, LUA_REGISTRYINDEX);
	const bool success = heldItemScriptInterface->protectedCall(L, 2, 0) == 0;
	if (!success) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	}

	PokemonStats_t result = stats;
	if (success) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, statsReference);
		const auto readStat = [L](const char* field, uint8_t fallback) {
			lua_getfield(L, -1, field);
			uint8_t value = fallback;
			if (lua_isnumber(L, -1)) {
				const lua_Number number = lua_tonumber(L, -1);
				if (std::isfinite(number)) {
					value = static_cast<uint8_t>(std::clamp<lua_Number>(std::floor(number), 1, 255));
				}
			}
			lua_pop(L, 1);
			return value;
		};
		result.hp = readStat("hp", result.hp);
		result.attack = readStat("attack", result.attack);
		result.defense = readStat("defense", result.defense);
		result.sp_attack = readStat("sp_attack", result.sp_attack);
		result.sp_defense = readStat("sp_defense", result.sp_defense);
		result.speed = readStat("speed", result.speed);
		lua_pop(L, 1);
	}
	luaL_unref(L, LUA_REGISTRYINDEX, statsReference);
	heldItemScriptInterface->resetScriptEnv();
	if (!nestedHeldItemEvent) {
		finishHeldItemEvent(owner);
	}
	return result;
}

void Pokemons::executeHeldItemCombatEnter(Pokemon* owner, Creature* opponent)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->combatEnterEvent, "executeHeldItemCombatEnter")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (opponent) {
		LuaScriptInterface::pushUserdata<Creature>(L, opponent);
		LuaScriptInterface::setCreatureMetatable(L, -1, opponent);
	} else {
		lua_pushnil(L);
	}
	callHeldItemVoidFunction(owner, 2);
}

void Pokemons::executeHeldItemCombatExit(Pokemon* owner)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (heldItem && prepareHeldItemEvent(owner, heldItem->combatExitEvent, "executeHeldItemCombatExit")) {
		callHeldItemVoidFunction(owner, 1);
	}
}

void Pokemons::executeHeldItemCombatPulse(Pokemon* owner, uint32_t interval)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->combatPulseEvent, "executeHeldItemCombatPulse")) {
		return;
	}
	lua_pushinteger(heldItemScriptInterface->getLuaState(), interval);
	callHeldItemVoidFunction(owner, 2);
}

bool Pokemons::executeHeldItemBeforeMoveUse(Pokemon* owner, Creature* target, const PokemonMoveType& move)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->beforeMoveUseEvent, "executeHeldItemBeforeMoveUse")) {
		return true;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, &move);

	bool accepted = true;
	if (heldItemScriptInterface->protectedCall(L, 8, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		accepted = !lua_isboolean(L, -1) || LuaScriptInterface::getBoolean(L, -1);
		lua_pop(L, 1);
	}
	heldItemScriptInterface->resetScriptEnv();
	finishHeldItemEvent(owner);
	return accepted;
}

void Pokemons::executeHeldItemAfterMoveUse(Pokemon* owner, Creature* target,
	const PokemonMoveType& move, bool success)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->afterMoveUseEvent, "executeHeldItemAfterMoveUse")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, &move);
	LuaScriptInterface::pushBoolean(L, success);
	callHeldItemVoidFunction(owner, 9);
}

int32_t Pokemons::executeHeldItemBeforeMoveDamage(Pokemon* owner, Creature* target,
	const PokemonMoveType& move, int32_t damage)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->beforeMoveDamageEvent, "executeHeldItemBeforeMoveDamage")) {
		return damage;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, move.id);
	LuaScriptInterface::pushString(L, move.name);
	lua_pushinteger(L, move.type);
	lua_pushinteger(L, move.category);
	lua_pushinteger(L, damage);
	lua_pushinteger(L, move.priority);
	LuaScriptInterface::pushPokemonMoveFlags(L, move.flags);

	int32_t result = damage;
	if (heldItemScriptInterface->protectedCall(L, 9, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			result = 0;
		} else if (lua_isnumber(L, -1)) {
			const lua_Number returnedDamage = lua_tonumber(L, -1);
			if (std::isfinite(returnedDamage)) {
				result = static_cast<int32_t>(std::clamp<lua_Number>(returnedDamage, 0, std::numeric_limits<int32_t>::max()));
			}
		}
		lua_pop(L, 1);
	}
	heldItemScriptInterface->resetScriptEnv();
	finishHeldItemEvent(owner);
	return result;
}

bool Pokemons::executeHeldItemBeforeDamage(Pokemon* owner, Creature* source,
	const PokemonMoveType* move, CombatDamage& damage)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->beforeDamageEvent, "executeHeldItemBeforeDamage")) {
		return true;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, move ? move->id : 0);
	LuaScriptInterface::pushString(L, move ? move->name : "");
	lua_pushinteger(L, move ? move->type : TYPE_NONE);
	lua_pushinteger(L, move ? move->category : -1);
	lua_pushinteger(L, damage.primary.value);
	lua_pushinteger(L, damage.primary.type);
	lua_pushinteger(L, damage.secondary.value);
	lua_pushinteger(L, damage.secondary.type);
	lua_pushinteger(L, damage.origin);
	LuaScriptInterface::pushBoolean(L, damage.critical);
	lua_pushinteger(L, move ? move->priority : 0);
	LuaScriptInterface::pushPokemonMoveFlags(L, move ? move->flags : POKEMON_MOVE_FLAG_NONE);

	bool accepted = true;
	if (heldItemScriptInterface->protectedCall(L, 14, 2) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -2) && !LuaScriptInterface::getBoolean(L, -2)) {
			damage.primary.value = 0;
			damage.secondary.value = 0;
			damage.defensiveAbilityBlocked = true;
			accepted = false;
		} else {
			const auto readDamage = [L](int32_t index, int32_t currentDamage) {
				if (!lua_isnumber(L, index)) {
					return currentDamage;
				}
				const lua_Number value = lua_tonumber(L, index);
				return std::isfinite(value) ? static_cast<int32_t>(std::clamp<lua_Number>(
					value, 0, std::numeric_limits<int32_t>::max())) : currentDamage;
			};
			damage.primary.value = readDamage(-2, damage.primary.value);
			damage.secondary.value = readDamage(-1, damage.secondary.value);
		}
		lua_pop(L, 2);
	}
	heldItemScriptInterface->resetScriptEnv();
	finishHeldItemEvent(owner);
	return accepted;
}

void Pokemons::executeHeldItemAfterDamage(Pokemon* owner, Creature* source, Creature* target,
	const PokemonMoveType* move, const CombatDamage& damage, bool ownerIsSource)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->afterDamageEvent, "executeHeldItemAfterDamage")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, move ? move->id : 0);
	lua_pushinteger(L, damage.primary.value);
	lua_pushinteger(L, damage.primary.type);
	lua_pushinteger(L, damage.secondary.value);
	lua_pushinteger(L, damage.secondary.type);
	lua_pushinteger(L, damage.origin);
	LuaScriptInterface::pushBoolean(L, ownerIsSource);
	lua_pushinteger(L, move ? move->priority : 0);
	LuaScriptInterface::pushPokemonMoveFlags(L, move ? move->flags : POKEMON_MOVE_FLAG_NONE);
	callHeldItemVoidFunction(owner, 12);
}

bool Pokemons::executeHeldItemBeforeStatus(Pokemon* owner, Creature* source,
	PokemonStatusCondition_t& status, uint32_t& duration)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->beforeStatusEvent, "executeHeldItemBeforeStatus")) {
		return true;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, status);
	lua_pushinteger(L, duration);
	Pokemon* sourcePokemon = source ? source->getPokemon() : nullptr;
	const PokemonMoveType* move = sourcePokemon && sourcePokemon->isExecutingPokemonMove() ?
		sourcePokemon->getExecutingMove() : nullptr;
	pushAbilityMoveContext(L, move);

	bool accepted = true;
	if (heldItemScriptInterface->protectedCall(L, 10, 2) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -2) && !LuaScriptInterface::getBoolean(L, -2)) {
			accepted = false;
		} else if (lua_isnumber(L, -2)) {
			const int32_t returnedStatus = LuaScriptInterface::getNumber<int32_t>(L, -2);
			if (returnedStatus >= POKEMON_STATUS_BURN && returnedStatus <= POKEMON_STATUS_CONFUSION) {
				status = static_cast<PokemonStatusCondition_t>(returnedStatus);
			}
		}
		if (lua_isnumber(L, -1)) {
			const lua_Number returnedDuration = lua_tonumber(L, -1);
			if (std::isfinite(returnedDuration)) {
				duration = static_cast<uint32_t>(std::clamp<lua_Number>(returnedDuration, 1, std::numeric_limits<uint32_t>::max()));
			}
		}
		lua_pop(L, 2);
	}
	heldItemScriptInterface->resetScriptEnv();
	finishHeldItemEvent(owner);
	return accepted;
}

void Pokemons::executeHeldItemAfterStatus(Pokemon* owner, Creature* source, PokemonStatusCondition_t status)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->afterStatusEvent, "executeHeldItemAfterStatus")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	lua_pushinteger(L, status);
	callHeldItemVoidFunction(owner, 3);
}

int32_t Pokemons::executeHeldItemBeforeHeal(Pokemon* owner, Creature* source, Creature* target,
	const PokemonMoveType* move, int32_t amount, bool ownerIsSource)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->beforeHealEvent, "executeHeldItemBeforeHeal")) {
		return amount;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	lua_pushinteger(L, amount);
	LuaScriptInterface::pushBoolean(L, ownerIsSource);

	int32_t result = amount;
	if (heldItemScriptInterface->protectedCall(L, 11, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			result = 0;
		} else if (lua_isnumber(L, -1)) {
			const lua_Number returnedAmount = lua_tonumber(L, -1);
			if (std::isfinite(returnedAmount)) {
				result = static_cast<int32_t>(std::clamp<lua_Number>(returnedAmount, 0, std::numeric_limits<int32_t>::max()));
			}
		}
		lua_pop(L, 1);
	}
	heldItemScriptInterface->resetScriptEnv();
	finishHeldItemEvent(owner);
	return result;
}

void Pokemons::executeHeldItemAfterHeal(Pokemon* owner, Creature* source, Creature* target,
	const PokemonMoveType* move, int32_t amount, bool ownerIsSource)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->afterHealEvent, "executeHeldItemAfterHeal")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	lua_pushinteger(L, amount);
	LuaScriptInterface::pushBoolean(L, ownerIsSource);
	callHeldItemVoidFunction(owner, 11);
}

void Pokemons::executeHeldItemKnockout(Pokemon* owner, Creature* target, const PokemonMoveType* move)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->knockoutEvent, "executeHeldItemKnockout")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	callHeldItemVoidFunction(owner, 8);
}

void Pokemons::executeHeldItemFaint(Pokemon* owner, Creature* source, const PokemonMoveType* move)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->faintEvent, "executeHeldItemFaint")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	callHeldItemVoidFunction(owner, 8);
}

uint64_t Pokemons::executeHeldItemExperienceGain(Pokemon* owner, uint64_t experience)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->experienceGainEvent, "executeHeldItemExperienceGain")) {
		return experience;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	lua_pushnumber(L, static_cast<lua_Number>(experience));

	uint64_t result = experience;
	if (heldItemScriptInterface->protectedCall(L, 2, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			result = 0;
		} else if (lua_isnumber(L, -1)) {
			const lua_Number returnedExperience = lua_tonumber(L, -1);
			if (std::isfinite(returnedExperience)) {
				if (returnedExperience <= 0) {
					result = 0;
				} else if (returnedExperience >= static_cast<lua_Number>(std::numeric_limits<uint64_t>::max())) {
					result = std::numeric_limits<uint64_t>::max();
				} else {
					result = static_cast<uint64_t>(std::floor(returnedExperience));
				}
			}
		}
		lua_pop(L, 1);
	}
	heldItemScriptInterface->resetScriptEnv();
	finishHeldItemEvent(owner);
	return result;
}

PokemonStats_t Pokemons::executeHeldItemEVGain(Pokemon* owner, const PokemonStats_t& evs)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->evGainEvent, "executeHeldItemEVGain")) {
		return evs;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	lua_createtable(L, 0, 6);
	LuaScriptInterface::setField(L, "hp", evs.hp);
	LuaScriptInterface::setField(L, "attack", evs.attack);
	LuaScriptInterface::setField(L, "defense", evs.defense);
	LuaScriptInterface::setField(L, "sp_attack", evs.sp_attack);
	LuaScriptInterface::setField(L, "sp_defense", evs.sp_defense);
	LuaScriptInterface::setField(L, "speed", evs.speed);
	lua_pushvalue(L, -1);
	const int32_t evReference = luaL_ref(L, LUA_REGISTRYINDEX);
	const bool success = heldItemScriptInterface->protectedCall(L, 2, 0) == 0;
	if (!success) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	}

	PokemonStats_t result = evs;
	if (success) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, evReference);
		const auto readEV = [L](const char* field, uint8_t fallback) {
			lua_getfield(L, -1, field);
			uint8_t value = fallback;
			if (lua_isnumber(L, -1)) {
				const lua_Number number = lua_tonumber(L, -1);
				if (std::isfinite(number)) {
					value = static_cast<uint8_t>(std::clamp<lua_Number>(std::floor(number), 0, 255));
				}
			}
			lua_pop(L, 1);
			return value;
		};
		result.hp = readEV("hp", result.hp);
		result.attack = readEV("attack", result.attack);
		result.defense = readEV("defense", result.defense);
		result.sp_attack = readEV("sp_attack", result.sp_attack);
		result.sp_defense = readEV("sp_defense", result.sp_defense);
		result.speed = readEV("speed", result.speed);
		lua_pop(L, 1);
	}
	luaL_unref(L, LUA_REGISTRYINDEX, evReference);
	heldItemScriptInterface->resetScriptEnv();
	finishHeldItemEvent(owner);
	return result;
}

void Pokemons::executeHeldItemFriendshipChange(Pokemon* owner, uint8_t oldValue, uint8_t newValue, int32_t delta)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->friendshipChangeEvent, "executeHeldItemFriendshipChange")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	lua_pushinteger(L, oldValue);
	lua_pushinteger(L, newValue);
	lua_pushinteger(L, delta);
	callHeldItemVoidFunction(owner, 4);
}

void Pokemons::executeHeldItemEvolution(Pokemon* owner, EvolveTypes_t type, uint32_t requirement)
{
	const PokemonHeldItemType* heldItem = getHeldItemForEvent(owner);
	if (!heldItem || !prepareHeldItemEvent(owner, heldItem->evolutionEvent, "executeHeldItemEvolution")) {
		return;
	}
	lua_State* L = heldItemScriptInterface->getLuaState();
	lua_pushinteger(L, type);
	lua_pushinteger(L, requirement);
	callHeldItemVoidFunction(owner, 3);
}

void Pokemons::executeAbilityKnockout(Pokemon* owner, Creature* target, const PokemonMoveType* move)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->knockoutEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityKnockout] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->knockoutEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->knockoutEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	abilityScriptInterface->callVoidFunction(8);
}

void Pokemons::executeAbilityFaint(Pokemon* owner, Creature* source, const PokemonMoveType* move)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->faintEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityFaint] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->faintEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->faintEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	abilityScriptInterface->callVoidFunction(8);
}

int32_t Pokemons::executeAbilityBeforeHeal(Pokemon* owner, Creature* source, Creature* target,
	const PokemonMoveType* move, int32_t amount, bool ownerIsSource)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->beforeHealEvent == -1) {
		return amount;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityBeforeHeal] Call stack overflow" << std::endl;
		return amount;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->beforeHealEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->beforeHealEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	lua_pushinteger(L, amount);
	LuaScriptInterface::pushBoolean(L, ownerIsSource);

	int32_t result = amount;
	if (abilityScriptInterface->protectedCall(L, 11, 1) != 0) {
		LuaScriptInterface::reportError(nullptr, LuaScriptInterface::popString(L));
	} else {
		if (lua_isboolean(L, -1) && !LuaScriptInterface::getBoolean(L, -1)) {
			result = 0;
		} else if (lua_isnumber(L, -1)) {
			const lua_Number returnedAmount = lua_tonumber(L, -1);
			if (std::isfinite(returnedAmount)) {
				result = static_cast<int32_t>(std::clamp<lua_Number>(
					returnedAmount, 0, std::numeric_limits<int32_t>::max()));
			}
		}
		lua_pop(L, 1);
	}
	abilityScriptInterface->resetScriptEnv();
	return result;
}

void Pokemons::executeAbilityAfterHeal(Pokemon* owner, Creature* source, Creature* target,
	const PokemonMoveType* move, int32_t amount, bool ownerIsSource)
{
	const PokemonAbilityType* ability = owner ? getAbilityById(owner->getAbilityId()) : nullptr;
	if (!abilityScriptInterface || !ability || ability->afterHealEvent == -1) {
		return;
	}
	if (!abilityScriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - Pokemons::executeAbilityAfterHeal] Call stack overflow" << std::endl;
		return;
	}
	ScriptEnvironment* env = abilityScriptInterface->getScriptEnv();
	env->setScriptId(ability->afterHealEvent, abilityScriptInterface.get());
	lua_State* L = abilityScriptInterface->getLuaState();
	abilityScriptInterface->pushFunction(ability->afterHealEvent);
	LuaScriptInterface::pushUserdata<Pokemon>(L, owner);
	LuaScriptInterface::setMetatable(L, -1, "Pokemon");
	if (source) {
		LuaScriptInterface::pushUserdata<Creature>(L, source);
		LuaScriptInterface::setCreatureMetatable(L, -1, source);
	} else {
		lua_pushnil(L);
	}
	if (target) {
		LuaScriptInterface::pushUserdata<Creature>(L, target);
		LuaScriptInterface::setCreatureMetatable(L, -1, target);
	} else {
		lua_pushnil(L);
	}
	pushAbilityMoveContext(L, move);
	lua_pushinteger(L, amount);
	LuaScriptInterface::pushBoolean(L, ownerIsSource);
	abilityScriptInterface->callVoidFunction(11);
}

std::vector<uint16_t> learnPokemonMoves(PokemonInfo_t& info, const PokemonType& pokemonType)
{
	std::vector<uint16_t> learned;
	std::unordered_set<uint16_t> known;
	std::array<bool, 5> occupiedSlots = {};

	auto stateIt = info.moves.begin();
	while (stateIt != info.moves.end()) {
		if (!g_pokemons.getMoveById(stateIt->moveId) || !known.emplace(stateIt->moveId).second) {
			stateIt = info.moves.erase(stateIt);
			continue;
		}

		if (stateIt->activeSlot > 4 || (stateIt->activeSlot != 0 && occupiedSlots[stateIt->activeSlot])) {
			stateIt->activeSlot = 0;
		}
		if (stateIt->activeSlot != 0) {
			occupiedSlots[stateIt->activeSlot] = true;
		}
		++stateIt;
	}

	std::vector<PokemonMoveState*> activeMoves;
	for (PokemonMoveState& state : info.moves) {
		if (state.activeSlot != 0) {
			activeMoves.push_back(&state);
		}
	}
	std::sort(activeMoves.begin(), activeMoves.end(), [](const PokemonMoveState* lhs, const PokemonMoveState* rhs) {
		return lhs->activeSlot < rhs->activeSlot;
	});
	occupiedSlots.fill(false);
	uint8_t compactSlot = 1;
	for (PokemonMoveState* state : activeMoves) {
		state->activeSlot = compactSlot;
		occupiedSlots[compactSlot++] = true;
	}

	for (const PokemonLearnMove& learnMove : pokemonType.info.learnset) {
		if (learnMove.level > info.level || known.find(learnMove.moveId) != known.end()) {
			continue;
		}

		uint8_t activeSlot = 0;
		for (uint8_t slot = 1; slot <= 4; ++slot) {
			if (!occupiedSlots[slot]) {
				activeSlot = slot;
				occupiedSlots[slot] = true;
				break;
			}
		}

		info.moves.push_back({learnMove.moveId, activeSlot});
		known.emplace(learnMove.moveId);
		learned.push_back(learnMove.moveId);
	}
	return learned;
}

bool teachPokemonMove(PokemonInfo_t& info, uint16_t moveId)
{
	if (!g_pokemons.getMoveById(moveId)) {
		return false;
	}

	std::array<bool, 5> occupiedSlots = {};
	for (const PokemonMoveState& state : info.moves) {
		if (state.moveId == moveId) {
			return false;
		}
		if (state.activeSlot >= 1 && state.activeSlot <= 4) {
			occupiedSlots[state.activeSlot] = true;
		}
	}

	uint8_t activeSlot = 0;
	for (uint8_t slot = 1; slot <= 4; ++slot) {
		if (!occupiedSlots[slot]) {
			activeSlot = slot;
			break;
		}
	}

	info.moves.push_back({moveId, activeSlot});
	return true;
}

double getPokemonTypeEffectiveness(PokemonTypes_t attackingType, PokemonTypes_t defendingType)
{
	if (defendingType == TYPE_NONE) return 1.0;
	switch (attackingType) {
		case TYPE_NORMAL:
			if (defendingType == TYPE_GHOST) return 0.0;
			if (defendingType == TYPE_ROCK || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_FIRE:
			if (defendingType == TYPE_GRASS || defendingType == TYPE_ICE || defendingType == TYPE_BUG || defendingType == TYPE_STEEL) return 2.0;
			if (defendingType == TYPE_FIRE || defendingType == TYPE_WATER || defendingType == TYPE_ROCK || defendingType == TYPE_DRAGON) return 0.5;
			break;
		case TYPE_WATER:
			if (defendingType == TYPE_FIRE || defendingType == TYPE_GROUND || defendingType == TYPE_ROCK) return 2.0;
			if (defendingType == TYPE_WATER || defendingType == TYPE_GRASS || defendingType == TYPE_DRAGON) return 0.5;
			break;
		case TYPE_ELECTRIC:
			if (defendingType == TYPE_GROUND) return 0.0;
			if (defendingType == TYPE_WATER || defendingType == TYPE_FLYING) return 2.0;
			if (defendingType == TYPE_ELECTRIC || defendingType == TYPE_GRASS || defendingType == TYPE_DRAGON) return 0.5;
			break;
		case TYPE_GRASS:
			if (defendingType == TYPE_WATER || defendingType == TYPE_GROUND || defendingType == TYPE_ROCK) return 2.0;
			if (defendingType == TYPE_FIRE || defendingType == TYPE_GRASS || defendingType == TYPE_POISON || defendingType == TYPE_FLYING || defendingType == TYPE_BUG || defendingType == TYPE_DRAGON || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_ICE:
			if (defendingType == TYPE_GRASS || defendingType == TYPE_GROUND || defendingType == TYPE_FLYING || defendingType == TYPE_DRAGON) return 2.0;
			if (defendingType == TYPE_FIRE || defendingType == TYPE_WATER || defendingType == TYPE_ICE || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_FIGHTING:
			if (defendingType == TYPE_GHOST) return 0.0;
			if (defendingType == TYPE_NORMAL || defendingType == TYPE_ICE || defendingType == TYPE_ROCK || defendingType == TYPE_DARK || defendingType == TYPE_STEEL) return 2.0;
			if (defendingType == TYPE_POISON || defendingType == TYPE_FLYING || defendingType == TYPE_PSYCHIC || defendingType == TYPE_BUG || defendingType == TYPE_FAIRY) return 0.5;
			break;
		case TYPE_POISON:
			if (defendingType == TYPE_STEEL) return 0.0;
			if (defendingType == TYPE_GRASS || defendingType == TYPE_FAIRY) return 2.0;
			if (defendingType == TYPE_POISON || defendingType == TYPE_GROUND || defendingType == TYPE_ROCK || defendingType == TYPE_GHOST) return 0.5;
			break;
		case TYPE_GROUND:
			if (defendingType == TYPE_FLYING) return 0.0;
			if (defendingType == TYPE_FIRE || defendingType == TYPE_ELECTRIC || defendingType == TYPE_POISON || defendingType == TYPE_ROCK || defendingType == TYPE_STEEL) return 2.0;
			if (defendingType == TYPE_GRASS || defendingType == TYPE_BUG) return 0.5;
			break;
		case TYPE_FLYING:
			if (defendingType == TYPE_GRASS || defendingType == TYPE_FIGHTING || defendingType == TYPE_BUG) return 2.0;
			if (defendingType == TYPE_ELECTRIC || defendingType == TYPE_ROCK || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_PSYCHIC:
			if (defendingType == TYPE_DARK) return 0.0;
			if (defendingType == TYPE_FIGHTING || defendingType == TYPE_POISON) return 2.0;
			if (defendingType == TYPE_PSYCHIC || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_BUG:
			if (defendingType == TYPE_GRASS || defendingType == TYPE_PSYCHIC || defendingType == TYPE_DARK) return 2.0;
			if (defendingType == TYPE_FIRE || defendingType == TYPE_FIGHTING || defendingType == TYPE_POISON || defendingType == TYPE_FLYING || defendingType == TYPE_GHOST || defendingType == TYPE_STEEL || defendingType == TYPE_FAIRY) return 0.5;
			break;
		case TYPE_ROCK:
			if (defendingType == TYPE_FIRE || defendingType == TYPE_ICE || defendingType == TYPE_FLYING || defendingType == TYPE_BUG) return 2.0;
			if (defendingType == TYPE_FIGHTING || defendingType == TYPE_GROUND || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_GHOST:
			if (defendingType == TYPE_NORMAL) return 0.0;
			if (defendingType == TYPE_PSYCHIC || defendingType == TYPE_GHOST) return 2.0;
			if (defendingType == TYPE_DARK) return 0.5;
			break;
		case TYPE_DRAGON:
			if (defendingType == TYPE_FAIRY) return 0.0;
			if (defendingType == TYPE_DRAGON) return 2.0;
			if (defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_DARK:
			if (defendingType == TYPE_PSYCHIC || defendingType == TYPE_GHOST) return 2.0;
			if (defendingType == TYPE_FIGHTING || defendingType == TYPE_DARK || defendingType == TYPE_FAIRY) return 0.5;
			break;
		case TYPE_STEEL:
			if (defendingType == TYPE_ICE || defendingType == TYPE_ROCK || defendingType == TYPE_FAIRY) return 2.0;
			if (defendingType == TYPE_FIRE || defendingType == TYPE_WATER || defendingType == TYPE_ELECTRIC || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_FAIRY:
			if (defendingType == TYPE_FIGHTING || defendingType == TYPE_DRAGON || defendingType == TYPE_DARK) return 2.0;
			if (defendingType == TYPE_FIRE || defendingType == TYPE_POISON || defendingType == TYPE_STEEL) return 0.5;
			break;
		case TYPE_NONE:
		default:
			break;
	}
	return 1.0;
}

bool Pokemons::reload()
{
	loaded = false;

	scriptInterface.reset();

	return loadFromXml(true);
}


PokemonType* Pokemons::loadPokemon(const std::string& file, const std::string& pokemonName, bool reloading /*= false*/)
{
	PokemonType* mType = nullptr;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(file.c_str());
	if (!result) {
		printXMLError("Error - Pokemons::loadPokemon", file, result);
		return nullptr;
	}

	pugi::xml_node pokemonNode = doc.child("pokemon");
	if (!pokemonNode) {
		std::cout << "[Error - Pokemons::loadPokemon] Missing pokemon node in: " << file << std::endl;
		return nullptr;
	}

	pugi::xml_attribute attr;
	if (!(attr = pokemonNode.attribute("name"))) {
		std::cout << "[Error - Pokemons::loadPokemon] Missing name in: " << file << std::endl;
		return nullptr;
	}

	if (reloading) {
		auto it = pokemons.find(asLowerCaseString(pokemonName));
		if (it != pokemons.end()) {
			mType = &it->second;
			mType->info = {};
		}
	}

	if (!mType) {
		mType = &pokemons[asLowerCaseString(pokemonName)];
	}

	mType->name = attr.as_string();

	if ((attr = pokemonNode.attribute("nameDescription"))) {
		mType->nameDescription = attr.as_string();
	} else {
		mType->nameDescription = "a " + asLowerCaseString(mType->name);
	}

	if ((attr = pokemonNode.attribute("race"))) {
		std::string tmpStrValue = asLowerCaseString(attr.as_string());
		uint16_t tmpInt = pugi::cast<uint16_t>(attr.value());
		if (tmpStrValue == "venom" || tmpInt == 1) {
			mType->info.race = RACE_VENOM;
		} else if (tmpStrValue == "blood" || tmpInt == 2) {
			mType->info.race = RACE_BLOOD;
		} else if (tmpStrValue == "undead" || tmpInt == 3) {
			mType->info.race = RACE_UNDEAD;
		} else if (tmpStrValue == "fire" || tmpInt == 4) {
			mType->info.race = RACE_FIRE;
		} else if (tmpStrValue == "energy" || tmpInt == 5) {
			mType->info.race = RACE_ENERGY;
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Unknown race type " << attr.as_string() << ". " << file << std::endl;
		}
	}

	if ((attr = pokemonNode.attribute("experience"))) {
		mType->info.experience = pugi::cast<uint64_t>(attr.value());
	}

	if ((attr = pokemonNode.attribute("speed"))) {
		mType->info.baseSpeed = pugi::cast<int32_t>(attr.value());
	}

	if ((attr = pokemonNode.attribute("script"))) {
		if (!scriptInterface) {
			scriptInterface.reset(new LuaScriptInterface("Pokemon Interface"));
			scriptInterface->initState();
		}

		std::string script = attr.as_string();
		if (scriptInterface->loadFile("data/pokemon/scripts/" + script) == 0) {
			mType->info.scriptInterface = scriptInterface.get();
			mType->info.creatureAppearEvent = scriptInterface->getEvent("onCreatureAppear");
			mType->info.creatureDisappearEvent = scriptInterface->getEvent("onCreatureDisappear");
			mType->info.creatureMoveEvent = scriptInterface->getEvent("onCreatureMove");
			mType->info.creatureSayEvent = scriptInterface->getEvent("onCreatureSay");
			mType->info.thinkEvent = scriptInterface->getEvent("onThink");
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Can not load script: " << script << std::endl;
			std::cout << scriptInterface->getLastLuaError() << std::endl;
		}
	}

	pugi::xml_node node;
	if ((node = pokemonNode.child("health"))) {
		if ((attr = node.attribute("now"))) {
			mType->info.health = pugi::cast<int32_t>(attr.value());
		} else {
			std::cout << "[Error - Pokemons::loadPokemon] Missing health now. " << file << std::endl;
		}

		if ((attr = node.attribute("max"))) {
			mType->info.healthMax = pugi::cast<int32_t>(attr.value());
		} else {
			std::cout << "[Error - Pokemons::loadPokemon] Missing health max. " << file << std::endl;
		}

		if (mType->info.health > mType->info.healthMax) {
			mType->info.health = mType->info.healthMax;
			std::cout << "[Warning - Pokemons::loadPokemon] Health now is greater than health max." << file << std::endl;
		}
	}

	if ((node = pokemonNode.child("flags"))) {
		for (auto flagNode : node.children()) {
			attr = flagNode.first_attribute();
			const char* attrName = attr.name();
			if (strcasecmp(attrName, "summonable") == 0) {
				mType->info.isSummonable = attr.as_bool();
			} else if (strcasecmp(attrName, "attackable") == 0) {
				mType->info.isAttackable = attr.as_bool();
			} else if (strcasecmp(attrName, "hostile") == 0) {
				mType->info.isHostile = attr.as_bool();
			} else if (strcasecmp(attrName, "ignorespawnblock") == 0) {
				mType->info.isIgnoringSpawnBlock = attr.as_bool();
			} else if (strcasecmp(attrName, "illusionable") == 0) {
				mType->info.isIllusionable = attr.as_bool();
			} else if (strcasecmp(attrName, "challengeable") == 0) {
				mType->info.isChallengeable = attr.as_bool();
			} else if (strcasecmp(attrName, "convinceable") == 0) {
				mType->info.isConvinceable = attr.as_bool();
			} else if (strcasecmp(attrName, "pushable") == 0) {
				mType->info.pushable = attr.as_bool();
			} else if (strcasecmp(attrName, "isboss") == 0) {
				mType->info.isBoss = attr.as_bool();
			} else if (strcasecmp(attrName, "canpushitems") == 0) {
				mType->info.canPushItems = attr.as_bool();
			} else if (strcasecmp(attrName, "canpushcreatures") == 0) {
				mType->info.canPushCreatures = attr.as_bool();
			} else if (strcasecmp(attrName, "staticattack") == 0) {
				uint32_t staticAttack = pugi::cast<uint32_t>(attr.value());
				if (staticAttack > 100) {
					std::cout << "[Warning - Pokemons::loadPokemon] staticattack greater than 100. " << file << std::endl;
					staticAttack = 100;
				}

				mType->info.staticAttackChance = staticAttack;
			} else if (strcasecmp(attrName, "lightlevel") == 0) {
				mType->info.light.level = pugi::cast<uint16_t>(attr.value());
			} else if (strcasecmp(attrName, "lightcolor") == 0) {
				mType->info.light.color = pugi::cast<uint16_t>(attr.value());
			} else if (strcasecmp(attrName, "targetdistance") == 0) {
				int32_t targetDistance = pugi::cast<int32_t>(attr.value());
				if (targetDistance < 1) {
					targetDistance = 1;
					std::cout << "[Warning - Pokemons::loadPokemon] targetdistance less than 1. " << file << std::endl;
				}
				mType->info.targetDistance = targetDistance;
			} else if (strcasecmp(attrName, "runonhealth") == 0) {
				mType->info.runAwayHealth = pugi::cast<int32_t>(attr.value());
			} else if (strcasecmp(attrName, "hidehealth") == 0) {
				mType->info.hiddenHealth = attr.as_bool();
			} else if (strcasecmp(attrName, "canwalkonenergy") == 0) {
				mType->info.canWalkOnEnergy = attr.as_bool();
			} else if (strcasecmp(attrName, "canwalkonfire") == 0) {
				mType->info.canWalkOnFire = attr.as_bool();
			} else if (strcasecmp(attrName, "canwalkonpoison") == 0) {
				mType->info.canWalkOnPoison = attr.as_bool();
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Unknown flag attribute: " << attrName << ". " << file << std::endl;
			}
		}

		// if a pokemon can push creatures,
		// it should not be pushable.
		if (mType->info.canPushCreatures) {
			mType->info.pushable = false;
		}
	}
	if ((node = pokemonNode.child("targetchange"))) {
		if ((attr = node.attribute("speed")) || (attr = node.attribute("interval"))) {
			mType->info.changeTargetSpeed = pugi::cast<uint32_t>(attr.value());
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Missing targetchange speed. " << file << std::endl;
		}

		if ((attr = node.attribute("chance"))) {
			int32_t chance = pugi::cast<int32_t>(attr.value());
			if (chance > 100) {
				chance = 100;
				std::cout << "[Warning - Pokemons::loadPokemon] targetchange chance value out of bounds. " << file << std::endl;
			}
			mType->info.changeTargetChance = chance;
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Missing targetchange chance. " << file << std::endl;
		}
	}

	if ((node = pokemonNode.child("look"))) {
		if ((attr = node.attribute("type"))) {
			mType->info.outfit.lookType = pugi::cast<uint16_t>(attr.value());

			if ((attr = node.attribute("head"))) {
				mType->info.outfit.lookHead = pugi::cast<uint16_t>(attr.value());
			}

			if ((attr = node.attribute("body"))) {
				mType->info.outfit.lookBody = pugi::cast<uint16_t>(attr.value());
			}

			if ((attr = node.attribute("legs"))) {
				mType->info.outfit.lookLegs = pugi::cast<uint16_t>(attr.value());
			}

			if ((attr = node.attribute("feet"))) {
				mType->info.outfit.lookFeet = pugi::cast<uint16_t>(attr.value());
			}

			if ((attr = node.attribute("addons"))) {
				mType->info.outfit.lookAddons = pugi::cast<uint16_t>(attr.value());
			}
		} else if ((attr = node.attribute("typeex"))) {
			mType->info.outfit.lookTypeEx = pugi::cast<uint16_t>(attr.value());
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Missing look type/typeex. " << file << std::endl;
		}

		if ((attr = node.attribute("mount"))) {
			mType->info.outfit.lookMount = pugi::cast<uint16_t>(attr.value());
		}

		if ((attr = node.attribute("corpse"))) {
			mType->info.lookcorpse = pugi::cast<uint16_t>(attr.value());
		}
	}

	if ((node = pokemonNode.child("immunities"))) {
		for (auto immunityNode : node.children()) {
			if ((attr = immunityNode.attribute("name"))) {
				std::string tmpStrValue = asLowerCaseString(attr.as_string());
				if (tmpStrValue == "physical") {
					mType->info.damageImmunities |= COMBAT_PHYSICALDAMAGE;
					mType->info.conditionImmunities |= CONDITION_BLEEDING;
				} else if (tmpStrValue == "energy") {
					mType->info.damageImmunities |= COMBAT_ENERGYDAMAGE;
					mType->info.conditionImmunities |= CONDITION_ENERGY;
				} else if (tmpStrValue == "fire") {
					mType->info.damageImmunities |= COMBAT_FIREDAMAGE;
					mType->info.conditionImmunities |= CONDITION_FIRE;
				} else if (tmpStrValue == "poison" ||
							tmpStrValue == "earth") {
					mType->info.damageImmunities |= COMBAT_EARTHDAMAGE;
					mType->info.conditionImmunities |= CONDITION_POISON;
				} else if (tmpStrValue == "drown") {
					mType->info.damageImmunities |= COMBAT_DROWNDAMAGE;
					mType->info.conditionImmunities |= CONDITION_DROWN;
				} else if (tmpStrValue == "ice") {
					mType->info.damageImmunities |= COMBAT_ICEDAMAGE;
					mType->info.conditionImmunities |= CONDITION_FREEZING;
				} else if (tmpStrValue == "holy") {
					mType->info.damageImmunities |= COMBAT_HOLYDAMAGE;
					mType->info.conditionImmunities |= CONDITION_DAZZLED;
				} else if (tmpStrValue == "death") {
					mType->info.damageImmunities |= COMBAT_DEATHDAMAGE;
					mType->info.conditionImmunities |= CONDITION_CURSED;
				} else if (tmpStrValue == "lifedrain") {
					mType->info.damageImmunities |= COMBAT_LIFEDRAIN;
				} else if (tmpStrValue == "paralyze") {
					mType->info.conditionImmunities |= CONDITION_PARALYZE;
				} else if (tmpStrValue == "outfit") {
					mType->info.conditionImmunities |= CONDITION_OUTFIT;
				} else if (tmpStrValue == "drunk") {
					mType->info.conditionImmunities |= CONDITION_DRUNK;
				} else if (tmpStrValue == "invisible" || tmpStrValue == "invisibility") {
					mType->info.conditionImmunities |= CONDITION_INVISIBLE;
				} else if (tmpStrValue == "bleed") {
					mType->info.conditionImmunities |= CONDITION_BLEEDING;
				} else {
					std::cout << "[Warning - Pokemons::loadPokemon] Unknown immunity name " << attr.as_string() << ". " << file << std::endl;
				}
			} else if ((attr = immunityNode.attribute("physical"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_PHYSICALDAMAGE;
					mType->info.conditionImmunities |= CONDITION_BLEEDING;
				}
			} else if ((attr = immunityNode.attribute("energy"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_ENERGYDAMAGE;
					mType->info.conditionImmunities |= CONDITION_ENERGY;
				}
			} else if ((attr = immunityNode.attribute("fire"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_FIREDAMAGE;
					mType->info.conditionImmunities |= CONDITION_FIRE;
				}
			} else if ((attr = immunityNode.attribute("poison")) || (attr = immunityNode.attribute("earth"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_EARTHDAMAGE;
					mType->info.conditionImmunities |= CONDITION_POISON;
				}
			} else if ((attr = immunityNode.attribute("drown"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_DROWNDAMAGE;
					mType->info.conditionImmunities |= CONDITION_DROWN;
				}
			} else if ((attr = immunityNode.attribute("ice"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_ICEDAMAGE;
					mType->info.conditionImmunities |= CONDITION_FREEZING;
				}
			} else if ((attr = immunityNode.attribute("holy"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_HOLYDAMAGE;
					mType->info.conditionImmunities |= CONDITION_DAZZLED;
				}
			} else if ((attr = immunityNode.attribute("death"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_DEATHDAMAGE;
					mType->info.conditionImmunities |= CONDITION_CURSED;
				}
			} else if ((attr = immunityNode.attribute("lifedrain"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_LIFEDRAIN;
				}
			} else if ((attr = immunityNode.attribute("paralyze"))) {
				if (attr.as_bool()) {
					mType->info.conditionImmunities |= CONDITION_PARALYZE;
				}
			} else if ((attr = immunityNode.attribute("outfit"))) {
				if (attr.as_bool()) {
					mType->info.conditionImmunities |= CONDITION_OUTFIT;
				}
			} else if ((attr = immunityNode.attribute("bleed"))) {
				if (attr.as_bool()) {
					mType->info.conditionImmunities |= CONDITION_BLEEDING;
				}
			} else if ((attr = immunityNode.attribute("drunk"))) {
				if (attr.as_bool()) {
					mType->info.conditionImmunities |= CONDITION_DRUNK;
				}
			} else if ((attr = immunityNode.attribute("invisible")) || (attr = immunityNode.attribute("invisibility"))) {
				if (attr.as_bool()) {
					mType->info.conditionImmunities |= CONDITION_INVISIBLE;
				}
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Unknown immunity. " << file << std::endl;
			}
		}
	}

	if ((node = pokemonNode.child("voices"))) {
		if ((attr = node.attribute("speed")) || (attr = node.attribute("interval"))) {
			mType->info.yellSpeedTicks = pugi::cast<uint32_t>(attr.value());
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Missing voices speed. " << file << std::endl;
		}

		if ((attr = node.attribute("chance"))) {
			uint32_t chance = pugi::cast<uint32_t>(attr.value());
			if (chance > 100) {
				chance = 100;
				std::cout << "[Warning - Pokemons::loadPokemon] yell chance value out of bounds. " << file << std::endl;
			}
			mType->info.yellChance = chance;
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Missing voices chance. " << file << std::endl;
		}

		for (auto voiceNode : node.children()) {
			voiceBlock_t vb;
			if ((attr = voiceNode.attribute("sentence"))) {
				vb.text = attr.as_string();
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Missing voice sentence. " << file << std::endl;
			}

			if ((attr = voiceNode.attribute("yell"))) {
				vb.yellText = attr.as_bool();
			} else {
				vb.yellText = false;
			}
			mType->info.voiceVector.emplace_back(vb);
		}
	}

	if ((node = pokemonNode.child("loot"))) {
		for (auto lootNode : node.children()) {
			LootBlock lootBlock;
			if (loadLootItem(lootNode, lootBlock)) {
				mType->info.lootItems.emplace_back(std::move(lootBlock));
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Cant load loot. " << file << std::endl;
			}
		}
	}

	if ((node = pokemonNode.child("elements"))) {
		for (auto elementNode : node.children()) {
			if ((attr = elementNode.attribute("physicalPercent"))) {
				mType->info.elementMap[COMBAT_PHYSICALDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_PHYSICALDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"physical\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("icePercent"))) {
				mType->info.elementMap[COMBAT_ICEDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_ICEDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"ice\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("poisonPercent")) || (attr = elementNode.attribute("earthPercent"))) {
				mType->info.elementMap[COMBAT_EARTHDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_EARTHDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"earth\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("firePercent"))) {
				mType->info.elementMap[COMBAT_FIREDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_FIREDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"fire\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("energyPercent"))) {
				mType->info.elementMap[COMBAT_ENERGYDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_ENERGYDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"energy\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("holyPercent"))) {
				mType->info.elementMap[COMBAT_HOLYDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_HOLYDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"holy\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("deathPercent"))) {
				mType->info.elementMap[COMBAT_DEATHDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_DEATHDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"death\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("drownPercent"))) {
				mType->info.elementMap[COMBAT_DROWNDAMAGE] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_DROWNDAMAGE) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"drown\" on immunity and element tags. " << file << std::endl;
				}
			} else if ((attr = elementNode.attribute("lifedrainPercent"))) {
				mType->info.elementMap[COMBAT_LIFEDRAIN] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_LIFEDRAIN) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"lifedrain\" on immunity and element tags. " << file << std::endl;
				}
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Unknown element percent. " << file << std::endl;
			}
		}
	}

	if ((node = pokemonNode.child("summons"))) {
		if ((attr = node.attribute("maxSummons"))) {
			mType->info.maxSummons = std::min<uint32_t>(pugi::cast<uint32_t>(attr.value()), 100);
		} else {
			std::cout << "[Warning - Pokemons::loadPokemon] Missing summons maxSummons. " << file << std::endl;
		}

		for (auto summonNode : node.children()) {
			int32_t chance = 100;
			int32_t speed = 1000;
			int32_t max = mType->info.maxSummons;
			bool force = false;

			if ((attr = summonNode.attribute("speed")) || (attr = summonNode.attribute("interval"))) {
				speed = std::max<int32_t>(1, pugi::cast<int32_t>(attr.value()));
			}

			if ((attr = summonNode.attribute("chance"))) {
				chance = pugi::cast<int32_t>(attr.value());
				if (chance > 100) {
					chance = 100;
					std::cout << "[Warning - Pokemons::loadPokemon] Summon chance value out of bounds. " << file << std::endl;
				}
			}

			if ((attr = summonNode.attribute("max"))) {
				max = pugi::cast<uint32_t>(attr.value());
			}

			if ((attr = summonNode.attribute("force"))) {
				force = attr.as_bool();
			}

			if ((attr = summonNode.attribute("name"))) {
				summonBlock_t sb;
				sb.name = attr.as_string();
				sb.speed = speed;
				sb.chance = chance;
				sb.max = max;
				sb.force = force;
				mType->info.summons.emplace_back(sb);
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Missing summon name. " << file << std::endl;
			}
		}
	}

	if ((node = pokemonNode.child("script"))) {
		for (auto eventNode : node.children()) {
			if ((attr = eventNode.attribute("name"))) {
				mType->info.scripts.emplace_back(attr.as_string());
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Missing name for script event. " << file << std::endl;
			}
		}
	}

	mType->info.summons.shrink_to_fit();
	mType->info.lootItems.shrink_to_fit();
	mType->info.voiceVector.shrink_to_fit();
	mType->info.scripts.shrink_to_fit();
	return mType;
}

bool PokemonType::loadCallback(LuaScriptInterface* scriptInterface)
{
	int32_t id = scriptInterface->getEvent();
	if (id == -1) {
		std::cout << "[Warning - PokemonType::loadCallback] Event not found. " << std::endl;
		return false;
	}

	info.scriptInterface = scriptInterface;
	if (info.eventType == POKEMONS_EVENT_THINK) {
		info.thinkEvent = id;
	} else if (info.eventType == POKEMONS_EVENT_APPEAR) {
		info.creatureAppearEvent = id;
	} else if (info.eventType == POKEMONS_EVENT_DISAPPEAR) {
		info.creatureDisappearEvent = id;
	} else if (info.eventType == POKEMONS_EVENT_MOVE) {
		info.creatureMoveEvent = id;
	} else if (info.eventType == POKEMONS_EVENT_SAY) {
		info.creatureSayEvent = id;
	}
	return true;
}

bool Pokemons::loadLootItem(const pugi::xml_node& node, LootBlock& lootBlock)
{
	pugi::xml_attribute attr;
	if ((attr = node.attribute("id"))) {
		int32_t id = pugi::cast<int32_t>(attr.value());
		const ItemType& it = Item::items.getItemType(id);

		if (it.name.empty()) {
			std::cout << "[Warning - Pokemons::loadPokemon] Unknown loot item id \"" << id << "\". " << std::endl;
			return false;
		}

		lootBlock.id = id;

	} else if ((attr = node.attribute("name"))) {
		auto name = attr.as_string();
		auto ids = Item::items.nameToItems.equal_range(asLowerCaseString(name));

		if (ids.first == Item::items.nameToItems.cend()) {
			std::cout << "[Warning - Pokemons::loadPokemon] Unknown loot item \"" << name << "\". " << std::endl;
			return false;
		}

		uint32_t id = ids.first->second;

		if (std::next(ids.first) != ids.second) {
			std::cout << "[Warning - Pokemons::loadPokemon] Non-unique loot item \"" << name << "\". " << std::endl;
			return false;
		}

		lootBlock.id = id;
	}

	if (lootBlock.id == 0) {
		return false;
	}

	if ((attr = node.attribute("countmax"))) {
		int32_t lootCountMax = pugi::cast<int32_t>(attr.value());
		lootBlock.countmax = std::max<int32_t>(1, lootCountMax);
	} else {
		lootBlock.countmax = 1;
	}

	if ((attr = node.attribute("chance")) || (attr = node.attribute("chance1"))) {
		int32_t lootChance = pugi::cast<int32_t>(attr.value());
		if (lootChance > static_cast<int32_t>(MAX_LOOTCHANCE)) {
			std::cout << "[Warning - Pokemons::loadPokemon] Invalid \"chance\" "<< lootChance <<" used for loot, the max is " << MAX_LOOTCHANCE << ". " << std::endl;
		}
		lootBlock.chance = std::min<int32_t>(MAX_LOOTCHANCE, lootChance);
	} else {
		lootBlock.chance = MAX_LOOTCHANCE;
	}

	if (Item::items[lootBlock.id].isContainer()) {
		loadLootContainer(node, lootBlock);
	}

	//optional
	if ((attr = node.attribute("subtype"))) {
		lootBlock.subType = pugi::cast<int32_t>(attr.value());
	} else {
		uint32_t charges = Item::items[lootBlock.id].charges;
		if (charges != 0) {
			lootBlock.subType = charges;
		}
	}

	if ((attr = node.attribute("actionId"))) {
		lootBlock.actionId = pugi::cast<int32_t>(attr.value());
	}

	if ((attr = node.attribute("text"))) {
		lootBlock.text = attr.as_string();
	}
	return true;
}

void Pokemons::loadLootContainer(const pugi::xml_node& node, LootBlock& lBlock)
{
	// NOTE: <inside> attribute was left for backwards compatibility with pre 1.x TFS versions.
	// Please don't use it, if you don't have to.
	for (auto subNode : node.child("inside") ? node.child("inside").children() : node.children()) {
		LootBlock lootBlock;
		if (loadLootItem(subNode, lootBlock)) {
			lBlock.childLoot.emplace_back(std::move(lootBlock));
		}
	}
}

PokemonType* Pokemons::getPokemonType(const std::string& name, bool loadFromFile /*= true */)
{
	std::string lowerCaseName = asLowerCaseString(name);

	auto it = pokemons.find(lowerCaseName);
	if (it == pokemons.end()) {
		if (!loadFromFile) {
			return nullptr;
		}

		auto it2 = unloadedPokemons.find(lowerCaseName);
		if (it2 == unloadedPokemons.end()) {
			return nullptr;
		}

		return loadPokemon(it2->second, name);
	}
	return &it->second;
}

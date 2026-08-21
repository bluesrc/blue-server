// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "pokemons.h"
#include "pokemon.h"
#include "moves.h"
#include "combat.h"
#include "weapons.h"
#include "configmanager.h"
#include "game.h"

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

moveBlock_t::~moveBlock_t()
{
	if (combatMove) {
		delete move;
	}
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
	for (const pugi::xml_node& node : doc.child("moves").children("instant")) {
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

ConditionDamage* Pokemons::getDamageCondition(ConditionType_t conditionType,
        int32_t maxDamage, int32_t minDamage, int32_t startDamage, uint32_t tickInterval)
{
	ConditionDamage* condition = static_cast<ConditionDamage*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, 0, 0));
	condition->setParam(CONDITION_PARAM_TICKINTERVAL, tickInterval);
	condition->setParam(CONDITION_PARAM_MINVALUE, minDamage);
	condition->setParam(CONDITION_PARAM_MAXVALUE, maxDamage);
	condition->setParam(CONDITION_PARAM_STARTVALUE, startDamage);
	condition->setParam(CONDITION_PARAM_DELAYED, 1);
	return condition;
}

bool Pokemons::deserializeMove(const pugi::xml_node& node, moveBlock_t& sb, const std::string& description)
{
	std::string name;
	std::string scriptName;
	bool isScripted;

	pugi::xml_attribute attr;
	if ((attr = node.attribute("script"))) {
		scriptName = attr.as_string();
		isScripted = true;
	} else if ((attr = node.attribute("name"))) {
		name = attr.as_string();
		isScripted = false;
	} else {
		return false;
	}

	if ((attr = node.attribute("speed")) || (attr = node.attribute("interval"))) {
		sb.speed = std::max<int32_t>(1, pugi::cast<int32_t>(attr.value()));
	}

	if ((attr = node.attribute("chance"))) {
		uint32_t chance = pugi::cast<uint32_t>(attr.value());
		if (chance > 100) {
			chance = 100;
			std::cout << "[Warning - Pokemons::deserializeMove] " << description << " - Chance value out of bounds for move: " << name << std::endl;
		}
		sb.chance = chance;
	} else if (asLowerCaseString(name) != "melee") {
		std::cout << "[Warning - Pokemons::deserializeMove] " << description << " - Missing chance value on non-melee move: " << name << std::endl;
	}

	if ((attr = node.attribute("range"))) {
		uint32_t range = pugi::cast<uint32_t>(attr.value());
		if (range > (Map::maxViewportX * 2)) {
			range = Map::maxViewportX * 2;
		}
		sb.range = range;
	}

	if ((attr = node.attribute("min"))) {
		sb.minCombatValue = pugi::cast<int32_t>(attr.value());
	}

	if ((attr = node.attribute("max"))) {
		sb.maxCombatValue = pugi::cast<int32_t>(attr.value());

		//normalize values
		if (std::abs(sb.minCombatValue) > std::abs(sb.maxCombatValue)) {
			int32_t value = sb.maxCombatValue;
			sb.maxCombatValue = sb.minCombatValue;
			sb.minCombatValue = value;
		}
	}

	if (auto move = g_moves->getMoveByName(name)) {
		sb.move = move;
		return true;
	}

	CombatMove* combatMove = nullptr;
	bool needTarget = false;
	bool needDirection = false;

	if (isScripted) {
		if ((attr = node.attribute("direction"))) {
			needDirection = attr.as_bool();
		}

		if ((attr = node.attribute("target"))) {
			needTarget = attr.as_bool();
		}

		std::unique_ptr<CombatMove> combatMovePtr(new CombatMove(nullptr, needTarget, needDirection));
		if (!combatMovePtr->loadScript("data/" + g_moves->getScriptBaseName() + "/scripts/" + scriptName)) {
			return false;
		}

		if (!combatMovePtr->loadScriptCombat()) {
			return false;
		}

		combatMove = combatMovePtr.release();
		combatMove->getCombat()->setPlayerCombatValues(COMBAT_FORMULA_DAMAGE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
	} else {
		Combat_ptr combat = std::make_shared<Combat>();
		if ((attr = node.attribute("length"))) {
			int32_t length = pugi::cast<int32_t>(attr.value());
			if (length > 0) {
				int32_t spread = 3;

				//need direction move
				if ((attr = node.attribute("spread"))) {
					spread = std::max<int32_t>(0, pugi::cast<int32_t>(attr.value()));
				}

				AreaCombat* area = new AreaCombat();
				area->setupArea(length, spread);
				combat->setArea(area);

				needDirection = true;
			}
		}

		if ((attr = node.attribute("radius"))) {
			int32_t radius = pugi::cast<int32_t>(attr.value());

			//target move
			if ((attr = node.attribute("target"))) {
				needTarget = attr.as_bool();
			}

			AreaCombat* area = new AreaCombat();
			area->setupArea(radius);
			combat->setArea(area);
		}

		std::string tmpName = asLowerCaseString(name);

		if (tmpName == "melee") {
			sb.isMelee = true;

			pugi::xml_attribute attackAttribute, skillAttribute;
			if ((attackAttribute = node.attribute("attack")) && (skillAttribute = node.attribute("skill"))) {
				sb.minCombatValue = 0;
				sb.maxCombatValue = -Weapons::getMaxMeleeDamage(pugi::cast<int32_t>(skillAttribute.value()), pugi::cast<int32_t>(attackAttribute.value()));
			}

			ConditionType_t conditionType = CONDITION_NONE;
			int32_t minDamage = 0;
			int32_t maxDamage = 0;
			uint32_t tickInterval = 2000;

			if ((attr = node.attribute("fire"))) {
				conditionType = CONDITION_FIRE;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 9000;
			} else if ((attr = node.attribute("poison"))) {
				conditionType = CONDITION_POISON;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 4000;
			} else if ((attr = node.attribute("energy"))) {
				conditionType = CONDITION_ENERGY;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 10000;
			} else if ((attr = node.attribute("drown"))) {
				conditionType = CONDITION_DROWN;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 5000;
			} else if ((attr = node.attribute("freeze"))) {
				conditionType = CONDITION_FREEZING;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 8000;
			} else if ((attr = node.attribute("dazzle"))) {
				conditionType = CONDITION_DAZZLED;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 10000;
			} else if ((attr = node.attribute("curse"))) {
				conditionType = CONDITION_CURSED;

				minDamage = pugi::cast<int32_t>(attr.value());
				maxDamage = minDamage;
				tickInterval = 4000;
			} else if ((attr = node.attribute("bleed")) || (attr = node.attribute("physical"))) {
				conditionType = CONDITION_BLEEDING;
				tickInterval = 4000;
			}

			if ((attr = node.attribute("tick"))) {
				int32_t value = pugi::cast<int32_t>(attr.value());
				if (value > 0) {
					tickInterval = value;
				}
			}

			if (conditionType != CONDITION_NONE) {
				Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, 0, tickInterval);
				combat->addCondition(condition);
			}

			sb.range = 1;
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBAT_PARAM_BLOCKARMOR, 1);
			combat->setParam(COMBAT_PARAM_BLOCKSHIELD, 1);
			combat->setOrigin(ORIGIN_MELEE);
		} else if (tmpName == "physical") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBAT_PARAM_BLOCKARMOR, 1);
			combat->setOrigin(ORIGIN_RANGED);
		} else if (tmpName == "bleed") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE);
		} else if (tmpName == "poison" || tmpName == "earth") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_EARTHDAMAGE);
		} else if (tmpName == "fire") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_FIREDAMAGE);
		} else if (tmpName == "energy") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_ENERGYDAMAGE);
		} else if (tmpName == "drown") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_DROWNDAMAGE);
		} else if (tmpName == "ice") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_ICEDAMAGE);
		} else if (tmpName == "holy") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_HOLYDAMAGE);
		} else if (tmpName == "death") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_DEATHDAMAGE);
		} else if (tmpName == "lifedrain") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_LIFEDRAIN);
		} else if (tmpName == "manadrain") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_MANADRAIN);
		} else if (tmpName == "healing") {
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_HEALING);
			combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
		} else if (tmpName == "speed") {
			int32_t minSpeedChange = 0;
			int32_t maxSpeedChange = 0;
			int32_t duration = 10000;

			if ((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			if ((attr = node.attribute("speedchange"))) {
				minSpeedChange = pugi::cast<int32_t>(attr.value());
			} else if ((attr = node.attribute("minspeedchange"))) {
				minSpeedChange = pugi::cast<int32_t>(attr.value());

				if ((attr = node.attribute("maxspeedchange"))) {
					maxSpeedChange = pugi::cast<int32_t>(attr.value());
				}

				if (minSpeedChange == 0) {
					std::cout << "[Error - Pokemons::deserializeMove] - " << description << " - missing speedchange/minspeedchange value" << std::endl;
					return false;
				}

				if (maxSpeedChange == 0) {
					maxSpeedChange = minSpeedChange; // static speedchange value
				}
			}

			if (minSpeedChange < -1000) {
				std::cout << "[Warning - Pokemons::deserializeMove] - " << description << " - you cannot reduce a creatures speed below -1000 (100%)" << std::endl;
				minSpeedChange = -1000;
			}

			ConditionType_t conditionType;
			if (minSpeedChange >= 0) {
				conditionType = CONDITION_HASTE;
				combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
			} else {
				conditionType = CONDITION_PARALYZE;
			}

			ConditionSpeed* condition = static_cast<ConditionSpeed*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, duration, 0));
			condition->setFormulaVars(minSpeedChange / 1000.0, 0, maxSpeedChange / 1000.0, 0);
			combat->addCondition(condition);
		} else if (tmpName == "outfit") {
			int32_t duration = 10000;

			if ((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			if ((attr = node.attribute("pokemon"))) {
				PokemonType* mType = g_pokemons.getPokemonType(attr.as_string());
				if (mType) {
					ConditionOutfit* condition = static_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
					condition->setOutfit(mType->info.outfit);
					combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
					combat->addCondition(condition);
				}
			} else if ((attr = node.attribute("item"))) {
				Outfit_t outfit;
				outfit.lookTypeEx = pugi::cast<uint16_t>(attr.value());

				ConditionOutfit* condition = static_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
				condition->setOutfit(outfit);
				combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
				combat->addCondition(condition);
			}
		} else if (tmpName == "invisible") {
			int32_t duration = 10000;

			if ((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_INVISIBLE, duration, 0);
			combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
			combat->addCondition(condition);
		} else if (tmpName == "drunk") {
			int32_t duration = 10000;
			uint8_t drunkenness = 25;

			if ((attr = node.attribute("duration"))) {
				duration = pugi::cast<int32_t>(attr.value());
			}

			if ((attr = node.attribute("drunkenness"))) {
				drunkenness = pugi::cast<uint8_t>(attr.value());
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_DRUNK, duration, drunkenness);
			combat->addCondition(condition);
		} else if (tmpName == "firefield") {
			combat->setParam(COMBAT_PARAM_CREATEITEM, ITEM_FIREFIELD_PVP_FULL);
		} else if (tmpName == "poisonfield") {
			combat->setParam(COMBAT_PARAM_CREATEITEM, ITEM_POISONFIELD_PVP);
		} else if (tmpName == "energyfield") {
			combat->setParam(COMBAT_PARAM_CREATEITEM, ITEM_ENERGYFIELD_PVP);
		} else if (tmpName == "firecondition" || tmpName == "energycondition" ||
				   tmpName == "earthcondition" || tmpName == "poisoncondition" ||
				   tmpName == "icecondition" || tmpName == "freezecondition" ||
				   tmpName == "deathcondition" || tmpName == "cursecondition" ||
				   tmpName == "holycondition" || tmpName == "dazzlecondition" ||
				   tmpName == "drowncondition" || tmpName == "bleedcondition" ||
				   tmpName == "physicalcondition") {
			ConditionType_t conditionType = CONDITION_NONE;
			uint32_t tickInterval = 2000;

			if (tmpName == "firecondition") {
				conditionType = CONDITION_FIRE;
				tickInterval = 10000;
			} else if (tmpName == "poisoncondition" || tmpName == "earthcondition") {
				conditionType = CONDITION_POISON;
				tickInterval = 4000;
			} else if (tmpName == "energycondition") {
				conditionType = CONDITION_ENERGY;
				tickInterval = 10000;
			} else if (tmpName == "drowncondition") {
				conditionType = CONDITION_DROWN;
				tickInterval = 5000;
			} else if (tmpName == "freezecondition" || tmpName == "icecondition") {
				conditionType = CONDITION_FREEZING;
				tickInterval = 10000;
			} else if (tmpName == "cursecondition" || tmpName == "deathcondition") {
				conditionType = CONDITION_CURSED;
				tickInterval = 4000;
			} else if (tmpName == "dazzlecondition" || tmpName == "holycondition") {
				conditionType = CONDITION_DAZZLED;
				tickInterval = 10000;
			} else if (tmpName == "physicalcondition" || tmpName == "bleedcondition") {
				conditionType = CONDITION_BLEEDING;
				tickInterval = 4000;
			}

			if ((attr = node.attribute("tick"))) {
				int32_t value = pugi::cast<int32_t>(attr.value());
				if (value > 0) {
					tickInterval = value;
				}
			}

			int32_t minDamage = std::abs(sb.minCombatValue);
			int32_t maxDamage = std::abs(sb.maxCombatValue);
			int32_t startDamage = 0;

			if ((attr = node.attribute("start"))) {
				int32_t value = std::abs(pugi::cast<int32_t>(attr.value()));
				if (value <= minDamage) {
					startDamage = value;
				}
			}

			Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, startDamage, tickInterval);
			combat->addCondition(condition);
		} else if (tmpName == "strength") {
			//
		} else if (tmpName == "effect") {
			//
		} else {
			std::cout << "[Error - Pokemons::deserializeMove] - " << description << " - Unknown move name: " << name << std::endl;
			return false;
		}

		combat->setPlayerCombatValues(COMBAT_FORMULA_DAMAGE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
		combatMove = new CombatMove(combat, needTarget, needDirection);

		for (auto attributeNode : node.children()) {
			if ((attr = attributeNode.attribute("key"))) {
				const char* value = attr.value();
				if (strcasecmp(value, "shooteffect") == 0) {
					if ((attr = attributeNode.attribute("value"))) {
						ShootType_t shoot = getShootType(asLowerCaseString(attr.as_string()));
						if (shoot != CONST_ANI_NONE) {
							combat->setParam(COMBAT_PARAM_DISTANCEEFFECT, shoot);
						} else {
							std::cout << "[Warning - Pokemons::deserializeMove] " << description << " - Unknown shootEffect: " << attr.as_string() << std::endl;
						}
					}
				} else if (strcasecmp(value, "areaeffect") == 0) {
					if ((attr = attributeNode.attribute("value"))) {
						MagicEffectClasses effect = getMagicEffect(asLowerCaseString(attr.as_string()));
						if (effect != CONST_ME_NONE) {
							combat->setParam(COMBAT_PARAM_EFFECT, effect);
						} else {
							std::cout << "[Warning - Pokemons::deserializeMove] " << description << " - Unknown areaEffect: " << attr.as_string() << std::endl;
						}
					}
				} else {
					std::cout << "[Warning - Pokemons::deserializeMoves] Effect type \"" << attr.as_string() << "\" does not exist." << std::endl;
				}
			}
		}
	}

	sb.move = combatMove;
	if (combatMove) {
		sb.combatMove = true;
	}
	return true;
}

bool Pokemons::deserializeMove(PokemonMove* move, moveBlock_t& sb, const std::string& description)
{
	if (!move->scriptName.empty()) {
		move->isScripted = true;
	} else if (!move->name.empty()) {
		move->isScripted = false;
	} else {
		return false;
	}

	sb.speed = move->interval;

	if (move->chance > 100) {
		sb.chance = 100;
	} else {
		sb.chance = move->chance;
	}

	if (move->range > (Map::maxViewportX * 2)) {
		move->range = Map::maxViewportX * 2;
	}
	sb.range = move->range;

	sb.minCombatValue = move->minCombatValue;
	sb.maxCombatValue = move->maxCombatValue;
	if (std::abs(sb.minCombatValue) > std::abs(sb.maxCombatValue)) {
		int32_t value = sb.maxCombatValue;
		sb.maxCombatValue = sb.minCombatValue;
		sb.minCombatValue = value;
	}

	sb.move = g_moves->getMoveByName(move->name);
	if (sb.move) {
		return true;
	}

	CombatMove* combatMove = nullptr;

	if (move->isScripted) {
		std::unique_ptr<CombatMove> combatMovePtr(new CombatMove(nullptr, move->needTarget, move->needDirection));
		if (!combatMovePtr->loadScript("data/" + g_moves->getScriptBaseName() + "/scripts/" + move->scriptName)) {
			std::cout << "cannot find file" << std::endl;
			return false;
		}

		if (!combatMovePtr->loadScriptCombat()) {
			return false;
		}

		combatMove = combatMovePtr.release();
		combatMove->getCombat()->setPlayerCombatValues(COMBAT_FORMULA_DAMAGE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
	} else {
		Combat_ptr combat = std::make_shared<Combat>();
		sb.combatMove = true;

		if (move->length > 0) {
			move->spread = std::max<int32_t>(0, move->spread);

			AreaCombat* area = new AreaCombat();
			area->setupArea(move->length, move->spread);
			combat->setArea(area);

			move->needDirection = true;
		}

		if (move->radius > 0) {
			AreaCombat* area = new AreaCombat();
			area->setupArea(move->radius);
			combat->setArea(area);
		}

		std::string tmpName = asLowerCaseString(move->name);

		if (tmpName == "melee") {
			sb.isMelee = true;

			if (move->attack > 0 && move->skill > 0) {
				sb.minCombatValue = 0;
				sb.maxCombatValue = -Weapons::getMaxMeleeDamage(move->skill, move->attack);
			}

			if (move->conditionType != CONDITION_NONE) {
				ConditionType_t conditionType = move->conditionType;

				int32_t minDamage = move->conditionMinDamage;
				int32_t maxDamage = minDamage;

				uint32_t tickInterval = 2000;
				if (move->tickInterval != 0) {
					tickInterval = move->tickInterval;
				}

				Condition* condition = getDamageCondition(conditionType, maxDamage, minDamage, move->conditionStartDamage, tickInterval);
				combat->addCondition(condition);
			}

			sb.range = 1;
			combat->setParam(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE);
			combat->setParam(COMBAT_PARAM_BLOCKARMOR, 1);
			combat->setParam(COMBAT_PARAM_BLOCKSHIELD, 1);
			combat->setOrigin(ORIGIN_MELEE);
		} else if (tmpName == "combat") {
			if (move->combatType == COMBAT_UNDEFINEDDAMAGE) {
				std::cout << "[Warning - Pokemons::deserializeMove] - " << description << " - move has undefined damage" << std::endl;
				combat->setParam(COMBAT_PARAM_TYPE, COMBAT_PHYSICALDAMAGE);
			}

			if (move->combatType == COMBAT_PHYSICALDAMAGE) {
				combat->setParam(COMBAT_PARAM_BLOCKARMOR, 1);
				combat->setOrigin(ORIGIN_RANGED);
			} else if (move->combatType == COMBAT_HEALING) {
				combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
			}
			combat->setParam(COMBAT_PARAM_TYPE, move->combatType);
		} else if (tmpName == "speed") {
			int32_t minSpeedChange = 0;
			int32_t maxSpeedChange = 0;
			int32_t duration = 10000;

			if (move->duration != 0) {
				duration = move->duration;
			}

			if (move->minSpeedChange != 0) {
				minSpeedChange = move->minSpeedChange;
			} else {
				std::cout << "[Error - Pokemons::deserializeMove] - " << description << " - missing speedchange/minspeedchange value" << std::endl;
				delete move;
				return false;
			}

			if (minSpeedChange < -1000) {
				std::cout << "[Warning - Pokemons::deserializeMove] - " << description << " - you cannot reduce a creatures speed below -1000 (100%)" << std::endl;
				minSpeedChange = -1000;
			}

			if (move->maxSpeedChange != 0) {
				maxSpeedChange = move->maxSpeedChange;
			} else {
				maxSpeedChange = minSpeedChange; // static speedchange value
			}

			ConditionType_t conditionType;
			if (minSpeedChange >= 0) {
				conditionType = CONDITION_HASTE;
				combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
			} else {
				conditionType = CONDITION_PARALYZE;
			}

			ConditionSpeed* condition = static_cast<ConditionSpeed*>(Condition::createCondition(CONDITIONID_COMBAT, conditionType, duration, 0));
			condition->setFormulaVars(minSpeedChange / 1000.0, 0, maxSpeedChange / 1000.0, 0);
			combat->addCondition(condition);
		} else if (tmpName == "outfit") {
			int32_t duration = 10000;

			if (move->duration != 0) {
				duration = move->duration;
			}

			ConditionOutfit* condition = static_cast<ConditionOutfit*>(Condition::createCondition(CONDITIONID_COMBAT, CONDITION_OUTFIT, duration, 0));
			condition->setOutfit(move->outfit);
			combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
			combat->addCondition(condition);
		} else if (tmpName == "invisible") {
			int32_t duration = 10000;

			if (move->duration != 0) {
				duration = move->duration;
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_INVISIBLE, duration, 0);
			combat->setParam(COMBAT_PARAM_AGGRESSIVE, 0);
			combat->addCondition(condition);
		} else if (tmpName == "drunk") {
			int32_t duration = 10000;
			uint8_t drunkenness = 25;

			if (move->duration != 0) {
				duration = move->duration;
			}

			if (move->drunkenness != 0) {
				drunkenness = move->drunkenness;
			}

			Condition* condition = Condition::createCondition(CONDITIONID_COMBAT, CONDITION_DRUNK, duration, drunkenness);
			combat->addCondition(condition);
		} else if (tmpName == "firefield") {
			combat->setParam(COMBAT_PARAM_CREATEITEM, ITEM_FIREFIELD_PVP_FULL);
		} else if (tmpName == "poisonfield") {
			combat->setParam(COMBAT_PARAM_CREATEITEM, ITEM_POISONFIELD_PVP);
		} else if (tmpName == "energyfield") {
			combat->setParam(COMBAT_PARAM_CREATEITEM, ITEM_ENERGYFIELD_PVP);
		} else if (tmpName == "condition") {
			uint32_t tickInterval = 2000;

			if (move->conditionType == CONDITION_NONE) {
				std::cout << "[Error - Pokemons::deserializeMove] - " << description << " - Condition is not set for: " << move->name << std::endl;
			}

			if (move->tickInterval != 0) {
				int32_t value = move->tickInterval;
				if (value > 0) {
					tickInterval = value;
				}
			}

			int32_t minDamage = std::abs(move->conditionMinDamage);
			int32_t maxDamage = std::abs(move->conditionMaxDamage);
			int32_t startDamage = 0;

			if (move->conditionStartDamage != 0) {
				int32_t value = std::abs(move->conditionStartDamage);
				if (value <= minDamage) {
					startDamage = value;
				}
			}

			Condition* condition = getDamageCondition(move->conditionType, maxDamage, minDamage, startDamage, tickInterval);
			combat->addCondition(condition);
		} else if (tmpName == "strength") {
			//
		} else if (tmpName == "effect") {
			//
		} else {
			std::cout << "[Error - Pokemons::deserializeMove] - " << description << " - Unknown move name: " << move->name << std::endl;
		}

		if (move->needTarget) {
			if (move->shoot != CONST_ANI_NONE) {
				combat->setParam(COMBAT_PARAM_DISTANCEEFFECT, move->shoot);
			}
		}

		if (move->effect != CONST_ME_NONE) {
			combat->setParam(COMBAT_PARAM_EFFECT, move->effect);
		}

		combat->setPlayerCombatValues(COMBAT_FORMULA_DAMAGE, sb.minCombatValue, 0, sb.maxCombatValue, 0);
		combatMove = new CombatMove(combat, move->needTarget, move->needDirection);
	}

	sb.move = combatMove;
	if (combatMove) {
		sb.combatMove = true;
	}
	return true;
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

	if ((attr = pokemonNode.attribute("manacost"))) {
		mType->info.manaCost = pugi::cast<uint32_t>(attr.value());
	}

	if ((attr = pokemonNode.attribute("skull"))) {
		mType->info.skull = getSkullType(asLowerCaseString(attr.as_string()));
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
	if (mType->info.manaCost == 0 && (mType->info.isSummonable || mType->info.isConvinceable)) {
		std::cout << "[Warning - Pokemons::loadPokemon] manaCost missing or zero on pokemon with summonable and/or convinceable flags: " << file << std::endl;
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

	if ((node = pokemonNode.child("attacks"))) {
		for (auto attackNode : node.children()) {
			moveBlock_t sb;
			if (deserializeMove(attackNode, sb, pokemonName)) {
				mType->info.attackMoves.emplace_back(std::move(sb));
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Cant load move. " << file << std::endl;
			}
		}
	}

	if ((node = pokemonNode.child("defenses"))) {
		if ((attr = node.attribute("defense"))) {
			mType->info.defense = pugi::cast<int32_t>(attr.value());
		}

		if ((attr = node.attribute("armor"))) {
			mType->info.armor = pugi::cast<int32_t>(attr.value());
		}

		for (auto defenseNode : node.children()) {
			moveBlock_t sb;
			if (deserializeMove(defenseNode, sb, pokemonName)) {
				mType->info.defenseMoves.emplace_back(std::move(sb));
			} else {
				std::cout << "[Warning - Pokemons::loadPokemon] Cant load move. " << file << std::endl;
			}
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
				} else if (tmpStrValue == "manadrain") {
					mType->info.damageImmunities |= COMBAT_MANADRAIN;
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
			} else if ((attr = immunityNode.attribute("manadrain"))) {
				if (attr.as_bool()) {
					mType->info.damageImmunities |= COMBAT_MANADRAIN;
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
			} else if ((attr = elementNode.attribute("manadrainPercent"))) {
				mType->info.elementMap[COMBAT_MANADRAIN] = pugi::cast<int32_t>(attr.value());
				if (mType->info.damageImmunities & COMBAT_MANADRAIN) {
					std::cout << "[Warning - Pokemons::loadPokemon] Same element \"manadrain\" on immunity and element tags. " << file << std::endl;
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
	mType->info.attackMoves.shrink_to_fit();
	mType->info.defenseMoves.shrink_to_fit();
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

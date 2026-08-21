// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_POKEMONS_H_776E8327BCE2450EB7C4A260785E6C0D
#define FS_POKEMONS_H_776E8327BCE2450EB7C4A260785E6C0D

#include "creature.h"

const uint32_t MAX_LOOTCHANCE = 100000;

struct LootBlock {
	uint16_t id;
	uint32_t countmax;
	uint32_t chance;

	//optional
	int32_t subType;
	int32_t actionId;
	std::string text;

	std::vector<LootBlock> childLoot;
	LootBlock() {
		id = 0;
		countmax = 1;
		chance = 0;

		subType = -1;
		actionId = -1;
	}
};

class Loot {
	public:
		Loot() = default;

		// non-copyable
		Loot(const Loot&) = delete;
		Loot& operator=(const Loot&) = delete;

		LootBlock lootBlock;
};

struct summonBlock_t {
	std::string name;
	uint32_t chance;
	uint32_t speed;
	uint32_t max;
	bool force = false;
};

class BaseMove;
struct PokemonMoveType {
	uint16_t id = 0;
	std::string key;
	std::string name;
	std::string effect;
	PokemonTypes_t type = TYPE_NONE;
	PokemonMoveCategory_t category = POKEMON_MOVE_CATEGORY_STATUS;
	uint16_t power = 0;
	uint16_t pp = 0;
	uint8_t accuracy = 100;
	uint8_t range = 1;
	uint32_t cooldown = 2000;
};

struct PokemonLearnMove {
	uint16_t moveId = 0;
	uint8_t level = 1;
};

struct PokemonMoveState {
	uint16_t moveId = 0;
	uint8_t activeSlot = 0;
};

struct moveBlock_t {
	constexpr moveBlock_t() = default;
	~moveBlock_t();
	moveBlock_t(const moveBlock_t& other) = delete;
	moveBlock_t& operator=(const moveBlock_t& other) = delete;
	moveBlock_t(moveBlock_t&& other) :
		move(other.move),
		chance(other.chance),
		speed(other.speed),
		range(other.range),
		minCombatValue(other.minCombatValue),
		maxCombatValue(other.maxCombatValue),
		combatMove(other.combatMove),
		isMelee(other.isMelee) {
		other.move = nullptr;
	}

	BaseMove* move = nullptr;
	uint32_t chance = 100;
	uint32_t speed = 2000;
	uint32_t range = 0;
	int32_t minCombatValue = 0;
	int32_t maxCombatValue = 0;
	bool combatMove = false;
	bool isMelee = false;
};

struct voiceBlock_t {
	std::string text;
	bool yellText;
};

class PokemonType
{
	struct PokemonInfo {
		LuaScriptInterface* scriptInterface;

		std::map<CombatType_t, int32_t> elementMap;

		std::vector<voiceBlock_t> voiceVector;

		std::vector<LootBlock> lootItems;
		std::vector<std::string> scripts;
		std::vector<moveBlock_t> attackMoves;
		std::vector<moveBlock_t> defenseMoves;
		std::vector<summonBlock_t> summons;

		Skulls_t skull = SKULL_NONE;
		Outfit_t outfit = {};
		RaceType_t race = RACE_BLOOD;

		LightInfo light = {};
		uint16_t lookcorpse = 0;

		uint64_t experience = 0;

		uint32_t manaCost = 0;
		uint32_t yellChance = 0;
		uint32_t yellSpeedTicks = 0;
		uint32_t staticAttackChance = 95;
		uint32_t maxSummons = 0;
		uint32_t changeTargetSpeed = 0;
		uint32_t conditionImmunities = 0;
		uint32_t damageImmunities = 0;
		uint32_t baseSpeed = 200;

		int32_t creatureAppearEvent = -1;
		int32_t creatureDisappearEvent = -1;
		int32_t creatureMoveEvent = -1;
		int32_t creatureSayEvent = -1;
		int32_t thinkEvent = -1;
		int32_t targetDistance = 1;
		int32_t runAwayHealth = 0;
		int32_t health = 100;
		int32_t healthMax = 100;
		int32_t changeTargetChance = 0;
		int32_t defense = 0;
		int32_t armor = 0;

		bool canPushItems = false;
		bool canPushCreatures = false;
		bool pushable = true;
		bool isAttackable = true;
		bool isBoss = false;
		bool isChallengeable = true;
		bool isConvinceable = false;
		bool isHostile = true;
		bool isIgnoringSpawnBlock = false;
		bool isIllusionable = false;
		bool isSummonable = false;
		bool hiddenHealth = false;
		bool canWalkOnEnergy = true;
		bool canWalkOnFire = true;
		bool canWalkOnPoison = true;

		PokemonsEvent_t eventType = POKEMONS_EVENT_NONE;

		PokemonStats_t ev_yield = {};
		PokemonStats_t base_stats = {};
		std::vector<PokemonLearnMove> learnset;

		uint16_t number {0};
		std::array<PokemonTypes_t, 2> types = { TYPE_NONE, TYPE_NONE };
		uint8_t catch_rate {0};
		LevelRate_t level_rate {RATE_NONE};
		uint8_t base_experience {0};
		float height {0};
		float weight {0};

		struct gender_ratio {
			float male = 0.0;
			float female = 0.0;
		} gender_ratio;

		std::vector<EggGroups_t> egg_groups;
		uint8_t egg_cycles {0};
		uint8_t base_friendship {0};

		struct evolution {
			EvolveTypes_t type = EVOLVE_NONE;
			union {
				uint8_t level;
				uint32_t itemId;
			};
		}evolution;
	};

	public:
		PokemonType() = default;

		// non-copyable
		PokemonType(const PokemonType&) = delete;
		PokemonType& operator=(const PokemonType&) = delete;

		bool loadCallback(LuaScriptInterface* scriptInterface);

		std::string name;
		std::string nameDescription;

		PokemonInfo info;

		void loadLoot(PokemonType* pokemonType, LootBlock lootBlock);

		uint8_t getCatchRate() { return info.catch_rate; }
};

class PokemonMove
{
	public:
		PokemonMove() = default;

		PokemonMove(const PokemonMove&) = delete;
		PokemonMove& operator=(const PokemonMove&) = delete;

		std::string name = "";
		std::string scriptName = "";

		uint8_t chance = 100;
		uint8_t range = 0;
		uint8_t drunkenness = 0;

		uint16_t interval = 2000;

		int32_t minCombatValue = 0;
		int32_t maxCombatValue = 0;
		int32_t attack = 0;
		int32_t skill = 0;
		int32_t length = 0;
		int32_t spread = 0;
		int32_t radius = 0;
		int32_t conditionMinDamage = 0;
		int32_t conditionMaxDamage = 0;
		int32_t conditionStartDamage = 0;
		int32_t tickInterval = 0;
		int32_t minSpeedChange = 0;
		int32_t maxSpeedChange = 0;
		int32_t duration = 0;

		bool isScripted = false;
		bool needTarget = false;
		bool needDirection = false;
		bool combatMove = false;
		bool isMelee = false;

		Outfit_t outfit = {};
		ShootType_t shoot = CONST_ANI_NONE;
		MagicEffectClasses effect = CONST_ME_NONE;
		ConditionType_t conditionType = CONDITION_NONE;
		CombatType_t combatType = COMBAT_UNDEFINEDDAMAGE;
};

class Pokemons
{
	public:
		Pokemons() = default;
		// non-copyable
		Pokemons(const Pokemons&) = delete;
		Pokemons& operator=(const Pokemons&) = delete;

		bool loadFromXml(bool reloading = false);
		bool isLoaded() const {
			return loaded;
		}
		bool reload();

		PokemonType* getPokemonType(const std::string& name, bool loadFromFile = true);
		const PokemonMoveType* getMoveById(uint16_t id) const;
		const PokemonMoveType* getMoveByName(const std::string& name) const;
		bool addLearnMove(PokemonType* pokemonType, const std::string& moveName, uint8_t level);
		bool deserializeMove(PokemonMove* move, moveBlock_t& sb, const std::string& description = "");

		std::unique_ptr<LuaScriptInterface> scriptInterface;
		std::map<std::string, PokemonType> pokemons;

	private:
		bool loadMoves();
		ConditionDamage* getDamageCondition(ConditionType_t conditionType,
		                                    int32_t maxDamage, int32_t minDamage, int32_t startDamage, uint32_t tickInterval);
		bool deserializeMove(const pugi::xml_node& node, moveBlock_t& sb, const std::string& description = "");

		PokemonType* loadPokemon(const std::string& file, const std::string& pokemonName, bool reloading = false);

		void loadLootContainer(const pugi::xml_node& node, LootBlock&);
		bool loadLootItem(const pugi::xml_node& node, LootBlock&);

		std::map<std::string, std::string> unloadedPokemons;
		std::map<uint16_t, PokemonMoveType> moves;
		std::map<std::string, uint16_t> moveNames;

		bool loaded = false;
};

struct PokemonInfo_t
{
	uint32_t p_id;
	uint32_t p_uid;
	std::string name;
	uint16_t number;
	int32_t health;
	int32_t maxHealth;
	bool fainted;

	uint8_t level;
	uint64_t experience;
	PokemonGenders_t gender;
	PokemonNatures_t nature;
	uint8_t friendship;
	bool shiny;

	PokemonStats_t stats;
	PokemonStats_t ivs;
	PokemonStats_t evs;
	std::vector<PokemonMoveState> moves;

	PokemonInfo_t() :
		p_id(0),
		p_uid(0),
		number(0),
		health(0),
		maxHealth(1),
		fainted(false),
		level(0),
		experience(0),
		gender(GENDER_UNDEFINED),
		nature(NATURE_NONE),
		friendship(0),
		shiny(false)
	{
	}
};

std::vector<uint16_t> learnPokemonMoves(PokemonInfo_t& info, const PokemonType& pokemonType);
double getPokemonTypeEffectiveness(PokemonTypes_t attackingType, PokemonTypes_t defendingType);

struct PokemonNatureModifiers_t
{
	// Percentage adjustments. HP is not affected by nature.
	int8_t attack = 0;
	int8_t defense = 0;
	int8_t sp_attack = 0;
	int8_t sp_defense = 0;
	int8_t speed = 0;
};

PokemonNatureModifiers_t getPokemonNatureModifiers(PokemonNatures_t nature);
PokemonStats_t calculatePokemonStats(const PokemonStats_t& baseStats, uint8_t level,
	const PokemonStats_t& ivs, const PokemonStats_t& evs, PokemonNatures_t nature);

struct PokemonStatOptions_t
{
	int16_t hp = -1;
	int16_t attack = -1;
	int16_t defense = -1;
	int16_t sp_attack = -1;
	int16_t sp_defense = -1;
	int16_t speed = -1;

	bool hasAny() const
	{
		return hp >= 0 || attack >= 0 || defense >= 0 || sp_attack >= 0 || sp_defense >= 0 || speed >= 0;
	}
};

struct PokemonCreateOptions_t
{
	int16_t level = -1;
	int16_t friendship = -1;
	int8_t shiny = -1;
	int8_t gender = -1;
	int8_t nature = -1;
	PokemonStatOptions_t ivs;
	PokemonStatOptions_t evs;
};

#endif

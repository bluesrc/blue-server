// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_POKEMON_H_9F5EEFE64314418CA7DA41D1B9409DD0
#define FS_POKEMON_H_9F5EEFE64314418CA7DA41D1B9409DD0

#include "tile.h"
#include "pokemons.h"

#include <array>
#include <unordered_set>
#include <variant>

class Creature;
class Game;
class Spawn;

using CreatureHashSet = std::unordered_set<Creature*>;
using CreatureList = std::list<Creature*>;

enum TargetSearchType_t {
	TARGETSEARCH_DEFAULT,
	TARGETSEARCH_RANDOM,
	TARGETSEARCH_ATTACKRANGE,
	TARGETSEARCH_NEAREST,
};

enum PokemonBattleStat_t : uint8_t {
	POKEMON_BATTLE_STAT_ATTACK,
	POKEMON_BATTLE_STAT_DEFENSE,
	POKEMON_BATTLE_STAT_SPECIAL_ATTACK,
	POKEMON_BATTLE_STAT_SPECIAL_DEFENSE,
	POKEMON_BATTLE_STAT_SPEED,
	POKEMON_BATTLE_STAT_ACCURACY,
	POKEMON_BATTLE_STAT_COUNT,
};

struct PokemonBattleModifier {
	PokemonBattleStat_t stat;
	int8_t amount;
	int64_t expiresAt;
};

using PokemonAbilityStateValue = std::variant<bool, double, std::string>;
using PokemonHeldItemStateValue = PokemonAbilityStateValue;

struct PokemonHeldItemBattleState {
	uint16_t temporaryItemId = 0;
	uint16_t consumedItemId = 0;
	uint16_t lockedMoveId = 0;
	uint32_t suppressionReasons = HELD_ITEM_SUPPRESSION_NONE;
	bool hasTemporaryItem = false;
	bool consumedTemporaryItem = false;
};

class Pokemon final : public Creature
{
	public:
		static Pokemon* createPokemon(const std::string& name);
		static Pokemon* createPlayerPokemon(PokemonInfo_t pInfo);
		static int32_t despawnRange;
		static int32_t despawnRadius;

		explicit Pokemon(PokemonType* mType);
		explicit Pokemon(PokemonType* mType, PokemonInfo_t pInfo);
		~Pokemon();

		// non-copyable
		Pokemon(const Pokemon&) = delete;
		Pokemon& operator=(const Pokemon&) = delete;

		Pokemon* getPokemon() override {
			return this;
		}
		const Pokemon* getPokemon() const override {
			return this;
		}

		void setID() override {
			if (id == 0) {
				id = pokemonAutoID++;
			}
		}

		void addList() override;
		void removeList() override;

		const std::string& getName() const override;
		void setName(const std::string& name);

		const std::string& getNameDescription() const override;
		void setNameDescription(const std::string& nameDescription) {
			this->nameDescription = nameDescription;
		};

		std::string getDescription(int32_t) const override {
			return nameDescription + '.';
		}

		CreatureType_t getType() const override {
			return CREATURETYPE_POKEMON;
		}

		const Position& getMasterPos() const {
			return masterPos;
		}
		void setMasterPos(Position pos) {
			masterPos = pos;
		}

		RaceType_t getRace() const override {
			return mType->info.race;
		}
		int32_t getArmor() const override {
			return mType->info.armor;
		}
		int32_t getDefense() const override {
			return mType->info.defense;
		}
		bool isPushable() const override {
			return mType->info.pushable && baseSpeed != 0;
		}
		bool isAttackable() const override {
			return mType->info.isAttackable;
		}

		bool canPushItems() const;
		bool canPushCreatures() const {
			return mType->info.canPushCreatures;
		}
		bool isHostile() const {
			return mType->info.isHostile;
		}
		bool canSee(const Position& pos) const override;
		bool canSeeInvisibility() const override {
			return isImmune(CONDITION_INVISIBLE);
		}
		uint32_t getManaCost() const {
			return mType->info.manaCost;
		}
		void setSpawn(Spawn* spawn) {
			this->spawn = spawn;
		}
		bool canWalkOnFieldType(CombatType_t combatType) const;

		void onAttackedCreatureDisappear(bool isLogout) override;

		void onCreatureAppear(Creature* creature, bool isLogin) override;
		void onRemoveCreature(Creature* creature, bool isLogout) override;
		void onCreatureMove(Creature* creature, const Tile* newTile, const Position& newPos, const Tile* oldTile, const Position& oldPos, bool teleport) override;
		void onCreatureSay(Creature* creature, SpeakClasses type, const std::string& text) override;
		void onPlacedCreature() override;

		void drainHealth(Creature* attacker, int32_t damage) override;
		void changeHealth(int32_t healthChange, bool sendHealthChange = true) override;
		void onAttackedCreatureDrainHealth(Creature* target, int32_t points) override;

		bool isWalkingToSpawn() const {
			return walkingToSpawn;
		}
		bool walkToSpawn();
		void onWalk() override;
		void onWalkComplete() override;
		bool getNextStep(Direction& direction, uint32_t& flags) override;
		void onFollowCreatureComplete(const Creature* creature) override;

		void onThink(uint32_t interval) override;

		bool challengeCreature(Creature* creature, bool force = false) override;
		uint64_t getGainedExperience(Creature* attacker) const override;
		void onGainExperience(uint64_t gainExp, Creature* target) override;

		void setNormalCreatureLight() override;
		bool getCombatValues(int32_t& min, int32_t& max) override;

		void doAttacking(uint32_t interval) override;
		bool hasExtraSwing() override {
			return lastMeleeAttack == 0;
		}

		bool searchTarget(TargetSearchType_t searchType = TARGETSEARCH_DEFAULT);
		bool selectTarget(Creature* creature);

		const CreatureList& getTargetList() const {
			return targetList;
		}
		const CreatureHashSet& getFriendList() const {
			return friendList;
		}

		bool isTarget(const Creature* creature) const;
		bool isFleeing() const {
			return !isSummon() && getHealth() <= mType->info.runAwayHealth && challengeFocusDuration <= 0;
		}

		bool getDistanceStep(const Position& targetPos, Direction& direction, bool flee = false);
		bool isTargetNearby() const {
			return stepDuration >= 1;
		}
		bool isIgnoringFieldDamage() const {
			return ignoreFieldDamage;
		}

		BlockType_t blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
		                     bool checkDefense = false, bool checkArmor = false, bool field = false, bool ignoreResistances = false) override;

		static uint32_t pokemonAutoID;

		uint8_t getLevel() const { return level; }
		uint64_t getExperience() const { return experience; }
		bool setLevel(uint8_t level, bool fullHealth = true);
		uint8_t addExperience(uint64_t experience, bool sendText = false);
		bool addLevel(bool sendText = false);
		bool canEvolve(EvolveTypes_t trigger, uint32_t requirement = 0) const;
		bool evolve(EvolveTypes_t trigger, uint32_t requirement = 0);
		void applyCreateOptions(const PokemonCreateOptions_t& options, bool fullHealth = true);
		static uint64_t getExperienceForLevel(LevelRate_t rate, uint8_t level);
		uint8_t getGender() const { return gender; }
		bool isShiny() const { return shiny; }
		uint16_t getAbilityId() const { return abilityId; }
		uint8_t getAbilitySlot() const { return abilitySlot; }
		uint16_t getHeldItemId() const { return heldItemId; }
		uint32_t getEvolutionSeed() const { return evolutionSeed; }
		const std::string& getPendingEvolution() const { return pendingEvolution; }
		std::string getLevelEvolutionTarget() const;
		uint16_t getEffectiveHeldItemId() const;
		uint16_t getConsumedHeldItemId() const { return heldItemBattleState.consumedItemId; }
		uint16_t getHeldItemLockedMoveId() const { return heldItemBattleState.lockedMoveId; }
		uint32_t getHeldItemSuppressionReasons() const { return heldItemBattleState.suppressionReasons; }
		bool isHeldItemEffectActive() const;
		bool isHeldItemSuppressed(uint32_t reason = HELD_ITEM_SUPPRESSION_NONE) const;
		void setHeldItemId(uint16_t itemId);
		bool consumeHeldItem();
		int32_t healFromHeldItem(uint32_t numerator, uint32_t denominator = 100);
		bool restoreConsumedHeldItem();
		bool suppressHeldItem(uint32_t reason);
		bool unsuppressHeldItem(uint32_t reason);
		bool setTemporaryHeldItemId(uint16_t itemId);
		bool clearTemporaryHeldItem();
		bool exchangeHeldItemsForBattle(Pokemon* other);
		bool stealHeldItemForBattle(Pokemon* other);
		bool setHeldItemLockedMoveId(uint16_t moveId);
		const PokemonHeldItemStateValue* getHeldItemState(const std::string& key) const;
		void setHeldItemState(std::string key, PokemonHeldItemStateValue value);
		bool clearHeldItemState(const std::string& key);
		void clearHeldItemState();
		const PokemonAbilityType* getAbility() const;
		PokemonStatusCondition_t getPokemonStatusCondition() const { return pokemonStatus; }
		const PokemonAbilityStateValue* getAbilityState(const std::string& key) const;
		void setAbilityState(std::string key, PokemonAbilityStateValue value);
		bool clearAbilityState(const std::string& key);
		void clearAbilityState();
		void refreshAbilityStats(bool preserveHealth = false, bool notify = true);
		void leaveAbilityCombat();
		bool isInPokemonCombat() const { return abilityCombatActive; }
		PokemonStats_t getIvs() { return ivs; }
		PokemonStats_t getEvs() { return evs; }
		PokemonNatures_t getNature() const { return nature; }
		uint8_t getFriendship() const { return friendship; }
		uint8_t addFriendship(int32_t amount);
		uint32_t getCombatFriendshipTime() const { return combatFriendshipTime; }
		uint16_t getNumber() { return mType->info.number; }
		const PokemonStats_t& getPokemonStats() const { return stats; }
		PokemonStats_t getEffectivePokemonStats() const;
		const PokemonType* getPokemonTypeData() const { return mType; }
		const std::vector<PokemonMoveState>& getMoves() const { return knownMoves; }
		bool learnMove(uint16_t moveId);
		bool refreshAvailableMoves();
		bool setMoveSlot(uint16_t moveId, uint8_t slot);
		bool useMove(uint8_t slot, Creature* target);
		bool modifyBattleStatStage(PokemonBattleStat_t stat, int8_t amount, uint32_t duration = 10000);
		bool applyStatusCondition(PokemonStatusCondition_t status, uint32_t duration, Creature* source = nullptr);
		bool cureStatusCondition();
		bool applyFlinch(uint32_t duration = 1500);
		int32_t getExecutingMoveDamage(Creature* target);
		bool rollExecutingMoveHit(const Creature* target) const;
		bool isExecutingPokemonMove() const { return executingPokemonMove; }
		const PokemonMoveType* getExecutingMove() const { return executingMove; }

	private:
		CreatureHashSet friendList;
		CreatureList targetList;

		std::string name;
		std::string nameDescription;

		PokemonType* mType;
		uint16_t heldItemId = 0;
		Spawn* spawn = nullptr;

		int64_t lastMeleeAttack = 0;

		uint32_t attackTicks = 0;
		uint32_t targetTicks = 0;
		uint32_t targetChangeTicks = 0;
		uint32_t defenseTicks = 0;
		uint32_t yellTicks = 0;
		int32_t minCombatValue = 0;
		int32_t maxCombatValue = 0;
		int32_t targetChangeCooldown = 0;
		int32_t challengeFocusDuration = 0;
		int32_t stepDuration = 0;

		Position masterPos;

		bool ignoreFieldDamage = false;
		bool isIdle = true;
		bool isMasterInRange = false;
		bool randomStepping = false;
		bool walkingToSpawn = false;

		PokemonStats_t stats = {};
		PokemonStats_t abilityStats = {};
		PokemonStats_t ivs = {};
		PokemonStats_t evs = {};
		PokemonGenders_t gender = GENDER_NONE;
		PokemonNatures_t nature = NATURE_NONE;
		uint8_t friendship {0};
		uint32_t combatFriendshipTime {0};
		int64_t lastCombatActivity {0};
		bool abilityCombatActive = false;
		uint8_t evasion {100};
		uint8_t accuracy {100};

		uint8_t level = 1;
		uint64_t experience = 0;
		bool shiny = false;
		uint16_t abilityId = 0;
		uint8_t abilitySlot = 0;
		uint32_t evolutionSeed = 0;
		std::string pendingEvolution;
		bool executingPokemonMove = false;
		bool processingPokemonMoveUse = false;
		bool calculatingAbilityStats = false;
		bool processingFriendshipChange = false;
		bool abilitySpawnProcessed = false;
		const PokemonMoveType* executingMove = nullptr;
		PokemonStatusCondition_t pokemonStatus = POKEMON_STATUS_NONE;
		int64_t pokemonStatusExpiresAt = 0;
		int64_t pokemonStatusNextTick = 0;
		int64_t flinchUntil = 0;
		uint32_t pokemonStatusSourceId = 0;

		std::vector<PokemonMoveState> knownMoves;
		std::unordered_map<uint16_t, int64_t> moveCooldowns;
		std::unordered_map<std::string, PokemonAbilityStateValue> abilityState;
		std::unordered_map<std::string, PokemonHeldItemStateValue> heldItemState;
		PokemonHeldItemBattleState heldItemBattleState;
		uint32_t heldItemCombatPulseElapsed = 0;
		std::unordered_set<uint32_t> abilityCombatOpponentIds;
		std::unordered_set<uint32_t> encounteredPokemonIds;
		std::array<int8_t, POKEMON_BATTLE_STAT_COUNT> battleStatStages = {};
		std::vector<PokemonBattleModifier> battleModifiers;

		void updateStats(bool preserveHealth = false);
		void syncPokeball();
		void resetHeldItemBattleState();
		void refreshHeldItemTransition();
		uint8_t changeFriendship(int32_t amount);
		void markCombatActivity(Creature* opponent);
		bool canEscapeCombat(const PokemonMoveType& move);
		void completeCombatEscape();
		void processAbilityCombatState();
		void processHeldItemCombatPulse(uint32_t interval);
		void processCombatFriendship(uint32_t interval);
		void gainEVs(const PokemonStats_t& gainedEVs);
		bool meetsEvolutionConditions(const PokemonEvolutionConditions& conditions) const;
		bool hasPartySpecies(const std::string& species) const;
		const PokemonEvolution* getEligibleEvolution(EvolveTypes_t trigger, uint32_t requirement = 0) const;
		const PokemonEvolution* findPendingEvolutionRule() const;
		void notifyLevelEvolutionAvailable();
		void processEncounter(Creature* creature);
		void learnAvailableMoves(bool notify = false);
		int32_t calculateMoveDamage(const PokemonMoveType& move, const Creature* target) const;
		double getBattleStatMultiplier(PokemonBattleStat_t stat) const;
		bool rollMoveHit(const PokemonMoveType& move, const Creature* target) const;
		void processPokemonBattleState();
		void expireBattleModifiers(int64_t now);
		void clearPokemonStatus();
		void refreshBattleSpeed(bool notify = true);
		void notifyBattleStateChanged();
		bool canPerformMove();

		void onCreatureEnter(Creature* creature);
		void onCreatureLeave(Creature* creature);
		void onCreatureFound(Creature* creature, bool pushFront = false);

		void updateLookDirection();

		void addFriend(Creature* creature);
		void removeFriend(Creature* creature);
		void addTarget(Creature* creature, bool pushFront = false);
		void removeTarget(Creature* creature);

		void updateTargetList();
		void clearTargetList();
		void clearFriendList();

		void death(Creature* lastHitCreature) override;
		Item* getCorpse(Creature* lastHitCreature, Creature* mostDamageCreature) override;

		void setIdle(bool idle);
		void updateIdleStatus();
		bool getIdleStatus() const {
			return isIdle;
		}

		void onAddCondition(ConditionType_t type) override;
		void onEndCondition(ConditionType_t type) override;

		bool canUseAttack(const Position& pos, const Creature* target) const;
		bool canUseMove(const Position& pos, const Position& targetPos,
		                 const moveBlock_t& sb, uint32_t interval, bool& inRange, bool& resetTicks);
		bool getRandomStep(const Position& creaturePos, Direction& direction) const;
		bool getDanceStep(const Position& creaturePos, Direction& direction,
		                  bool keepAttack = true, bool keepDistance = true);
		bool isInSpawnRange(const Position& pos) const;
		bool canWalkTo(Position pos, Direction direction) const;

		static bool pushItem(Item* item);
		static void pushItems(Tile* tile);
		static bool pushCreature(Creature* creature);
		static void pushCreatures(Tile* tile);

		void onThinkTarget(uint32_t interval);
		void onThinkYell(uint32_t interval);
		void onThinkDefense(uint32_t interval);

		bool isFriend(const Creature* creature) const;
		bool isOpponent(const Creature* creature) const;

		uint64_t getLostExperience() const override {
			return skillLoss ? mType->info.experience : 0;
		}
		uint16_t getLookCorpse() const override {
			return mType->info.lookcorpse;
		}
		void dropLoot(Container* corpse, Creature* lastHitCreature) override;
		uint32_t getDamageImmunities() const override {
			return mType->info.damageImmunities;
		}
		uint32_t getConditionImmunities() const override {
			return mType->info.conditionImmunities;
		}
		void getPathSearchParams(const Creature* creature, FindPathParams& fpp) const override;
		bool useCacheMap() const override {
			return !randomStepping;
		}

		friend class LuaScriptInterface;
};

#endif

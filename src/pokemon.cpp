// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "pokemon.h"
#include "game.h"
#include "moves.h"
#include "events.h"
#include "configmanager.h"
#include "pokeball.h"

#include <numeric>

extern Game g_game;
extern Moves* g_moves;
extern Pokemons g_pokemons;
extern Events* g_events;
extern ConfigManager g_config;

int32_t Pokemon::despawnRange;
int32_t Pokemon::despawnRadius;

uint32_t Pokemon::pokemonAutoID = 0x40000000;

namespace {
	constexpr uint32_t POKEMON_STATUS_DAMAGE_TICK = 3000;
	constexpr uint32_t POKEMON_STATUS_VISUAL_TICK = 2000;
	constexpr uint32_t POKEMON_COMBAT_FRIENDSHIP_INTERVAL = 5 * 60 * 1000;
	constexpr int64_t POKEMON_COMBAT_ACTIVITY_TIMEOUT = 10 * 1000;

	MagicEffectClasses getPokemonStatusEffect(PokemonStatusCondition_t status)
	{
		switch (status) {
			case POKEMON_STATUS_BURN: return CONST_ME_FIREAREA;
			case POKEMON_STATUS_FREEZE: return CONST_ME_ICEATTACK; // Effect 44
			case POKEMON_STATUS_PARALYSIS: return CONST_ME_HOLYDAMAGE; // Effect 40
			case POKEMON_STATUS_POISON: return CONST_ME_POISONAREA;
			case POKEMON_STATUS_SLEEP: return CONST_ME_SLEEP; // Effect 33
			case POKEMON_STATUS_CONFUSION: return CONST_ME_STUN; // Effect 32
			case POKEMON_STATUS_NONE: return CONST_ME_NONE;
		}
		return CONST_ME_NONE;
	}
}

Pokemon* Pokemon::createPokemon(const std::string& name)
{
	PokemonType* mType = g_pokemons.getPokemonType(name);
	if (!mType) {
		return nullptr;
	}
	return new Pokemon(mType);
}

Pokemon* Pokemon::createPlayerPokemon(PokemonInfo_t pInfo)
{
	PokemonType* mType = g_pokemons.getPokemonType(pInfo.name);
	if (!mType) {
		return nullptr;
	}
	return new Pokemon(mType, pInfo);
}

const PokemonAbilityType* Pokemon::getAbility() const
{
	return g_pokemons.getAbilityById(abilityId);
}

const PokemonAbilityStateValue* Pokemon::getAbilityState(const std::string& key) const
{
	auto it = abilityState.find(key);
	return it != abilityState.end() ? &it->second : nullptr;
}

void Pokemon::setAbilityState(std::string key, PokemonAbilityStateValue value)
{
	abilityState.insert_or_assign(std::move(key), std::move(value));
	refreshAbilityStats();
}

bool Pokemon::clearAbilityState(const std::string& key)
{
	const bool removed = abilityState.erase(key) != 0;
	if (removed) {
		refreshAbilityStats();
	}
	return removed;
}

void Pokemon::clearAbilityState()
{
	if (abilityState.empty()) {
		return;
	}
	abilityState.clear();
	refreshAbilityStats();
}

Pokemon::Pokemon(PokemonType* mType) :
	Creature(),
	nameDescription(mType->nameDescription),
	mType(mType)
{
	defaultOutfit = mType->info.outfit;
	currentOutfit = mType->info.outfit;
	skull = mType->info.skull;
	baseSpeed = mType->info.baseSpeed;
	internalLight = mType->info.light;
	hiddenHealth = mType->info.hiddenHealth;

	std::mt19937 generator(static_cast<unsigned int>(std::time(0)));
	std::uniform_int_distribution<int> distribution(1, 31);
	ivs.hp = distribution(generator);
	ivs.attack = distribution(generator);
	ivs.defense = distribution(generator);
	ivs.sp_attack = distribution(generator);
	ivs.sp_defense = distribution(generator);
	ivs.speed = distribution(generator);
	nature = static_cast<PokemonNatures_t>(uniform_random(NATURE_HARDY, NATURE_QUIRKY));
	friendship = mType->info.base_friendship;
	abilityId = g_pokemons.selectAbility(*mType);

	experience = getExperienceForLevel(mType->info.level_rate, level);
	learnAvailableMoves();
	updateStats();
	health = healthMax;

	if (mType->info.gender_ratio.male == 0.0 && mType->info.gender_ratio.female == 0.0) {
		gender = GENDER_UNDEFINED;
	} else {
		std::vector<double> weights = { mType->info.gender_ratio.male, mType->info.gender_ratio.female };
		std::mt19937 generator(static_cast<unsigned int>(std::time(0)));
		std::discrete_distribution<> distribution(weights.begin(), weights.end());
		int selection = distribution(generator);

		if (selection == 0) {
			gender = GENDER_MALE;
		}
		else {
			gender = GENDER_FEMALE;
		}
	}

	// register creature events
	for (const std::string& scriptName : mType->info.scripts) {
		if (!registerCreatureEvent(scriptName)) {
			std::cout << "[Warning - Pokemon::Pokemon] Unknown event name: " << scriptName << std::endl;
		}
	}
}

Pokemon::Pokemon(PokemonType* mType, PokemonInfo_t pInfo) :
	Creature(),
	nameDescription(mType->nameDescription),
	mType(mType)
{
	defaultOutfit = mType->info.outfit;
	currentOutfit = mType->info.outfit;
	skull = mType->info.skull;
	internalLight = mType->info.light;
	hiddenHealth = mType->info.hiddenHealth;
	level = std::clamp<uint8_t>(pInfo.level, 1, 100);
	experience = std::max(pInfo.experience, getExperienceForLevel(mType->info.level_rate, level));
	const uint64_t maxExperience = getExperienceForLevel(mType->info.level_rate, 100);
	experience = std::min(experience, maxExperience);
	while (level < 100 && experience >= getExperienceForLevel(mType->info.level_rate, level + 1)) {
		++level;
	}

	ivs.hp = pInfo.ivs.hp;
	ivs.attack = pInfo.ivs.attack;
	ivs.defense = pInfo.ivs.defense;
	ivs.sp_attack = pInfo.ivs.sp_attack;
	ivs.sp_defense = pInfo.ivs.sp_defense;
	ivs.speed = pInfo.ivs.speed;

	evs.hp = pInfo.evs.hp;
	evs.attack = pInfo.evs.attack;
	evs.defense = pInfo.evs.defense;
	evs.sp_attack = pInfo.evs.sp_attack;
	evs.sp_defense = pInfo.evs.sp_defense;
	evs.speed = pInfo.evs.speed;
	nature = pInfo.nature;
	knownMoves = std::move(pInfo.moves);
	learnAvailableMoves();
	friendship = pInfo.friendship;
	combatFriendshipTime = std::min<uint32_t>(pInfo.combatFriendshipTime, POKEMON_COMBAT_FRIENDSHIP_INTERVAL - 1);
	shiny = pInfo.shiny;
	gender = pInfo.gender;
	abilityId = g_pokemons.isAbilityAvailable(*mType, pInfo.abilityId) ?
		pInfo.abilityId : g_pokemons.selectAbility(*mType);

	updateStats();
	health = std::clamp(pInfo.health, 0, healthMax);
	refreshAbilityStats(false, false);

	// register creature events
	for (const std::string& scriptName : mType->info.scripts) {
		if (!registerCreatureEvent(scriptName)) {
			std::cout << "[Warning - Pokemon::Pokemon] Unknown event name: " << scriptName << std::endl;
		}
	}
}

Pokemon::~Pokemon()
{
	clearTargetList();
	clearFriendList();
}

uint64_t Pokemon::getExperienceForLevel(LevelRate_t rate, uint8_t requestedLevel)
{
	const uint64_t n = std::clamp<uint8_t>(requestedLevel, 1, 100);
	const uint64_t n2 = n * n;
	const uint64_t n3 = n2 * n;

	switch (rate) {
		case RATE_ERRATIC:
			if (n <= 50) {
				return n3 * (100 - n) / 50;
			}
			if (n <= 68) {
				return n3 * (150 - n) / 100;
			}
			if (n <= 98) {
				return n3 * ((1911 - (10 * n)) / 3) / 500;
			}
			return n3 * (160 - n) / 100;
		case RATE_FAST:
			return 4 * n3 / 5;
		case RATE_MEDIUM_SLOW: {
			if (n == 1) {
				return 0;
			}
			const int64_t value = static_cast<int64_t>(6 * n3 / 5) - static_cast<int64_t>(15 * n2) + static_cast<int64_t>(100 * n) - 140;
			return static_cast<uint64_t>(std::max<int64_t>(0, value));
		}
		case RATE_SLOW:
			return 5 * n3 / 4;
		case RATE_FLUCTUATING:
			if (n <= 15) {
				return n3 * (((n + 1) / 3) + 24) / 50;
			}
			if (n <= 35) {
				return n3 * (n + 14) / 50;
			}
			return n3 * ((n / 2) + 32) / 50;
		case RATE_MEDIUM_FAST:
		case RATE_NONE:
		default:
			return n == 1 ? 0 : n3;
	}
}

namespace {
uint8_t getPokemonStatOption(uint8_t current, int16_t option, uint8_t maxValue)
{
	if (option < 0) {
		return current;
	}

	return static_cast<uint8_t>(std::clamp<int16_t>(option, 0, maxValue));
}
}

void Pokemon::applyCreateOptions(const PokemonCreateOptions_t& options, bool fullHealth)
{
	const uint8_t previousLevel = level;
	bool statsChanged = false;
	if (options.level >= 0) {
		level = static_cast<uint8_t>(std::clamp<int16_t>(options.level, 1, 100));
		experience = getExperienceForLevel(mType->info.level_rate, level);
		statsChanged = true;
	}

	if (options.ivs.hasAny()) {
		ivs.hp = getPokemonStatOption(ivs.hp, options.ivs.hp, 31);
		ivs.attack = getPokemonStatOption(ivs.attack, options.ivs.attack, 31);
		ivs.defense = getPokemonStatOption(ivs.defense, options.ivs.defense, 31);
		ivs.sp_attack = getPokemonStatOption(ivs.sp_attack, options.ivs.sp_attack, 31);
		ivs.sp_defense = getPokemonStatOption(ivs.sp_defense, options.ivs.sp_defense, 31);
		ivs.speed = getPokemonStatOption(ivs.speed, options.ivs.speed, 31);
		statsChanged = true;
	}

	if (options.evs.hasAny()) {
		evs.hp = getPokemonStatOption(evs.hp, options.evs.hp, 252);
		evs.attack = getPokemonStatOption(evs.attack, options.evs.attack, 252);
		evs.defense = getPokemonStatOption(evs.defense, options.evs.defense, 252);
		evs.sp_attack = getPokemonStatOption(evs.sp_attack, options.evs.sp_attack, 252);
		evs.sp_defense = getPokemonStatOption(evs.sp_defense, options.evs.sp_defense, 252);
		evs.speed = getPokemonStatOption(evs.speed, options.evs.speed, 252);
		statsChanged = true;
	}

	if (options.friendship >= 0) {
		friendship = static_cast<uint8_t>(std::clamp<int16_t>(options.friendship, 0, 255));
	}

	if (options.shiny >= 0) {
		shiny = options.shiny != 0;
	}

	if (options.gender >= 0) {
		gender = static_cast<PokemonGenders_t>(std::clamp<int16_t>(options.gender, GENDER_NONE, GENDER_UNDEFINED));
	}

	if (options.nature >= 0) {
		nature = static_cast<PokemonNatures_t>(std::clamp<int16_t>(options.nature, NATURE_NONE, NATURE_QUIRKY));
		statsChanged = true;
	}

	if (statsChanged) {
		updateStats(!fullHealth);
		if (fullHealth) {
			health = healthMax;
		}

		if (getTile()) {
			g_game.changeSpeed(this, 0);
			g_game.addCreatureHealth(this);
		}
	}

	if (level != previousLevel) {
		learnAvailableMoves();
	}

	syncPokeball();
}

void Pokemon::learnAvailableMoves(bool notify)
{
	PokemonInfo_t info;
	info.level = level;
	info.moves = knownMoves;
	const std::vector<uint16_t> learned = learnPokemonMoves(info, *mType);
	knownMoves = std::move(info.moves);

	if (!notify || learned.empty()) {
		return;
	}

	Player* player = master ? master->getPlayer() : nullptr;
	if (!player) {
		return;
	}

	for (uint16_t moveId : learned) {
		if (const PokemonMoveType* move = g_pokemons.getMoveById(moveId)) {
			player->sendTextMessage(MESSAGE_EVENT_ADVANCE, fmt::format("Your {} learned {}.", getName(), move->name));
		}
	}
}

bool Pokemon::refreshAvailableMoves()
{
	const size_t previousMoveCount = knownMoves.size();
	learnAvailableMoves();
	if (knownMoves.size() == previousMoveCount) {
		return false;
	}

	syncPokeball();
	return true;
}

void Pokemon::updateStats(bool preserveHealth)
{
	stats = calculatePokemonStats(mType->info.base_stats, level, ivs, evs, nature);
	refreshAbilityStats(preserveHealth, false);
}

void Pokemon::refreshAbilityStats(bool preserveHealth, bool notify)
{
	if (calculatingAbilityStats) {
		return;
	}

	calculatingAbilityStats = true;
	const PokemonStats_t calculatedStats = g_pokemons.executeAbilityCalculateStats(this, stats);
	calculatingAbilityStats = false;

	const bool statsChanged = abilityStats.hp != calculatedStats.hp ||
		abilityStats.attack != calculatedStats.attack || abilityStats.defense != calculatedStats.defense ||
		abilityStats.sp_attack != calculatedStats.sp_attack || abilityStats.sp_defense != calculatedStats.sp_defense ||
		abilityStats.speed != calculatedStats.speed;
	const int32_t previousMaxHealth = healthMax;
	const int32_t previousHealth = health;
	const uint32_t previousBaseSpeed = getBaseSpeed();
	abilityStats = calculatedStats;
	healthMax = std::max<int32_t>(1, abilityStats.hp);
	if (preserveHealth) {
		health = std::clamp(health + (healthMax - previousMaxHealth), 0, healthMax);
	} else {
		health = std::clamp(health, 0, healthMax);
	}
	refreshBattleSpeed(false);

	const bool healthChanged = previousHealth != health || previousMaxHealth != healthMax;
	const bool speedChanged = previousBaseSpeed != getBaseSpeed();
	if (healthChanged && getTile()) {
		g_game.addCreatureHealth(this);
	}
	if (notify && (statsChanged || healthChanged || speedChanged)) {
		notifyBattleStateChanged();
	}
}

void Pokemon::syncPokeball()
{
	Player* player = master ? master->getPlayer() : nullptr;
	Pokeball* pokeball = player ? player->getActivePokemon() : nullptr;
	if (!pokeball || pokeball->getPokemon() != this) {
		return;
	}

	PokemonInfo_t info = pokeball->getPokemonInfo();
	info.health = health;
	info.maxHealth = healthMax;
	info.level = level;
	info.experience = experience;
	info.stats = stats;
	info.ivs = ivs;
	info.evs = evs;
	info.nature = nature;
	info.friendship = friendship;
	info.combatFriendshipTime = combatFriendshipTime;
	info.gender = gender;
	info.shiny = shiny;
	info.abilityId = abilityId;
	info.moves = knownMoves;
	pokeball->setPokemonInfo(info);
	player->updatePokemonInfo(pokeball);
}

uint8_t Pokemon::changeFriendship(int32_t amount)
{
	const uint8_t previousFriendship = friendship;
	const int64_t updatedFriendship = static_cast<int64_t>(friendship) + amount;
	friendship = static_cast<uint8_t>(std::clamp<int64_t>(updatedFriendship, 0, 255));
	if (friendship >= 255) {
		combatFriendshipTime = 0;
	}
	if (friendship != previousFriendship && !processingFriendshipChange) {
		processingFriendshipChange = true;
		g_pokemons.executeAbilityFriendshipChange(
			this, previousFriendship, friendship, static_cast<int32_t>(friendship) - previousFriendship);
		processingFriendshipChange = false;
	}
	return friendship;
}

uint8_t Pokemon::addFriendship(int32_t amount)
{
	const uint8_t previousFriendship = friendship;
	changeFriendship(amount);
	if (friendship != previousFriendship) {
		syncPokeball();
	}
	return friendship;
}

void Pokemon::markCombatActivity(Creature* opponent)
{
	const int64_t now = OTSYS_TIME();
	if (abilityCombatActive && lastCombatActivity != 0 &&
			now - lastCombatActivity > POKEMON_COMBAT_ACTIVITY_TIMEOUT) {
		leaveAbilityCombat();
	}
	const bool enteringCombat = !abilityCombatActive;
	if (opponent && opponent != this) {
		abilityCombatOpponentIds.insert(opponent->getID());
	}
	lastCombatActivity = now;
	if (enteringCombat) {
		abilityCombatActive = true;
		g_pokemons.executeAbilityCombatEnter(this, opponent);
	}
}

void Pokemon::processAbilityCombatState()
{
	if (abilityCombatActive && OTSYS_TIME() - lastCombatActivity > POKEMON_COMBAT_ACTIVITY_TIMEOUT) {
		leaveAbilityCombat();
	}
}

void Pokemon::leaveAbilityCombat()
{
	if (abilityCombatActive) {
		abilityCombatActive = false;
		g_pokemons.executeAbilityCombatExit(this);
	}
	abilityCombatOpponentIds.clear();
	clearAbilityState();
}

bool Pokemon::canEscapeCombat(const PokemonMoveType& move)
{
	if (!abilityCombatActive) {
		return true;
	}

	for (uint32_t opponentId : abilityCombatOpponentIds) {
		Creature* opponent = g_game.getCreatureByID(opponentId);
		Pokemon* abilityOwner = opponent ? opponent->getPokemon() : nullptr;
		if (!abilityOwner || abilityOwner == this || !abilityOwner->abilityCombatActive ||
				!canSee(abilityOwner->getPosition())) {
			continue;
		}
		if (!g_pokemons.executeAbilityBeforeEscape(abilityOwner, this, move)) {
			return false;
		}
	}
	return true;
}

void Pokemon::completeCombatEscape()
{
	const std::unordered_set<uint32_t> opponents = abilityCombatOpponentIds;
	setAttackedCreature(nullptr);
	setFollowCreature(isSummon() ? getMaster() : nullptr);
	clearTargetList();

	if (Creature* pokemonMaster = getMaster()) {
		Creature* masterTarget = pokemonMaster->getAttackedCreature();
		if (masterTarget && opponents.find(masterTarget->getID()) != opponents.end()) {
			pokemonMaster->setAttackedCreature(nullptr);
		}
	}

	for (uint32_t opponentId : opponents) {
		Creature* opponentCreature = g_game.getCreatureByID(opponentId);
		Pokemon* opponent = opponentCreature ? opponentCreature->getPokemon() : nullptr;
		if (!opponent || opponent == this) {
			continue;
		}

		opponent->abilityCombatOpponentIds.erase(getID());
		if (opponent->getAttackedCreature() == this) {
			opponent->setAttackedCreature(nullptr);
		}
		if (opponent->getFollowCreature() == this) {
			opponent->setFollowCreature(opponent->isSummon() ? opponent->getMaster() : nullptr);
		}
		opponent->removeTarget(this);
		opponent->updateIdleStatus();

		if (Creature* opponentMaster = opponent->getMaster();
				opponentMaster && opponentMaster->getAttackedCreature() == this) {
			opponentMaster->setAttackedCreature(nullptr);
		}
		if (opponent->abilityCombatOpponentIds.empty()) {
			opponent->leaveAbilityCombat();
		}
	}

	updateIdleStatus();
	leaveAbilityCombat();
}

void Pokemon::processCombatFriendship(uint32_t interval)
{
	Player* player = master ? master->getPlayer() : nullptr;
	if (!isSummon() || !player) {
		return;
	}

	if (friendship >= 255) {
		if (combatFriendshipTime != 0) {
			combatFriendshipTime = 0;
			if (Pokeball* pokeball = player->getActivePokemon(); pokeball && pokeball->getPokemon() == this) {
				pokeball->setPokemonCombatFriendshipTime(0);
			}
		}
		return;
	}

	const int64_t now = OTSYS_TIME();
	if (lastCombatActivity == 0 || now - lastCombatActivity > POKEMON_COMBAT_ACTIVITY_TIMEOUT) {
		return;
	}

	combatFriendshipTime = std::min<uint32_t>(combatFriendshipTime + interval, POKEMON_COMBAT_FRIENDSHIP_INTERVAL);
	if (combatFriendshipTime >= POKEMON_COMBAT_FRIENDSHIP_INTERVAL) {
		combatFriendshipTime -= POKEMON_COMBAT_FRIENDSHIP_INTERVAL;
		addFriendship(1);
	}

	if (Pokeball* pokeball = player->getActivePokemon(); pokeball && pokeball->getPokemon() == this) {
		pokeball->setPokemonCombatFriendshipTime(combatFriendshipTime);
	}
}

uint8_t Pokemon::addExperience(uint64_t amount, bool sendText)
{
	if (amount == 0 || level >= 100) {
		return 0;
	}

	const uint64_t maxExperience = getExperienceForLevel(mType->info.level_rate, 100);
	const uint64_t oldExperience = experience;
	experience = amount > maxExperience - experience ? maxExperience : experience + amount;
	const uint64_t gainedExperience = experience - oldExperience;
	if (gainedExperience == 0) {
		return 0;
	}

	const uint8_t previousLevel = level;
	while (level < 100 && experience >= getExperienceForLevel(mType->info.level_rate, level + 1)) {
		++level;
	}

	if (level != previousLevel) {
		changeFriendship(level - previousLevel);
		learnAvailableMoves(true);
		updateStats(true);
		processEvolutionEvent(previousLevel);
		g_game.changeSpeed(this, 0);
		g_game.addCreatureHealth(this);
		g_game.addMagicEffect(this->getPosition(), CONST_ME_HOLYDAMAGE);
	}

	syncPokeball();

	if (sendText) {
		TextMessage message(MESSAGE_EXPERIENCE_OTHERS, ucfirst(getNameDescription()) + " gained " + std::to_string(gainedExperience) + (gainedExperience != 1 ? " experience points." : " experience point."));
		message.position = position;
		message.primary.color = TEXTCOLOR_WHITE_EXP;
		message.primary.value = gainedExperience;

		SpectatorVec spectators;
		g_game.map.getSpectators(spectators, position, false, true);
		for (Creature* spectator : spectators) {
			spectator->getPlayer()->sendTextMessage(message);
		}
	}

	if (level != previousLevel) {
		if (Player* player = master ? master->getPlayer() : nullptr) {
			player->sendTextMessage(MESSAGE_EVENT_ADVANCE, fmt::format("Your {} advanced from Level {:d} to Level {:d}.", getName(), previousLevel, level));
		}
	}

	return level - previousLevel;
}

bool Pokemon::setLevel(uint8_t newLevel, bool fullHealth)
{
	newLevel = std::clamp<uint8_t>(newLevel, 1, 100);
	if (level == newLevel) {
		if (fullHealth) {
			health = healthMax;
			if (getTile()) {
				g_game.addCreatureHealth(this);
			}
		}
		syncPokeball();
		return false;
	}

	const uint8_t previousLevel = level;
	level = newLevel;
	if (level > previousLevel) {
		changeFriendship(level - previousLevel);
	}
	experience = getExperienceForLevel(mType->info.level_rate, level);
	learnAvailableMoves();
	updateStats(!fullHealth);
	processEvolutionEvent(previousLevel);
	if (fullHealth) {
		health = healthMax;
	}

	if (getTile()) {
		g_game.changeSpeed(this, 0);
		g_game.addCreatureHealth(this);
	}
	syncPokeball();
	return true;
}

bool Pokemon::addLevel(bool sendText)
{
	if (level >= 100) {
		return false;
	}

	const uint64_t nextLevelExperience = getExperienceForLevel(mType->info.level_rate, level + 1);
	addExperience(nextLevelExperience - experience, sendText);
	return true;
}

uint64_t Pokemon::getGainedExperience(Creature* attacker) const
{
	if (!attacker || mType->info.base_experience == 0) {
		return 0;
	}

	// Generation IV flat formula for a wild battle: floor(b * L / 7),
	// distributed by each attacker's contribution in this real-time battle system.
	const uint64_t battleExperience = static_cast<uint64_t>(mType->info.base_experience) * level / 7;
	return std::floor(getDamageRatio(attacker) * battleExperience);
}

void Pokemon::onGainExperience(uint64_t gainExp, Creature* target)
{
	if (gainExp == 0 || !master) {
		return;
	}

	addExperience(gainExp, true);
	master->onGainExperience(gainExp / 2, target);
}

void Pokemon::addList()
{
	g_game.addPokemon(this);
}

void Pokemon::removeList()
{
	g_game.removePokemon(this);
}

const std::string& Pokemon::getName() const
{
	if (name.empty()) {
		return mType->name;
	}
	return name;
}

void Pokemon::setName(const std::string& name)
{
	if (getName() == name) {
		return;
	}

	this->name = name;

	// NOTE: Due to how client caches known creatures,
	// it is not feasible to send creature update to everyone that has ever met it
	SpectatorVec spectators;
	g_game.map.getSpectators(spectators, position, true, true);
	for (Creature* spectator : spectators) {
		if (Player* tmpPlayer = spectator->getPlayer()) {
			tmpPlayer->sendUpdateTileCreature(this);
		}
	}
}

const std::string& Pokemon::getNameDescription() const
{
	if (nameDescription.empty()) {
		return mType->nameDescription;
	}
	return nameDescription;
}

bool Pokemon::canSee(const Position& pos) const
{
	return Creature::canSee(getPosition(), pos, 9, 9);
}

bool Pokemon::canWalkOnFieldType(CombatType_t combatType) const
{
	switch (combatType) {
		case COMBAT_ENERGYDAMAGE:
			return mType->info.canWalkOnEnergy;
		case COMBAT_FIREDAMAGE:
			return mType->info.canWalkOnFire;
		case COMBAT_EARTHDAMAGE:
			return mType->info.canWalkOnPoison;
		default:
			return true;
	}
}

void Pokemon::onAttackedCreatureDisappear(bool)
{
	attackTicks = 0;
}

void Pokemon::onCreatureAppear(Creature* creature, bool isLogin)
{
	Creature::onCreatureAppear(creature, isLogin);

	if (mType->info.creatureAppearEvent != -1) {
		// onCreatureAppear(self, creature)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Pokemon::onCreatureAppear] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureAppearEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureAppearEvent);

		LuaScriptInterface::pushUserdata<Pokemon>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		if (scriptInterface->callFunction(2)) {
			return;
		}
	}

	if (creature == this) {
		//We just spawned lets look around to see who is there.
		if (isSummon()) {
			isMasterInRange = canSee(getMaster()->getPosition());
		}

		updateTargetList();
		updateIdleStatus();
	} else {
		onCreatureEnter(creature);
	}
}

void Pokemon::processEvolutionEvent(uint8_t previousLevel)
{
	const auto& evolution = mType->info.evolution;
	if (evolution.type == EVOLVE_LEVEL && evolution.level > 0 &&
			previousLevel < evolution.level && level >= evolution.level) {
		g_pokemons.executeAbilityEvolution(this, evolution.type, evolution.level);
	}
}

void Pokemon::onPlacedCreature()
{
	if (abilitySpawnProcessed) {
		return;
	}
	abilitySpawnProcessed = true;
	g_pokemons.executeAbilitySpawn(this);
	if (Creature* master = getMaster()) {
		g_pokemons.executeAbilitySummon(this, master);
	}
	updateTargetList();
}

void Pokemon::onRemoveCreature(Creature* creature, bool isLogout)
{
	Creature::onRemoveCreature(creature, isLogout);

	if (mType->info.creatureDisappearEvent != -1) {
		// onCreatureDisappear(self, creature)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Pokemon::onCreatureDisappear] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureDisappearEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureDisappearEvent);

		LuaScriptInterface::pushUserdata<Pokemon>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		if (scriptInterface->callFunction(2)) {
			return;
		}
	}

	if (creature == this) {
		if (spawn) {
			spawn->startSpawnCheck();
		}

		setIdle(true);
	} else {
		onCreatureLeave(creature);
	}
}

void Pokemon::onCreatureMove(Creature* creature, const Tile* newTile, const Position& newPos,
                             const Tile* oldTile, const Position& oldPos, bool teleport)
{
	Creature::onCreatureMove(creature, newTile, newPos, oldTile, oldPos, teleport);

	if (mType->info.creatureMoveEvent != -1) {
		// onCreatureMove(self, creature, oldPosition, newPosition)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Pokemon::onCreatureMove] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureMoveEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureMoveEvent);

		LuaScriptInterface::pushUserdata<Pokemon>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		LuaScriptInterface::pushPosition(L, oldPos);
		LuaScriptInterface::pushPosition(L, newPos);

		if (scriptInterface->callFunction(4)) {
			return;
		}
	}

	if (creature == this) {
		if (isSummon()) {
			isMasterInRange = canSee(getMaster()->getPosition());
		}

		updateTargetList();
		updateIdleStatus();
		if (!teleport) {
			g_pokemons.executeAbilityStep(this, oldPos, newPos);
		}
	} else {
		bool canSeeNewPos = canSee(newPos);
		bool canSeeOldPos = canSee(oldPos);

		if (canSeeNewPos && !canSeeOldPos) {
			onCreatureEnter(creature);
		} else if (!canSeeNewPos && canSeeOldPos) {
			onCreatureLeave(creature);
		}

		if (canSeeNewPos && isSummon() && getMaster() == creature) {
			isMasterInRange = true; //Follow master again
		}

		updateIdleStatus();

		if (!isSummon()) {
			if (followCreature) {
				const Position& followPosition = followCreature->getPosition();
				const Position& position = getPosition();

				int32_t offset_x = Position::getDistanceX(followPosition, position);
				int32_t offset_y = Position::getDistanceY(followPosition, position);
				if ((offset_x > 1 || offset_y > 1) && mType->info.changeTargetChance > 0) {
					Direction dir = getDirectionTo(position, followPosition);
					const Position& checkPosition = getNextPosition(dir, position);

					Tile* tile = g_game.map.getTile(checkPosition);
					if (tile) {
						Creature* topCreature = tile->getTopCreature();
						if (topCreature && followCreature != topCreature && isOpponent(topCreature)) {
							selectTarget(topCreature);
						}
					}
				}
			} else if (isOpponent(creature)) {
				//we have no target lets try pick this one
				selectTarget(creature);
			}
		}
	}
}

void Pokemon::onCreatureSay(Creature* creature, SpeakClasses type, const std::string& text)
{
	Creature::onCreatureSay(creature, type, text);

	if (mType->info.creatureSayEvent != -1) {
		// onCreatureSay(self, creature, type, message)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Pokemon::onCreatureSay] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.creatureSayEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.creatureSayEvent);

		LuaScriptInterface::pushUserdata<Pokemon>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");

		LuaScriptInterface::pushUserdata<Creature>(L, creature);
		LuaScriptInterface::setCreatureMetatable(L, -1, creature);

		lua_pushnumber(L, type);
		LuaScriptInterface::pushString(L, text);

		scriptInterface->callVoidFunction(4);
	}
}

void Pokemon::addFriend(Creature* creature)
{
	assert(creature != this);
	auto result = friendList.insert(creature);
	if (result.second) {
		creature->incrementReferenceCounter();
	}
}

void Pokemon::removeFriend(Creature* creature)
{
	auto it = friendList.find(creature);
	if (it != friendList.end()) {
		creature->decrementReferenceCounter();
		friendList.erase(it);
	}
}

void Pokemon::addTarget(Creature* creature, bool pushFront/* = false*/)
{
	assert(creature != this);
	if (std::find(targetList.begin(), targetList.end(), creature) == targetList.end()) {
		creature->incrementReferenceCounter();
		if (pushFront) {
			targetList.push_front(creature);
		} else {
			targetList.push_back(creature);
		}
	}
}

void Pokemon::removeTarget(Creature* creature)
{
	auto it = std::find(targetList.begin(), targetList.end(), creature);
	if (it != targetList.end()) {
		creature->decrementReferenceCounter();
		targetList.erase(it);
	}
}

void Pokemon::updateTargetList()
{
	auto friendIterator = friendList.begin();
	while (friendIterator != friendList.end()) {
		Creature* creature = *friendIterator;
		if (creature->getHealth() <= 0 || !canSee(creature->getPosition())) {
			creature->decrementReferenceCounter();
			friendIterator = friendList.erase(friendIterator);
		} else {
			++friendIterator;
		}
	}

	auto targetIterator = targetList.begin();
	while (targetIterator != targetList.end()) {
		Creature* creature = *targetIterator;
		if (creature->getHealth() <= 0 || !canSee(creature->getPosition())) {
			creature->decrementReferenceCounter();
			targetIterator = targetList.erase(targetIterator);
		} else {
			++targetIterator;
		}
	}

	SpectatorVec spectators;
	g_game.map.getSpectators(spectators, position, true);
	spectators.erase(this);
	for (Creature* spectator : spectators) {
		onCreatureFound(spectator);
	}
}

void Pokemon::clearTargetList()
{
	for (Creature* creature : targetList) {
		creature->decrementReferenceCounter();
	}
	targetList.clear();
}

void Pokemon::clearFriendList()
{
	for (Creature* creature : friendList) {
		creature->decrementReferenceCounter();
	}
	friendList.clear();
}

void Pokemon::onCreatureFound(Creature* creature, bool pushFront/* = false*/)
{
	if (!creature) {
		return;
	}

	if (!canSee(creature->getPosition())) {
		return;
	}
	processEncounter(creature);

	if (isFriend(creature)) {
		addFriend(creature);
	}

	if (isOpponent(creature)) {
		addTarget(creature, pushFront);
	}

	updateIdleStatus();
}

void Pokemon::processEncounter(Creature* creature)
{
	if (!abilitySpawnProcessed || !isSummon() || !getMaster() || !getMaster()->getPlayer() ||
			!creature || creature == this) {
		return;
	}

	Pokemon* encountered = creature->getPokemon();
	if (!encountered || encountered->isSummon()) {
		return;
	}
	if (encounteredPokemonIds.insert(encountered->getID()).second) {
		g_pokemons.executeAbilityEncounter(this, encountered);
	}
}

void Pokemon::onCreatureEnter(Creature* creature)
{
	// std::cout << "onCreatureEnter - " << creature->getName() << std::endl;

	if (getMaster() == creature) {
		//Follow master again
		isMasterInRange = true;
	}

	onCreatureFound(creature, true);
}

bool Pokemon::isFriend(const Creature* creature) const
{
	if (isSummon() && getMaster()->getPlayer()) {
		const Player* masterPlayer = getMaster()->getPlayer();
		const Player* tmpPlayer = nullptr;

		if (creature->getPlayer()) {
			tmpPlayer = creature->getPlayer();
		} else {
			const Creature* creatureMaster = creature->getMaster();

			if (creatureMaster && creatureMaster->getPlayer()) {
				tmpPlayer = creatureMaster->getPlayer();
			}
		}

		if (tmpPlayer && (tmpPlayer == getMaster() || masterPlayer->isPartner(tmpPlayer))) {
			return true;
		}
	} else if (creature->getPokemon() && !creature->isSummon()) {
		return true;
	}

	return false;
}

bool Pokemon::isOpponent(const Creature* creature) const
{
	if (isSummon() && getMaster()->getPlayer()) {
		if (creature != getMaster()) {
			return true;
		}
	} else {
		if ((creature->getPlayer() && !creature->getPlayer()->hasFlag(PlayerFlag_IgnoredByPokemons)) ||
		        (creature->getMaster() && creature->getMaster()->getPlayer())) {
			return true;
		}
	}

	return false;
}

void Pokemon::onCreatureLeave(Creature* creature)
{
	// std::cout << "onCreatureLeave - " << creature->getName() << std::endl;

	if (getMaster() == creature) {
		//Take random steps and only use defense abilities (e.g. heal) until its master comes back
		isMasterInRange = false;
	}
	if (creature) {
		encounteredPokemonIds.erase(creature->getID());
	}

	//update friendList
	if (isFriend(creature)) {
		removeFriend(creature);
	}

	//update targetList
	if (isOpponent(creature)) {
		removeTarget(creature);
		updateIdleStatus();

		if (!isSummon() && targetList.empty()) {
			int32_t walkToSpawnRadius = g_config.getNumber(ConfigManager::DEFAULT_WALKTOSPAWNRADIUS);
			if (walkToSpawnRadius > 0 && !Position::areInRange(position, masterPos, walkToSpawnRadius, walkToSpawnRadius)) {
				walkToSpawn();
			}
		}
	}
}

bool Pokemon::searchTarget(TargetSearchType_t searchType /*= TARGETSEARCH_DEFAULT*/)
{
	std::list<Creature*> resultList;
	const Position& myPos = getPosition();

	for (Creature* creature : targetList) {
		if (followCreature != creature && isTarget(creature)) {
			if (searchType == TARGETSEARCH_RANDOM || canUseAttack(myPos, creature)) {
				resultList.push_back(creature);
			}
		}
	}

	switch (searchType) {
		case TARGETSEARCH_NEAREST: {
			Creature* target = nullptr;
			if (!resultList.empty()) {
				auto it = resultList.begin();
				target = *it;

				if (++it != resultList.end()) {
					const Position& targetPosition = target->getPosition();
					int32_t minRange = Position::getDistanceX(myPos, targetPosition) + Position::getDistanceY(myPos, targetPosition);
					do {
						const Position& pos = (*it)->getPosition();

						int32_t distance = Position::getDistanceX(myPos, pos) + Position::getDistanceY(myPos, pos);
						if (distance < minRange) {
							target = *it;
							minRange = distance;
						}
					} while (++it != resultList.end());
				}
			} else {
				int32_t minRange = std::numeric_limits<int32_t>::max();
				for (Creature* creature : targetList) {
					if (!isTarget(creature)) {
						continue;
					}

					const Position& pos = creature->getPosition();
					int32_t distance = Position::getDistanceX(myPos, pos) + Position::getDistanceY(myPos, pos);
					if (distance < minRange) {
						target = creature;
						minRange = distance;
					}
				}
			}

			if (target && selectTarget(target)) {
				return true;
			}
			break;
		}

		case TARGETSEARCH_DEFAULT:
		case TARGETSEARCH_ATTACKRANGE:
		case TARGETSEARCH_RANDOM:
		default: {
			if (!resultList.empty()) {
				auto it = resultList.begin();
				std::advance(it, uniform_random(0, resultList.size() - 1));
				return selectTarget(*it);
			}

			if (searchType == TARGETSEARCH_ATTACKRANGE) {
				return false;
			}

			break;
		}
	}

	//lets just pick the first target in the list
	for (Creature* target : targetList) {
		if (followCreature != target && selectTarget(target)) {
			return true;
		}
	}
	return false;
}

void Pokemon::onFollowCreatureComplete(const Creature* creature)
{
	if (creature) {
		auto it = std::find(targetList.begin(), targetList.end(), creature);
		if (it != targetList.end()) {
			Creature* target = (*it);
			targetList.erase(it);

			if (hasFollowPath) {
				targetList.push_front(target);
			} else if (!isSummon()) {
				targetList.push_back(target);
			} else {
				target->decrementReferenceCounter();
			}
		}
	}
}

BlockType_t Pokemon::blockHit(Creature* attacker, CombatType_t combatType, int32_t& damage,
                              bool checkDefense /* = false*/, bool checkArmor /* = false*/, bool field /* = false */, bool ignoreResistances /* = false */)
{
	const Pokemon* attackingPokemon = attacker ? attacker->getPokemon() : nullptr;
	const bool pokemonFormulaDamage = attackingPokemon && attackingPokemon->isExecutingPokemonMove();
	BlockType_t blockType = Creature::blockHit(attacker, combatType, damage,
		pokemonFormulaDamage ? false : checkDefense, pokemonFormulaDamage ? false : checkArmor,
		field, pokemonFormulaDamage || ignoreResistances);

	if (!pokemonFormulaDamage && damage != 0) {
		int32_t elementMod = 0;
		auto it = mType->info.elementMap.find(combatType);
		if (it != mType->info.elementMap.end()) {
			elementMod = it->second;
		}

		if (elementMod != 0) {
			damage = static_cast<int32_t>(std::round(damage * ((100 - elementMod) / 100.)));
			if (damage <= 0) {
				damage = 0;
				blockType = BLOCK_ARMOR;
			}
		}
	}

	return blockType;
}

bool Pokemon::isTarget(const Creature* creature) const
{
	if (creature->isRemoved() || !creature->isAttackable() ||
	        creature->getZone() == ZONE_PROTECTION || !canSeeCreature(creature)) {
		return false;
	}

	if (creature->getPosition().z != getPosition().z) {
		return false;
	}
	return true;
}

bool Pokemon::selectTarget(Creature* creature)
{
	if (!isTarget(creature)) {
		return false;
	}

	auto it = std::find(targetList.begin(), targetList.end(), creature);
	if (it == targetList.end()) {
		//Target not found in our target list.
		return false;
	}

	if (isHostile() || isSummon()) {
		if (setAttackedCreature(creature) && !isSummon()) {
			g_dispatcher.addTask(createTask(std::bind(&Game::checkCreatureAttack, &g_game, getID())));
		}
	}
	return setFollowCreature(creature);
}

void Pokemon::setIdle(bool idle)
{
	if (isRemoved() || getHealth() <= 0) {
		return;
	}

	isIdle = idle;

	if (!isIdle) {
		g_game.addCreatureCheck(this);
	} else {
		onIdleStatus();
		clearTargetList();
		clearFriendList();
		Game::removeCreatureCheck(this);
	}
}

void Pokemon::updateIdleStatus()
{
	bool idle = false;
	if (!isSummon() && targetList.empty()) {
		// check if there are aggressive conditions
		idle = std::find_if(conditions.begin(), conditions.end(), [](Condition* condition) {
			return condition->isAggressive();
		}) == conditions.end();
	}

	setIdle(idle);
}

void Pokemon::onAddCondition(ConditionType_t type)
{
	if (type == CONDITION_FIRE || type == CONDITION_ENERGY || type == CONDITION_POISON) {
		updateMapCache();
	}

	updateIdleStatus();
}

void Pokemon::onEndCondition(ConditionType_t type)
{
	if (type == CONDITION_FIRE || type == CONDITION_ENERGY || type == CONDITION_POISON) {
		ignoreFieldDamage = false;
		updateMapCache();
	}

	updateIdleStatus();
}

void Pokemon::onThink(uint32_t interval)
{
	Creature::onThink(interval);
	processAbilityCombatState();
	processPokemonBattleState();
	processCombatFriendship(interval);

	if (mType->info.thinkEvent != -1) {
		// onThink(self, interval)
		LuaScriptInterface* scriptInterface = mType->info.scriptInterface;
		if (!scriptInterface->reserveScriptEnv()) {
			std::cout << "[Error - Pokemon::onThink] Call stack overflow" << std::endl;
			return;
		}

		ScriptEnvironment* env = scriptInterface->getScriptEnv();
		env->setScriptId(mType->info.thinkEvent, scriptInterface);

		lua_State* L = scriptInterface->getLuaState();
		scriptInterface->pushFunction(mType->info.thinkEvent);

		LuaScriptInterface::pushUserdata<Pokemon>(L, this);
		LuaScriptInterface::setMetatable(L, -1, "Pokemon");

		lua_pushnumber(L, interval);

		if (scriptInterface->callFunction(2)) {
			return;
		}
	}

	if (!isInSpawnRange(position)) {
		g_game.addMagicEffect(this->getPosition(), CONST_ME_POFF);
		if (g_config.getBoolean(ConfigManager::REMOVE_ON_DESPAWN)) {
			g_game.removeCreature(this, false);
		} else {
			g_game.internalTeleport(this, masterPos);
			setIdle(true);
		}
	} else {
		updateIdleStatus();

		if (!isIdle) {
			addEventWalk();

			if (isSummon()) {
				if (!attackedCreature) {
					if (getMaster() && getMaster()->getAttackedCreature()) {
						//This happens if the pokemon is summoned during combat
						selectTarget(getMaster()->getAttackedCreature());
					} else if (getMaster() != followCreature) {
						//Our master has not ordered us to attack anything, lets follow him around instead.
						setFollowCreature(getMaster());
					}
				} else if (attackedCreature == this) {
					setFollowCreature(nullptr);
				} else if (followCreature != attackedCreature) {
					//This happens just after a master orders an attack, so lets follow it as well.
					setFollowCreature(attackedCreature);
				}
			} else if (!targetList.empty()) {
				if (!followCreature || !hasFollowPath) {
					searchTarget();
				} else if (isFleeing()) {
					if (attackedCreature && !canUseAttack(getPosition(), attackedCreature)) {
						searchTarget(TARGETSEARCH_ATTACKRANGE);
					}
				}
			}

			onThinkTarget(interval);
			onThinkYell(interval);
			onThinkDefense(interval);
		}
	}
}

void Pokemon::doAttacking(uint32_t)
{
	if (!attackedCreature || (isSummon() && attackedCreature == this)) {
		return;
	}

	if (isSummon() && master && master->getPlayer()) {
		updateLookDirection();
		return;
	}

	if (!knownMoves.empty()) {
		updateLookDirection();
		const uint8_t firstSlot = static_cast<uint8_t>(uniform_random(1, 4));
		for (uint8_t offset = 0; offset < 4; ++offset) {
			const uint8_t slot = static_cast<uint8_t>(((firstSlot - 1 + offset) % 4) + 1);
			if (useMove(slot, attackedCreature)) {
				break;
			}
		}
	} else {
		updateLookDirection();
	}
}

bool Pokemon::canUseAttack(const Position& pos, const Creature* target) const
{
	if (isHostile()) {
		const Position& targetPos = target->getPosition();
		uint32_t distance = std::max<uint32_t>(Position::getDistanceX(pos, targetPos), Position::getDistanceY(pos, targetPos));
		if (!knownMoves.empty()) {
			for (const PokemonMoveState& state : knownMoves) {
				const PokemonMoveType* move = state.activeSlot != 0 ? g_pokemons.getMoveById(state.moveId) : nullptr;
				if (move && distance <= move->range) {
					return g_game.isSightClear(pos, targetPos, true);
				}
			}
			return false;
		}

		return false;
	}
	return true;
}

bool Pokemon::setMoveSlot(uint16_t moveId, uint8_t slot)
{
	if (slot > 4) {
		return false;
	}

	auto moveIt = std::find_if(knownMoves.begin(), knownMoves.end(), [moveId](const PokemonMoveState& state) {
		return state.moveId == moveId;
	});
	if (moveIt == knownMoves.end()) {
		return false;
	}

	if (slot != 0) {
		for (PokemonMoveState& state : knownMoves) {
			if (state.activeSlot == slot) {
				state.activeSlot = 0;
			}
		}
	}
	moveIt->activeSlot = slot;
	syncPokeball();
	return true;
}

bool Pokemon::modifyBattleStatStage(PokemonBattleStat_t stat, int8_t amount, uint32_t duration)
{
	if (stat >= POKEMON_BATTLE_STAT_COUNT || amount == 0) {
		return false;
	}

	expireBattleModifiers(OTSYS_TIME());
	int8_t& stage = battleStatStages[stat];
	const int8_t previousStage = stage;
	stage = static_cast<int8_t>(std::clamp<int16_t>(static_cast<int16_t>(stage) + amount, -6, 6));
	const int8_t appliedAmount = stage - previousStage;
	if (appliedAmount == 0) {
		return false;
	}
	battleModifiers.push_back({stat, appliedAmount, OTSYS_TIME() + std::max<uint32_t>(1, duration)});

	if (stat == POKEMON_BATTLE_STAT_SPEED) {
		refreshBattleSpeed();
	} else {
		notifyBattleStateChanged();
	}
	return true;
}

void Pokemon::expireBattleModifiers(int64_t now)
{
	bool changed = false;
	bool speedChanged = false;
	for (const PokemonBattleModifier& modifier : battleModifiers) {
		if (modifier.expiresAt > now) {
			continue;
		}

		battleStatStages[modifier.stat] = static_cast<int8_t>(std::clamp<int16_t>(
			static_cast<int16_t>(battleStatStages[modifier.stat]) - modifier.amount, -6, 6));
		changed = true;
		speedChanged = speedChanged || modifier.stat == POKEMON_BATTLE_STAT_SPEED;
	}

	battleModifiers.erase(std::remove_if(battleModifiers.begin(), battleModifiers.end(), [now](const PokemonBattleModifier& modifier) {
		return modifier.expiresAt <= now;
	}), battleModifiers.end());

	if (speedChanged) {
		refreshBattleSpeed();
	} else if (changed) {
		notifyBattleStateChanged();
	}
}

void Pokemon::notifyBattleStateChanged()
{
	if (Player* player = master ? master->getPlayer() : nullptr) {
		Pokeball* pokeball = player->getActivePokemon();
		if (pokeball && pokeball->getPokemon() == this) {
			player->updatePokemonInfo(pokeball);
		}
	}
}

void Pokemon::refreshBattleSpeed(bool notify)
{
	double modifier = getBattleStatMultiplier(POKEMON_BATTLE_STAT_SPEED);
	if (pokemonStatus == POKEMON_STATUS_PARALYSIS) {
		modifier *= 0.5;
	}
	const int32_t stagedSpeed = static_cast<int32_t>(std::floor(abilityStats.speed * modifier)) + 100;
	const uint32_t newBaseSpeed = std::max<int32_t>(1, stagedSpeed);
	const bool changed = newBaseSpeed != getBaseSpeed();
	setBaseSpeed(newBaseSpeed);
	if (changed && getTile()) {
		g_game.changeSpeed(this, 0);
	}
	if (notify) {
		notifyBattleStateChanged();
	}
}

bool Pokemon::applyStatusCondition(PokemonStatusCondition_t status, uint32_t duration, Creature* source)
{
	processPokemonBattleState();
	if (status == POKEMON_STATUS_NONE || pokemonStatus != POKEMON_STATUS_NONE || getHealth() <= 0) {
		return false;
	}
	if (source && isOpponent(source)) {
		markCombatActivity(source);
		if (Pokemon* sourcePokemon = source->getPokemon()) {
			sourcePokemon->markCombatActivity(this);
		}
	}
	if (!g_pokemons.executeAbilityBeforeStatus(this, source, status, duration)) {
		return false;
	}

	const auto hasType = [this](PokemonTypes_t type) {
		return std::find(mType->info.types.begin(), mType->info.types.end(), type) != mType->info.types.end();
	};
	if ((status == POKEMON_STATUS_BURN && hasType(TYPE_FIRE)) ||
		(status == POKEMON_STATUS_FREEZE && hasType(TYPE_ICE)) ||
		(status == POKEMON_STATUS_PARALYSIS && hasType(TYPE_ELECTRIC)) ||
		(status == POKEMON_STATUS_POISON && (hasType(TYPE_POISON) || hasType(TYPE_STEEL)))) {
		g_game.addMagicEffect(getPosition(), CONST_ME_POFF);
		return false;
	}

	pokemonStatus = status;
	pokemonStatusExpiresAt = OTSYS_TIME() + std::max<uint32_t>(1, duration);
	const bool damagingStatus = status == POKEMON_STATUS_BURN || status == POKEMON_STATUS_POISON;
	pokemonStatusNextTick = OTSYS_TIME() + (damagingStatus ? POKEMON_STATUS_DAMAGE_TICK : POKEMON_STATUS_VISUAL_TICK);
	pokemonStatusSourceId = source ? source->getID() : 0;
	refreshAbilityStats(false, false);
	notifyBattleStateChanged();

	g_game.addMagicEffect(getPosition(), getPokemonStatusEffect(status));
	return true;
}

void Pokemon::clearPokemonStatus()
{
	pokemonStatus = POKEMON_STATUS_NONE;
	pokemonStatusExpiresAt = 0;
	pokemonStatusNextTick = 0;
	pokemonStatusSourceId = 0;
	refreshAbilityStats(false, false);
	notifyBattleStateChanged();
}

bool Pokemon::applyFlinch(uint32_t duration)
{
	if (getHealth() <= 0) {
		return false;
	}
	flinchUntil = std::max<int64_t>(flinchUntil, OTSYS_TIME() + std::max<uint32_t>(1, duration));
	g_game.addMagicEffect(getPosition(), CONST_ME_BLOCKHIT); // Effect 4
	return true;
}

void Pokemon::processPokemonBattleState()
{
	const int64_t now = OTSYS_TIME();
	expireBattleModifiers(now);
	if (pokemonStatus == POKEMON_STATUS_NONE) {
		return;
	}
	if (pokemonStatusExpiresAt <= now) {
		clearPokemonStatus();
		return;
	}

	if (pokemonStatusNextTick > now) {
		return;
	}

	if (pokemonStatus == POKEMON_STATUS_BURN || pokemonStatus == POKEMON_STATUS_POISON) {
		pokemonStatusNextTick = now + POKEMON_STATUS_DAMAGE_TICK;
		CombatDamage damage;
		damage.origin = ORIGIN_CONDITION;
		damage.primary.type = pokemonStatus == POKEMON_STATUS_BURN ? COMBAT_POKEMON_FIREDAMAGE : COMBAT_POKEMON_POISONDAMAGE;
		damage.primary.value = -std::max<int32_t>(1, getMaxHealth() / 16);
		Creature* source = pokemonStatusSourceId != 0 ? g_game.getCreatureByID(pokemonStatusSourceId) : nullptr;
		g_game.combatChangeHealth(source, this, damage);
	} else {
		pokemonStatusNextTick = now + POKEMON_STATUS_VISUAL_TICK;
		g_game.addMagicEffect(getPosition(), getPokemonStatusEffect(pokemonStatus));
	}
}

bool Pokemon::canPerformMove()
{
	processPokemonBattleState();
	const int64_t now = OTSYS_TIME();
	if (flinchUntil > now) {
		flinchUntil = 0;
		g_game.addMagicEffect(getPosition(), CONST_ME_BLOCKHIT); // Effect 4
		return false;
	}
	if (pokemonStatus == POKEMON_STATUS_SLEEP) {
		g_game.addMagicEffect(getPosition(), getPokemonStatusEffect(pokemonStatus));
		return false;
	}
	if (pokemonStatus == POKEMON_STATUS_FREEZE) {
		if (uniform_random(1, 100) <= 20) {
			clearPokemonStatus();
			g_game.addMagicEffect(getPosition(), CONST_ME_ICEATTACK);
		} else {
			g_game.addMagicEffect(getPosition(), getPokemonStatusEffect(pokemonStatus));
			return false;
		}
	}
	if (pokemonStatus == POKEMON_STATUS_PARALYSIS && uniform_random(1, 100) <= 25) {
		g_game.addMagicEffect(getPosition(), getPokemonStatusEffect(pokemonStatus));
		return false;
	}
	if (pokemonStatus == POKEMON_STATUS_CONFUSION && uniform_random(1, 100) <= 33) {
		g_game.addMagicEffect(getPosition(), getPokemonStatusEffect(pokemonStatus));
		return false;
	}
	return true;
}

PokemonStats_t Pokemon::getEffectivePokemonStats() const
{
	PokemonStats_t effectiveStats = abilityStats;
	const auto applyStage = [this](uint8_t value, PokemonBattleStat_t stat) {
		return static_cast<uint8_t>(std::clamp<int32_t>(
			static_cast<int32_t>(std::floor(value * getBattleStatMultiplier(stat))), 1, 255));
	};

	effectiveStats.attack = applyStage(abilityStats.attack, POKEMON_BATTLE_STAT_ATTACK);
	effectiveStats.defense = applyStage(abilityStats.defense, POKEMON_BATTLE_STAT_DEFENSE);
	effectiveStats.sp_attack = applyStage(abilityStats.sp_attack, POKEMON_BATTLE_STAT_SPECIAL_ATTACK);
	effectiveStats.sp_defense = applyStage(abilityStats.sp_defense, POKEMON_BATTLE_STAT_SPECIAL_DEFENSE);
	effectiveStats.speed = applyStage(abilityStats.speed, POKEMON_BATTLE_STAT_SPEED);
	if (pokemonStatus == POKEMON_STATUS_BURN) {
		effectiveStats.attack = std::max<uint8_t>(1, effectiveStats.attack / 2);
	}
	if (pokemonStatus == POKEMON_STATUS_PARALYSIS) {
		effectiveStats.speed = std::max<uint8_t>(1, effectiveStats.speed / 2);
	}
	return effectiveStats;
}

double Pokemon::getBattleStatMultiplier(PokemonBattleStat_t stat) const
{
	if (stat >= POKEMON_BATTLE_STAT_COUNT) {
		return 1.0;
	}

	const int8_t stage = battleStatStages[stat];
	const double base = stat == POKEMON_BATTLE_STAT_ACCURACY ? 3.0 : 2.0;
	return stage >= 0 ? (base + stage) / base : base / (base - stage);
}

int32_t Pokemon::calculateMoveDamage(const PokemonMoveType& move, const Creature* target) const
{
	if (move.category == POKEMON_MOVE_CATEGORY_STATUS || move.power == 0) {
		return 0;
	}

	const Pokemon* targetPokemon = target ? target->getPokemon() : nullptr;
	const PokemonStats_t attackerStats = getEffectivePokemonStats();
	double attackStat = move.category == POKEMON_MOVE_CATEGORY_PHYSICAL ?
		attackerStats.attack : attackerStats.sp_attack;
	double defenseStat = 50;
	if (targetPokemon) {
		const PokemonStats_t targetStats = targetPokemon->getEffectivePokemonStats();
		defenseStat = move.category == POKEMON_MOVE_CATEGORY_PHYSICAL ?
			targetStats.defense : targetStats.sp_defense;
	}
	defenseStat = std::max(1.0, defenseStat);

	const double baseDamage = (((2.0 * level / 5.0 + 2.0) * move.power * attackStat / defenseStat) / 50.0) + 2.0;
	double modifier = uniform_random(85, 100) / 100.0;
	if (mType->info.types[0] == move.type || mType->info.types[1] == move.type) {
		modifier *= 1.5;
	}

	if (targetPokemon) {
		const PokemonType* targetType = targetPokemon->getPokemonTypeData();
		modifier *= getPokemonTypeEffectiveness(move.type, targetType->info.types[0]);
		modifier *= getPokemonTypeEffectiveness(move.type, targetType->info.types[1]);
	}

	if (modifier == 0.0) {
		return 0;
	}
	return std::max<int32_t>(1, static_cast<int32_t>(std::floor(baseDamage * modifier)));
}

bool Pokemon::rollMoveHit(const PokemonMoveType& move, const Creature* target) const
{
	const Pokemon* targetPokemon = target ? target->getPokemon() : nullptr;
	const uint32_t targetEvasion = targetPokemon ? targetPokemon->evasion : 100;
	const double stagedAccuracy = (move.accuracy * accuracy * getBattleStatMultiplier(POKEMON_BATTLE_STAT_ACCURACY)) /
		std::max<uint32_t>(1, targetEvasion);
	const uint32_t hitChance = std::clamp<uint32_t>(static_cast<uint32_t>(std::floor(stagedAccuracy)), 1, 100);
	return static_cast<uint32_t>(uniform_random(1, 100)) <= hitChance;
}

int32_t Pokemon::getExecutingMoveDamage(Creature* target)
{
	if (!executingMove) {
		return 0;
	}
	if (target && isOpponent(target)) {
		markCombatActivity(target);
		if (Pokemon* targetPokemon = target->getPokemon()) {
			targetPokemon->markCombatActivity(this);
		}
	}

	const int32_t damage = calculateMoveDamage(*executingMove, target);
	return target ? g_pokemons.executeAbilityBeforeMoveDamage(this, target, *executingMove, damage) : damage;
}

bool Pokemon::rollExecutingMoveHit(const Creature* target) const
{
	return executingMove && rollMoveHit(*executingMove, target);
}

bool Pokemon::useMove(uint8_t slot, Creature* target)
{
	if (processingPokemonMoveUse || slot == 0 || slot > 4) {
		return false;
	}
	if (!canPerformMove()) {
		return false;
	}

	const auto stateIt = std::find_if(knownMoves.begin(), knownMoves.end(), [slot](const PokemonMoveState& state) {
		return state.activeSlot == slot;
	});
	if (stateIt == knownMoves.end()) {
		return false;
	}

	const PokemonMoveType* move = g_pokemons.getMoveById(stateIt->moveId);
	Move* effect = move && !move->effect.empty() ? g_moves->getMoveByName(move->effect) : nullptr;
	if (!move || !effect) {
		return false;
	}

	const bool needTarget = effect->getNeedTarget();
	if (needTarget && (!target || target->getHealth() <= 0 || !isTarget(target))) {
		return false;
	}

	const int64_t now = OTSYS_TIME();
	const auto cooldownIt = moveCooldowns.find(move->id);
	if (cooldownIt != moveCooldowns.end() && cooldownIt->second > now) {
		return false;
	}
	const auto startCooldown = [this, move, slot, now]() {
		moveCooldowns[move->id] = now + move->cooldown;
		if (Player* player = master ? master->getPlayer() : nullptr) {
			player->sendPokemonMoveCooldown(getID(), slot, move->cooldown);
		}
	};

	updateLookDirection();

	if (needTarget) {
		const Position& fromPosition = getPosition();
		const Position& targetPosition = target->getPosition();
		const uint32_t distance = std::max<uint32_t>(Position::getDistanceX(fromPosition, targetPosition), Position::getDistanceY(fromPosition, targetPosition));
		if (fromPosition.z != targetPosition.z || distance > move->range || !g_game.isSightClear(fromPosition, targetPosition, true)) {
			return false;
		}

	}

	Creature* combatTarget = move->target == POKEMON_MOVE_TARGET_SELF ? this :
		(needTarget ? target : attackedCreature);
	const uint32_t combatTargetId = combatTarget ? combatTarget->getID() : 0;
	processingPokemonMoveUse = true;
	if (!g_pokemons.executeAbilityBeforeMoveUse(this, combatTarget, *move)) {
		processingPokemonMoveUse = false;
		return false;
	}
	if (move->hasFlag(POKEMON_MOVE_FLAG_ESCAPE) && !canEscapeCombat(*move)) {
		startCooldown();
		Creature* currentCombatTarget = combatTargetId != 0 ? g_game.getCreatureByID(combatTargetId) : nullptr;
		g_pokemons.executeAbilityAfterMoveUse(this, currentCombatTarget, *move, false);
		processingPokemonMoveUse = false;
		return false;
	}

	executingPokemonMove = true;
	executingMove = move;
	const bool result = needTarget ? effect->castMove(this, target) : effect->castMove(this);
	executingMove = nullptr;
	executingPokemonMove = false;
	if (result) {
		startCooldown();
		if (!move->hasFlag(POKEMON_MOVE_FLAG_ESCAPE) && combatTarget && isOpponent(combatTarget)) {
			markCombatActivity(combatTarget);
		}
	}
	Creature* currentCombatTarget = combatTargetId != 0 ? g_game.getCreatureByID(combatTargetId) : nullptr;
	g_pokemons.executeAbilityAfterMoveUse(this, currentCombatTarget, *move, result);
	if (result && move->hasFlag(POKEMON_MOVE_FLAG_ESCAPE)) {
		completeCombatEscape();
	}
	processingPokemonMoveUse = false;
	return result;
}

bool Pokemon::canUseMove(const Position& pos, const Position& targetPos,
                          const moveBlock_t& sb, uint32_t interval, bool& inRange, bool& resetTicks)
{
	inRange = true;

	if (sb.isMelee) {
		if (isFleeing() || (OTSYS_TIME() - lastMeleeAttack) < sb.speed) {
			return false;
		}
	} else {
		if (sb.speed > attackTicks) {
			resetTicks = false;
			return false;
		}

		if (attackTicks % sb.speed >= interval) {
			//already used this move for this round
			return false;
		}
	}

	if (sb.range != 0 && std::max<uint32_t>(Position::getDistanceX(pos, targetPos), Position::getDistanceY(pos, targetPos)) > sb.range) {
		inRange = false;
		return false;
	}
	return true;
}

void Pokemon::onThinkTarget(uint32_t interval)
{
	if (!isSummon()) {
		if (mType->info.changeTargetSpeed != 0) {
			bool canChangeTarget = true;

			if (challengeFocusDuration > 0) {
				challengeFocusDuration -= interval;

				if (challengeFocusDuration <= 0) {
					challengeFocusDuration = 0;
				}
			}

			if (targetChangeCooldown > 0) {
				targetChangeCooldown -= interval;

				if (targetChangeCooldown <= 0) {
					targetChangeCooldown = 0;
					targetChangeTicks = mType->info.changeTargetSpeed;
				} else {
					canChangeTarget = false;
				}
			}

			if (canChangeTarget) {
				targetChangeTicks += interval;

				if (targetChangeTicks >= mType->info.changeTargetSpeed) {
					targetChangeTicks = 0;
					targetChangeCooldown = mType->info.changeTargetSpeed;

					if (challengeFocusDuration > 0) {
						challengeFocusDuration = 0;
					}

					if (mType->info.changeTargetChance >= uniform_random(1, 100)) {
						if (mType->info.targetDistance <= 1) {
							searchTarget(TARGETSEARCH_RANDOM);
						} else {
							searchTarget(TARGETSEARCH_NEAREST);
						}
					}
				}
			}
		}
	}
}

void Pokemon::onThinkDefense(uint32_t interval)
{
	bool resetTicks = true;
	defenseTicks += interval;

	for (const moveBlock_t& moveBlock : mType->info.defenseMoves) {
		if (moveBlock.speed > defenseTicks) {
			resetTicks = false;
			continue;
		}

		if (defenseTicks % moveBlock.speed >= interval) {
			//already used this move for this round
			continue;
		}

		if ((moveBlock.chance >= static_cast<uint32_t>(uniform_random(1, 100)))) {
			minCombatValue = moveBlock.minCombatValue;
			maxCombatValue = moveBlock.maxCombatValue;
			moveBlock.move->castMove(this, this);
		}
	}

	if (!isSummon() && summons.size() < mType->info.maxSummons && hasFollowPath) {
		for (const summonBlock_t& summonBlock : mType->info.summons) {
			if (summonBlock.speed > defenseTicks) {
				resetTicks = false;
				continue;
			}

			if (summons.size() >= mType->info.maxSummons) {
				continue;
			}

			if (defenseTicks % summonBlock.speed >= interval) {
				//already used this move for this round
				continue;
			}

			uint32_t summonCount = 0;
			for (Creature* summon : summons) {
				if (summon->getName() == summonBlock.name) {
					++summonCount;
				}
			}

			if (summonCount >= summonBlock.max) {
				continue;
			}

			if (summonBlock.chance < static_cast<uint32_t>(uniform_random(1, 100))) {
				continue;
			}

			Pokemon* summon = Pokemon::createPokemon(summonBlock.name);
			if (summon) {
				if (g_game.placeCreature(summon, getPosition(), false, summonBlock.force)) {
					summon->setDropLoot(false);
					summon->setSkillLoss(false);
					summon->setMaster(this);
					g_game.addMagicEffect(getPosition(), CONST_ME_MAGIC_BLUE);
					g_game.addMagicEffect(summon->getPosition(), CONST_ME_TELEPORT);
				} else {
					delete summon;
				}
			}
		}
	}

	if (resetTicks) {
		defenseTicks = 0;
	}
}

void Pokemon::onThinkYell(uint32_t interval)
{
	if (mType->info.yellSpeedTicks == 0) {
		return;
	}

	yellTicks += interval;
	if (yellTicks >= mType->info.yellSpeedTicks) {
		yellTicks = 0;

		if (!mType->info.voiceVector.empty() && (mType->info.yellChance >= static_cast<uint32_t>(uniform_random(1, 100)))) {
			uint32_t index = uniform_random(0, mType->info.voiceVector.size() - 1);
			const voiceBlock_t& vb = mType->info.voiceVector[index];

			if (vb.yellText) {
				g_game.internalCreatureSay(this, TALKTYPE_POKEMON_YELL, vb.text, false);
			} else {
				g_game.internalCreatureSay(this, TALKTYPE_POKEMON_SAY, vb.text, false);
			}
		}
	}
}

bool Pokemon::walkToSpawn()
{
	if (walkingToSpawn || !spawn || !targetList.empty()) {
		return false;
	}

	int32_t distance = std::max<int32_t>(Position::getDistanceX(position, masterPos), Position::getDistanceY(position, masterPos));
	if (distance == 0) {
		return false;
	}

	listWalkDir.clear();
	if (!getPathTo(masterPos, listWalkDir, 0, std::max<int32_t>(0, distance - 5), true, true, distance)) {
		return false;
	}

	walkingToSpawn = true;
	startAutoWalk();
	return true;
}

void Pokemon::onWalk()
{
	Creature::onWalk();
}

void Pokemon::onWalkComplete()
{
	// Continue walking to spawn
	if (walkingToSpawn) {
		walkingToSpawn = false;
		walkToSpawn();
	}
}

bool Pokemon::pushItem(Item* item)
{
	const Position& centerPos = item->getPosition();

	static std::vector<std::pair<int32_t, int32_t>> relList {
		{-1, -1}, {0, -1}, {1, -1},
		{-1,  0},          {1,  0},
		{-1,  1}, {0,  1}, {1,  1}
	};

	std::shuffle(relList.begin(), relList.end(), getRandomGenerator());

	for (const auto& it : relList) {
		Position tryPos(centerPos.x + it.first, centerPos.y + it.second, centerPos.z);
		Tile* tile = g_game.map.getTile(tryPos);
		if (tile && g_game.canThrowObjectTo(centerPos, tryPos, true, true)) {
			if (g_game.internalMoveItem(item->getParent(), tile, INDEX_WHEREEVER, item, item->getItemCount(), nullptr) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
	}
	return false;
}

void Pokemon::pushItems(Tile* tile)
{
	//We can not use iterators here since we can push the item to another tile
	//which will invalidate the iterator.
	//start from the end to minimize the amount of traffic
	if (TileItemVector* items = tile->getItemList()) {
		uint32_t moveCount = 0;
		uint32_t removeCount = 0;

		int32_t downItemSize = tile->getDownItemCount();
		for (int32_t i = downItemSize; --i >= 0;) {
			Item* item = items->at(i);
			if (item && item->hasProperty(CONST_PROP_MOVEABLE) && (item->hasProperty(CONST_PROP_BLOCKPATH)
			        || item->hasProperty(CONST_PROP_BLOCKSOLID))) {
				if (moveCount < 20 && Pokemon::pushItem(item)) {
					++moveCount;
				} else if (g_game.internalRemoveItem(item) == RETURNVALUE_NOERROR) {
					++removeCount;
				}
			}
		}

		if (removeCount > 0) {
			g_game.addMagicEffect(tile->getPosition(), CONST_ME_POFF);
		}
	}
}

bool Pokemon::pushCreature(Creature* creature)
{
	static std::vector<Direction> dirList {
			DIRECTION_NORTH,
		DIRECTION_WEST, DIRECTION_EAST,
			DIRECTION_SOUTH
	};
	std::shuffle(dirList.begin(), dirList.end(), getRandomGenerator());

	for (Direction dir : dirList) {
		const Position& tryPos = Moves::getCasterPosition(creature, dir);
		Tile* toTile = g_game.map.getTile(tryPos);
		if (toTile && !toTile->hasFlag(TILESTATE_BLOCKPATH)) {
			if (g_game.internalMoveCreature(creature, dir) == RETURNVALUE_NOERROR) {
				return true;
			}
		}
	}
	return false;
}

void Pokemon::pushCreatures(Tile* tile)
{
	//We can not use iterators here since we can push a creature to another tile
	//which will invalidate the iterator.
	if (CreatureVector* creatures = tile->getCreatures()) {
		uint32_t removeCount = 0;
		Pokemon* lastPushedPokemon = nullptr;

		for (size_t i = 0; i < creatures->size();) {
			Pokemon* pokemon = creatures->at(i)->getPokemon();
			if (pokemon && pokemon->isPushable()) {
				if (pokemon != lastPushedPokemon && Pokemon::pushCreature(pokemon)) {
					lastPushedPokemon = pokemon;
					continue;
				}

				pokemon->changeHealth(-pokemon->getHealth());
				removeCount++;
			}

			++i;
		}

		if (removeCount > 0) {
			g_game.addMagicEffect(tile->getPosition(), CONST_ME_BLOCKHIT);
		}
	}
}

bool Pokemon::getNextStep(Direction& direction, uint32_t& flags)
{
	const int64_t now = OTSYS_TIME();
	if (flinchUntil > now || ((pokemonStatus == POKEMON_STATUS_SLEEP || pokemonStatus == POKEMON_STATUS_FREEZE) && pokemonStatusExpiresAt > now)) {
		return false;
	}
	if (!walkingToSpawn && (isIdle || getHealth() <= 0)) {
		//we don't have anyone watching, might as well stop walking
		eventWalk = 0;
		return false;
	}

	bool result = false;
	if (!walkingToSpawn && (!followCreature || !hasFollowPath) && (!isSummon() || !isMasterInRange)) {
		if (getTimeSinceLastMove() >= 1000) {
			randomStepping = true;
			//choose a random direction
			result = getRandomStep(getPosition(), direction);
		}
	} else if ((isSummon() && isMasterInRange) || followCreature || walkingToSpawn) {
		randomStepping = false;
		result = Creature::getNextStep(direction, flags);
		if (result) {
			flags |= FLAG_PATHFINDING;
		} else {
			if (ignoreFieldDamage) {
				ignoreFieldDamage = false;
				updateMapCache();
			}
			//target dancing
			if (attackedCreature && attackedCreature == followCreature) {
				if (isFleeing()) {
					result = getDanceStep(getPosition(), direction, false, false);
				} else if (mType->info.staticAttackChance < static_cast<uint32_t>(uniform_random(1, 100))) {
					result = getDanceStep(getPosition(), direction);
				}
			}
		}
	}

	if (result && (canPushItems() || canPushCreatures())) {
		const Position& pos = Moves::getCasterPosition(this, direction);
		Tile* tile = g_game.map.getTile(pos);
		if (tile) {
			if (canPushItems()) {
				Pokemon::pushItems(tile);
			}

			if (canPushCreatures()) {
				Pokemon::pushCreatures(tile);
			}
		}
	}

	return result;
}

bool Pokemon::getRandomStep(const Position& creaturePos, Direction& direction) const
{
	static std::vector<Direction> dirList{
			DIRECTION_NORTH,
		DIRECTION_WEST, DIRECTION_EAST,
			DIRECTION_SOUTH
	};
	std::shuffle(dirList.begin(), dirList.end(), getRandomGenerator());

	for (Direction dir : dirList) {
		if (canWalkTo(creaturePos, dir)) {
			direction = dir;
			return true;
		}
	}
	return false;
}

bool Pokemon::getDanceStep(const Position& creaturePos, Direction& direction,
                           bool keepAttack /*= true*/, bool keepDistance /*= true*/)
{
	bool canDoAttackNow = canUseAttack(creaturePos, attackedCreature);

	assert(attackedCreature != nullptr);
	const Position& centerPos = attackedCreature->getPosition();

	int_fast32_t offset_x = Position::getOffsetX(creaturePos, centerPos);
	int_fast32_t offset_y = Position::getOffsetY(creaturePos, centerPos);

	int_fast32_t distance_x = std::abs(offset_x);
	int_fast32_t distance_y = std::abs(offset_y);

	uint32_t centerToDist = std::max<uint32_t>(distance_x, distance_y);

	std::vector<Direction> dirList;

	if (!keepDistance || offset_y >= 0) {
		uint32_t tmpDist = std::max<uint32_t>(distance_x, std::abs((creaturePos.getY() - 1) - centerPos.getY()));
		if (tmpDist == centerToDist && canWalkTo(creaturePos, DIRECTION_NORTH)) {
			bool result = true;

			if (keepAttack) {
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x, creaturePos.y - 1, creaturePos.z), attackedCreature));
			}

			if (result) {
				dirList.push_back(DIRECTION_NORTH);
			}
		}
	}

	if (!keepDistance || offset_y <= 0) {
		uint32_t tmpDist = std::max<uint32_t>(distance_x, std::abs((creaturePos.getY() + 1) - centerPos.getY()));
		if (tmpDist == centerToDist && canWalkTo(creaturePos, DIRECTION_SOUTH)) {
			bool result = true;

			if (keepAttack) {
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x, creaturePos.y + 1, creaturePos.z), attackedCreature));
			}

			if (result) {
				dirList.push_back(DIRECTION_SOUTH);
			}
		}
	}

	if (!keepDistance || offset_x <= 0) {
		uint32_t tmpDist = std::max<uint32_t>(std::abs((creaturePos.getX() + 1) - centerPos.getX()), distance_y);
		if (tmpDist == centerToDist && canWalkTo(creaturePos, DIRECTION_EAST)) {
			bool result = true;

			if (keepAttack) {
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x + 1, creaturePos.y, creaturePos.z), attackedCreature));
			}

			if (result) {
				dirList.push_back(DIRECTION_EAST);
			}
		}
	}

	if (!keepDistance || offset_x >= 0) {
		uint32_t tmpDist = std::max<uint32_t>(std::abs((creaturePos.getX() - 1) - centerPos.getX()), distance_y);
		if (tmpDist == centerToDist && canWalkTo(creaturePos, DIRECTION_WEST)) {
			bool result = true;

			if (keepAttack) {
				result = (!canDoAttackNow || canUseAttack(Position(creaturePos.x - 1, creaturePos.y, creaturePos.z), attackedCreature));
			}

			if (result) {
				dirList.push_back(DIRECTION_WEST);
			}
		}
	}

	if (!dirList.empty()) {
		std::shuffle(dirList.begin(), dirList.end(), getRandomGenerator());
		direction = dirList[uniform_random(0, dirList.size() - 1)];
		return true;
	}
	return false;
}

bool Pokemon::getDistanceStep(const Position& targetPos, Direction& direction, bool flee /* = false */)
{
	const Position& creaturePos = getPosition();

	int_fast32_t dx = Position::getDistanceX(creaturePos, targetPos);
	int_fast32_t dy = Position::getDistanceY(creaturePos, targetPos);

	int32_t distance = std::max<int32_t>(dx, dy);

	if (!flee && (distance > mType->info.targetDistance || !g_game.isSightClear(creaturePos, targetPos, true))) {
		return false; // let the A* calculate it
	} else if (!flee && distance == mType->info.targetDistance) {
		return true; // we don't really care here, since it's what we wanted to reach (a dance-step will take of dancing in that position)
	}

	int_fast32_t offsetx = Position::getOffsetX(creaturePos, targetPos);
	int_fast32_t offsety = Position::getOffsetY(creaturePos, targetPos);

	if (dx <= 1 && dy <= 1) {
		//seems like a target is near, it this case we need to slow down our movements (as a pokemon)
		if (stepDuration < 2) {
			stepDuration++;
		}
	} else if (stepDuration > 0) {
		stepDuration--;
	}

	if (offsetx == 0 && offsety == 0) {
		return getRandomStep(creaturePos, direction); // player is "on" the pokemon so let's get some random step and rest will be taken care later.
	}

	if (dx == dy) {
		//player is diagonal to the pokemon
		if (offsetx >= 1 && offsety >= 1) {
			// player is NW
			//escape to SE, S or E [and some extra]
			bool s = canWalkTo(creaturePos, DIRECTION_SOUTH);
			bool e = canWalkTo(creaturePos, DIRECTION_EAST);

			if (s && e) {
				direction = boolean_random() ? DIRECTION_SOUTH : DIRECTION_EAST;
				return true;
			} else if (s) {
				direction = DIRECTION_SOUTH;
				return true;
			} else if (e) {
				direction = DIRECTION_EAST;
				return true;
			} else if (canWalkTo(creaturePos, DIRECTION_SOUTHEAST)) {
				direction = DIRECTION_SOUTHEAST;
				return true;
			}

			/* fleeing */
			bool n = canWalkTo(creaturePos, DIRECTION_NORTH);
			bool w = canWalkTo(creaturePos, DIRECTION_WEST);

			if (flee) {
				if (n && w) {
					direction = boolean_random() ? DIRECTION_NORTH : DIRECTION_WEST;
					return true;
				} else if (n) {
					direction = DIRECTION_NORTH;
					return true;
				} else if (w) {
					direction = DIRECTION_WEST;
					return true;
				}
			}

			/* end of fleeing */

			if (w && canWalkTo(creaturePos, DIRECTION_SOUTHWEST)) {
				direction = DIRECTION_WEST;
			} else if (n && canWalkTo(creaturePos, DIRECTION_NORTHEAST)) {
				direction = DIRECTION_NORTH;
			}

			return true;
		} else if (offsetx <= -1 && offsety <= -1) {
			//player is SE
			//escape to NW , W or N [and some extra]
			bool w = canWalkTo(creaturePos, DIRECTION_WEST);
			bool n = canWalkTo(creaturePos, DIRECTION_NORTH);

			if (w && n) {
				direction = boolean_random() ? DIRECTION_WEST : DIRECTION_NORTH;
				return true;
			} else if (w) {
				direction = DIRECTION_WEST;
				return true;
			} else if (n) {
				direction = DIRECTION_NORTH;
				return true;
			}

			if (canWalkTo(creaturePos, DIRECTION_NORTHWEST)) {
				direction = DIRECTION_NORTHWEST;
				return true;
			}

			/* fleeing */
			bool s = canWalkTo(creaturePos, DIRECTION_SOUTH);
			bool e = canWalkTo(creaturePos, DIRECTION_EAST);

			if (flee) {
				if (s && e) {
					direction = boolean_random() ? DIRECTION_SOUTH : DIRECTION_EAST;
					return true;
				} else if (s) {
					direction = DIRECTION_SOUTH;
					return true;
				} else if (e) {
					direction = DIRECTION_EAST;
					return true;
				}
			}

			/* end of fleeing */

			if (s && canWalkTo(creaturePos, DIRECTION_SOUTHWEST)) {
				direction = DIRECTION_SOUTH;
			} else if (e && canWalkTo(creaturePos, DIRECTION_NORTHEAST)) {
				direction = DIRECTION_EAST;
			}

			return true;
		} else if (offsetx >= 1 && offsety <= -1) {
			//player is SW
			//escape to NE, N, E [and some extra]
			bool n = canWalkTo(creaturePos, DIRECTION_NORTH);
			bool e = canWalkTo(creaturePos, DIRECTION_EAST);
			if (n && e) {
				direction = boolean_random() ? DIRECTION_NORTH : DIRECTION_EAST;
				return true;
			} else if (n) {
				direction = DIRECTION_NORTH;
				return true;
			} else if (e) {
				direction = DIRECTION_EAST;
				return true;
			}

			if (canWalkTo(creaturePos, DIRECTION_NORTHEAST)) {
				direction = DIRECTION_NORTHEAST;
				return true;
			}

			/* fleeing */
			bool s = canWalkTo(creaturePos, DIRECTION_SOUTH);
			bool w = canWalkTo(creaturePos, DIRECTION_WEST);

			if (flee) {
				if (s && w) {
					direction = boolean_random() ? DIRECTION_SOUTH : DIRECTION_WEST;
					return true;
				} else if (s) {
					direction = DIRECTION_SOUTH;
					return true;
				} else if (w) {
					direction = DIRECTION_WEST;
					return true;
				}
			}

			/* end of fleeing */

			if (w && canWalkTo(creaturePos, DIRECTION_NORTHWEST)) {
				direction = DIRECTION_WEST;
			} else if (s && canWalkTo(creaturePos, DIRECTION_SOUTHEAST)) {
				direction = DIRECTION_SOUTH;
			}

			return true;
		} else if (offsetx <= -1 && offsety >= 1) {
			// player is NE
			//escape to SW, S, W [and some extra]
			bool w = canWalkTo(creaturePos, DIRECTION_WEST);
			bool s = canWalkTo(creaturePos, DIRECTION_SOUTH);
			if (w && s) {
				direction = boolean_random() ? DIRECTION_WEST : DIRECTION_SOUTH;
				return true;
			} else if (w) {
				direction = DIRECTION_WEST;
				return true;
			} else if (s) {
				direction = DIRECTION_SOUTH;
				return true;
			} else if (canWalkTo(creaturePos, DIRECTION_SOUTHWEST)) {
				direction = DIRECTION_SOUTHWEST;
				return true;
			}

			/* fleeing */
			bool n = canWalkTo(creaturePos, DIRECTION_NORTH);
			bool e = canWalkTo(creaturePos, DIRECTION_EAST);

			if (flee) {
				if (n && e) {
					direction = boolean_random() ? DIRECTION_NORTH : DIRECTION_EAST;
					return true;
				} else if (n) {
					direction = DIRECTION_NORTH;
					return true;
				} else if (e) {
					direction = DIRECTION_EAST;
					return true;
				}
			}

			/* end of fleeing */

			if (e && canWalkTo(creaturePos, DIRECTION_SOUTHEAST)) {
				direction = DIRECTION_EAST;
			} else if (n && canWalkTo(creaturePos, DIRECTION_NORTHWEST)) {
				direction = DIRECTION_NORTH;
			}

			return true;
		}
	}

	//Now let's decide where the player is located to the pokemon (what direction) so we can decide where to escape.
	if (dy > dx) {
		Direction playerDir = offsety < 0 ? DIRECTION_SOUTH : DIRECTION_NORTH;
		switch (playerDir) {
			case DIRECTION_NORTH: {
				// Player is to the NORTH, so obviously we need to check if we can go SOUTH, if not then let's choose WEST or EAST
				// and again if we can't we need to decide about some diagonal movements.
				if (canWalkTo(creaturePos, DIRECTION_SOUTH)) {
					direction = DIRECTION_SOUTH;
					return true;
				}

				bool w = canWalkTo(creaturePos, DIRECTION_WEST);
				bool e = canWalkTo(creaturePos, DIRECTION_EAST);
				if (w && e && offsetx == 0) {
					direction = boolean_random() ? DIRECTION_WEST : DIRECTION_EAST;
					return true;
				} else if (w && offsetx <= 0) {
					direction = DIRECTION_WEST;
					return true;
				} else if (e && offsetx >= 0) {
					direction = DIRECTION_EAST;
					return true;
				}

				/* fleeing */
				if (flee) {
					if (w && e) {
						direction = boolean_random() ? DIRECTION_WEST : DIRECTION_EAST;
						return true;
					} else if (w) {
						direction = DIRECTION_WEST;
						return true;
					} else if (e) {
						direction = DIRECTION_EAST;
						return true;
					}
				}

				/* end of fleeing */

				bool sw = canWalkTo(creaturePos, DIRECTION_SOUTHWEST);
				bool se = canWalkTo(creaturePos, DIRECTION_SOUTHEAST);
				if (sw || se) {
					// we can move both dirs
					if (sw && se) {
						direction = boolean_random() ? DIRECTION_SOUTHWEST : DIRECTION_SOUTHEAST;
					} else if (w) {
						direction = DIRECTION_WEST;
					} else if (sw) {
						direction = DIRECTION_SOUTHWEST;
					} else if (e) {
						direction = DIRECTION_EAST;
					} else if (se) {
						direction = DIRECTION_SOUTHEAST;
					}
					return true;
				}

				/* fleeing */
				if (flee && canWalkTo(creaturePos, DIRECTION_NORTH)) {
					// towards player, yea
					direction = DIRECTION_NORTH;
					return true;
				}

				/* end of fleeing */
				break;
			}

			case DIRECTION_SOUTH: {
				if (canWalkTo(creaturePos, DIRECTION_NORTH)) {
					direction = DIRECTION_NORTH;
					return true;
				}

				bool w = canWalkTo(creaturePos, DIRECTION_WEST);
				bool e = canWalkTo(creaturePos, DIRECTION_EAST);
				if (w && e && offsetx == 0) {
					direction = boolean_random() ? DIRECTION_WEST : DIRECTION_EAST;
					return true;
				} else if (w && offsetx <= 0) {
					direction = DIRECTION_WEST;
					return true;
				} else if (e && offsetx >= 0) {
					direction = DIRECTION_EAST;
					return true;
				}

				/* fleeing */
				if (flee) {
					if (w && e) {
						direction = boolean_random() ? DIRECTION_WEST : DIRECTION_EAST;
						return true;
					} else if (w) {
						direction = DIRECTION_WEST;
						return true;
					} else if (e) {
						direction = DIRECTION_EAST;
						return true;
					}
				}

				/* end of fleeing */

				bool nw = canWalkTo(creaturePos, DIRECTION_NORTHWEST);
				bool ne = canWalkTo(creaturePos, DIRECTION_NORTHEAST);
				if (nw || ne) {
					// we can move both dirs
					if (nw && ne) {
						direction = boolean_random() ? DIRECTION_NORTHWEST : DIRECTION_NORTHEAST;
					} else if (w) {
						direction = DIRECTION_WEST;
					} else if (nw) {
						direction = DIRECTION_NORTHWEST;
					} else if (e) {
						direction = DIRECTION_EAST;
					} else if (ne) {
						direction = DIRECTION_NORTHEAST;
					}
					return true;
				}

				/* fleeing */
				if (flee && canWalkTo(creaturePos, DIRECTION_SOUTH)) {
					// towards player, yea
					direction = DIRECTION_SOUTH;
					return true;
				}

				/* end of fleeing */
				break;
			}

			default:
				break;
		}
	} else {
		Direction playerDir = offsetx < 0 ? DIRECTION_EAST : DIRECTION_WEST;
		switch (playerDir) {
			case DIRECTION_WEST: {
				if (canWalkTo(creaturePos, DIRECTION_EAST)) {
					direction = DIRECTION_EAST;
					return true;
				}

				bool n = canWalkTo(creaturePos, DIRECTION_NORTH);
				bool s = canWalkTo(creaturePos, DIRECTION_SOUTH);
				if (n && s && offsety == 0) {
					direction = boolean_random() ? DIRECTION_NORTH : DIRECTION_SOUTH;
					return true;
				} else if (n && offsety <= 0) {
					direction = DIRECTION_NORTH;
					return true;
				} else if (s && offsety >= 0) {
					direction = DIRECTION_SOUTH;
					return true;
				}

				/* fleeing */
				if (flee) {
					if (n && s) {
						direction = boolean_random() ? DIRECTION_NORTH : DIRECTION_SOUTH;
						return true;
					} else if (n) {
						direction = DIRECTION_NORTH;
						return true;
					} else if (s) {
						direction = DIRECTION_SOUTH;
						return true;
					}
				}

				/* end of fleeing */

				bool se = canWalkTo(creaturePos, DIRECTION_SOUTHEAST);
				bool ne = canWalkTo(creaturePos, DIRECTION_NORTHEAST);
				if (se || ne) {
					if (se && ne) {
						direction = boolean_random() ? DIRECTION_SOUTHEAST : DIRECTION_NORTHEAST;
					} else if (s) {
						direction = DIRECTION_SOUTH;
					} else if (se) {
						direction = DIRECTION_SOUTHEAST;
					} else if (n) {
						direction = DIRECTION_NORTH;
					} else if (ne) {
						direction = DIRECTION_NORTHEAST;
					}
					return true;
				}

				/* fleeing */
				if (flee && canWalkTo(creaturePos, DIRECTION_WEST)) {
					// towards player, yea
					direction = DIRECTION_WEST;
					return true;
				}

				/* end of fleeing */
				break;
			}

			case DIRECTION_EAST: {
				if (canWalkTo(creaturePos, DIRECTION_WEST)) {
					direction = DIRECTION_WEST;
					return true;
				}

				bool n = canWalkTo(creaturePos, DIRECTION_NORTH);
				bool s = canWalkTo(creaturePos, DIRECTION_SOUTH);
				if (n && s && offsety == 0) {
					direction = boolean_random() ? DIRECTION_NORTH : DIRECTION_SOUTH;
					return true;
				} else if (n && offsety <= 0) {
					direction = DIRECTION_NORTH;
					return true;
				} else if (s && offsety >= 0) {
					direction = DIRECTION_SOUTH;
					return true;
				}

				/* fleeing */
				if (flee) {
					if (n && s) {
						direction = boolean_random() ? DIRECTION_NORTH : DIRECTION_SOUTH;
						return true;
					} else if (n) {
						direction = DIRECTION_NORTH;
						return true;
					} else if (s) {
						direction = DIRECTION_SOUTH;
						return true;
					}
				}

				/* end of fleeing */

				bool nw = canWalkTo(creaturePos, DIRECTION_NORTHWEST);
				bool sw = canWalkTo(creaturePos, DIRECTION_SOUTHWEST);
				if (nw || sw) {
					if (nw && sw) {
						direction = boolean_random() ? DIRECTION_NORTHWEST : DIRECTION_SOUTHWEST;
					} else if (n) {
						direction = DIRECTION_NORTH;
					} else if (nw) {
						direction = DIRECTION_NORTHWEST;
					} else if (s) {
						direction = DIRECTION_SOUTH;
					} else if (sw) {
						direction = DIRECTION_SOUTHWEST;
					}
					return true;
				}

				/* fleeing */
				if (flee && canWalkTo(creaturePos, DIRECTION_EAST)) {
					// towards player, yea
					direction = DIRECTION_EAST;
					return true;
				}

				/* end of fleeing */
				break;
			}

			default:
				break;
		}
	}

	return true;
}

bool Pokemon::canWalkTo(Position pos, Direction direction) const
{
	pos = getNextPosition(direction, pos);
	if (isInSpawnRange(pos)) {
		if (getWalkCache(pos) == 0) {
			return false;
		}

		Tile* tile = g_game.map.getTile(pos);
		if (tile && tile->getTopVisibleCreature(this) == nullptr && tile->queryAdd(0, *this, 1, FLAG_PATHFINDING) == RETURNVALUE_NOERROR) {
			return true;
		}
	}
	return false;
}

void Pokemon::death(Creature*)
{
	setAttackedCreature(nullptr);

	for (Creature* summon : summons) {
		summon->changeHealth(-summon->getHealth());
		summon->removeMaster();
	}
	summons.clear();

	clearTargetList();
	clearFriendList();
	onIdleStatus();
}

Item* Pokemon::getCorpse(Creature* lastHitCreature, Creature* mostDamageCreature)
{
	Item* corpse = Creature::getCorpse(lastHitCreature, mostDamageCreature);
	if (corpse) {
		if (mostDamageCreature) {
			if (mostDamageCreature->getPlayer()) {
				corpse->setCorpseOwner(mostDamageCreature->getID());
			} else {
				const Creature* mostDamageCreatureMaster = mostDamageCreature->getMaster();
				if (mostDamageCreatureMaster && mostDamageCreatureMaster->getPlayer()) {
					corpse->setCorpseOwner(mostDamageCreatureMaster->getID());
				}
			}
		}
	}
	return corpse;
}

bool Pokemon::isInSpawnRange(const Position& pos) const
{
	if (!spawn) {
		return true;
	}

	if (Pokemon::despawnRadius == 0) {
		return true;
	}

	if (!Spawns::isInZone(masterPos, Pokemon::despawnRadius, pos)) {
		return false;
	}

	if (Pokemon::despawnRange == 0) {
		return true;
	}

	if (Position::getDistanceZ(pos, masterPos) > Pokemon::despawnRange) {
		return false;
	}

	return true;
}

bool Pokemon::getCombatValues(int32_t& min, int32_t& max)
{
	if (!executingPokemonMove && minCombatValue == 0 && maxCombatValue == 0) {
		return false;
	}

	min = minCombatValue;
	max = maxCombatValue;
	return true;
}

void Pokemon::updateLookDirection()
{
	Direction newDir = getDirection();

	if (attackedCreature) {
		const Position& pos = getPosition();
		const Position& attackedCreaturePos = attackedCreature->getPosition();
		int_fast32_t offsetx = Position::getOffsetX(attackedCreaturePos, pos);
		int_fast32_t offsety = Position::getOffsetY(attackedCreaturePos, pos);

		int32_t dx = std::abs(offsetx);
		int32_t dy = std::abs(offsety);
		if (dx > dy) {
			//look EAST/WEST
			if (offsetx < 0) {
				newDir = DIRECTION_WEST;
			} else {
				newDir = DIRECTION_EAST;
			}
		} else if (dx < dy) {
			//look NORTH/SOUTH
			if (offsety < 0) {
				newDir = DIRECTION_NORTH;
			} else {
				newDir = DIRECTION_SOUTH;
			}
		} else {
			Direction dir = getDirection();
			if (offsetx < 0 && offsety < 0) {
				if (dir == DIRECTION_SOUTH) {
					newDir = DIRECTION_WEST;
				} else if (dir == DIRECTION_NORTH) {
					newDir = DIRECTION_WEST;
				} else if (dir == DIRECTION_EAST) {
					newDir = DIRECTION_NORTH;
				}
			} else if (offsetx < 0 && offsety > 0) {
				if (dir == DIRECTION_NORTH) {
					newDir = DIRECTION_WEST;
				} else if (dir == DIRECTION_SOUTH) {
					newDir = DIRECTION_WEST;
				} else if (dir == DIRECTION_EAST) {
					newDir = DIRECTION_SOUTH;
				}
			} else if (offsetx > 0 && offsety < 0) {
				if (dir == DIRECTION_SOUTH) {
					newDir = DIRECTION_EAST;
				} else if (dir == DIRECTION_NORTH) {
					newDir = DIRECTION_EAST;
				} else if (dir == DIRECTION_WEST) {
					newDir = DIRECTION_NORTH;
				}
			} else {
				if (dir == DIRECTION_NORTH) {
					newDir = DIRECTION_EAST;
				} else if (dir == DIRECTION_SOUTH) {
					newDir = DIRECTION_EAST;
				} else if (dir == DIRECTION_WEST) {
					newDir = DIRECTION_SOUTH;
				}
			}
		}
	}

	g_game.internalCreatureTurn(this, newDir);
}

void Pokemon::dropLoot(Container* corpse, Creature* lastHitCreature)
{
	if (corpse && lootDrop) {
		g_events->eventPokemonOnDropLoot(this, corpse);
		Pokemon* sourcePokemon = lastHitCreature ? lastHitCreature->getPokemon() : nullptr;
		if (sourcePokemon && sourcePokemon != this) {
			g_pokemons.executeAbilityLoot(sourcePokemon, this, corpse, false);
		}
		g_pokemons.executeAbilityLoot(this, this, corpse, true);
	}
}

void Pokemon::setNormalCreatureLight()
{
	internalLight = mType->info.light;
}

void Pokemon::drainHealth(Creature* attacker, int32_t damage)
{
	if (damage > 0 && attacker && isOpponent(attacker)) {
		markCombatActivity(attacker);
	}
	Creature::drainHealth(attacker, damage);

	if (damage > 0 && randomStepping) {
		ignoreFieldDamage = true;
		updateMapCache();
	}

	if (isInvisible()) {
		removeCondition(CONDITION_INVISIBLE);
	}
}

void Pokemon::onAttackedCreatureDrainHealth(Creature* target, int32_t points)
{
	Creature::onAttackedCreatureDrainHealth(target, points);
	if (points > 0 && target && isOpponent(target)) {
		markCombatActivity(target);
	}
}

void Pokemon::changeHealth(int32_t healthChange, bool sendHealthChange/* = true*/)
{
	//In case a player with ignore flag set attacks the pokemon
	setIdle(false);
	Creature::changeHealth(healthChange, sendHealthChange);
	refreshAbilityStats();
}

bool Pokemon::challengeCreature(Creature* creature, bool force/* = false*/)
{
	if (isSummon()) {
		return false;
	}

	if (!mType->info.isChallengeable && !force) {
		return false;
	}

	bool result = selectTarget(creature);
	if (result) {
		targetChangeCooldown = 8000;
		challengeFocusDuration = targetChangeCooldown;
		targetChangeTicks = 0;
	}
	return result;
}

void Pokemon::getPathSearchParams(const Creature* creature, FindPathParams& fpp) const
{
	Creature::getPathSearchParams(creature, fpp);

	fpp.minTargetDist = 1;
	fpp.maxTargetDist = mType->info.targetDistance;

	if (isSummon()) {
		if (getMaster() == creature) {
			fpp.maxTargetDist = 2;
			fpp.fullPathSearch = true;
		} else if (mType->info.targetDistance <= 1) {
			fpp.fullPathSearch = true;
		} else {
			fpp.fullPathSearch = !canUseAttack(getPosition(), creature);
		}
	} else if (isFleeing()) {
		//Distance should be higher than the client view range (Map::maxClientViewportX/Map::maxClientViewportY)
		fpp.maxTargetDist = Map::maxViewportX;
		fpp.clearSight = false;
		fpp.keepDistance = true;
		fpp.fullPathSearch = false;
	} else if (mType->info.targetDistance <= 1) {
		fpp.fullPathSearch = true;
	} else {
		fpp.fullPathSearch = !canUseAttack(getPosition(), creature);
	}
}

bool Pokemon::canPushItems() const
{
	Pokemon* master = this->master ? this->master->getPokemon() : nullptr;
	if (master) {
		return master->mType->info.canPushItems;
	}

	return mType->info.canPushItems;
}

#include "otpch.h"
#include "game.h"
#include "pokemon.h"

#include "pokeball.h"
#include "pokeballs.h"

extern Pokemons g_pokemons;

namespace {
uint8_t getPokemonStatOption(uint8_t current, int16_t option, uint8_t maxValue)
{
	if (option < 0) {
		return current;
	}

	return static_cast<uint8_t>(std::clamp<int16_t>(option, 0, maxValue));
}

void applyPokemonStatOptions(PokemonStats_t& stats, const PokemonStatOptions_t& options, uint8_t maxValue)
{
	stats.hp = getPokemonStatOption(stats.hp, options.hp, maxValue);
	stats.attack = getPokemonStatOption(stats.attack, options.attack, maxValue);
	stats.defense = getPokemonStatOption(stats.defense, options.defense, maxValue);
	stats.sp_attack = getPokemonStatOption(stats.sp_attack, options.sp_attack, maxValue);
	stats.sp_defense = getPokemonStatOption(stats.sp_defense, options.sp_defense, maxValue);
	stats.speed = getPokemonStatOption(stats.speed, options.speed, maxValue);
}
}

Pokeball::Pokeball(uint16_t id) : Item(id), active{ false }, pokemon{nullptr}
{
	gobackEffect = PokeballManager::getPropertiesByPokeballId(id)->gobackEffect;
}

void Pokeball::setPokemon(Pokemon* pokemon)
{
	pInfo.p_id = pokemon->getID();
	this->pokemon = pokemon;
}

void Pokeball::setPokemonMaxHealth()
{
	auto mType = g_pokemons.getPokemonType(pInfo.name);
	pInfo.maxHealth = std::floor((((2 * mType->info.base_stats.hp) + pInfo.ivs.hp + (pInfo.evs.hp / 4)) * pInfo.level) / 100) + pInfo.level + 10;
}

PokemonInfo_t Pokeball::createNewPokemon(std::string pokemon, uint8_t level)
{
	PokemonCreateOptions_t options;
	options.level = level;
	return createNewPokemon(pokemon, options);
}

PokemonInfo_t Pokeball::createNewPokemon(std::string pokemon, const PokemonCreateOptions_t& options)
{
	auto pInfo = PokemonInfo_t();
	pInfo.p_uid = g_game.assignPokemonUID();
	pInfo.name = pokemon;
	pInfo.fainted = false;

	std::mt19937 generator(static_cast<unsigned int>(std::time(0)));
	std::uniform_int_distribution<int> distribution(1, 31);
	pInfo.ivs.hp = distribution(generator);
	pInfo.ivs.attack = distribution(generator);
	pInfo.ivs.defense = distribution(generator);
	pInfo.ivs.sp_attack = distribution(generator);
	pInfo.ivs.sp_defense = distribution(generator);
	pInfo.ivs.speed = distribution(generator);
	pInfo.nature = static_cast<PokemonNatures_t>(uniform_random(NATURE_HARDY, NATURE_QUIRKY));

	auto mType = g_pokemons.getPokemonType(pokemon);
	pInfo.level = static_cast<uint8_t>(std::clamp<int16_t>(options.level >= 0 ? options.level : 1, 1, 100));
	if (options.ivs.hasAny()) {
		applyPokemonStatOptions(pInfo.ivs, options.ivs, 31);
	}
	if (options.evs.hasAny()) {
		applyPokemonStatOptions(pInfo.evs, options.evs, 252);
	}
	if (options.nature >= 0) {
		pInfo.nature = static_cast<PokemonNatures_t>(std::clamp<int16_t>(options.nature, NATURE_NONE, NATURE_QUIRKY));
	}

	pInfo.experience = Pokemon::getExperienceForLevel(mType->info.level_rate, pInfo.level);
	pInfo.stats = calculatePokemonStats(mType->info.base_stats, pInfo.level, pInfo.ivs, pInfo.evs, pInfo.nature);
	pInfo.maxHealth = pInfo.stats.hp;
	pInfo.health = pInfo.maxHealth;
	pInfo.number = mType->info.number;


	pInfo.friendship = options.friendship >= 0 ? static_cast<uint8_t>(std::clamp<int16_t>(options.friendship, 0, 255)) : mType->info.base_friendship;
	if (options.shiny >= 0) {
		pInfo.shiny = options.shiny != 0;
	}

	if (mType->info.gender_ratio.male == 0.0 && mType->info.gender_ratio.female == 0.0) {
		pInfo.gender = GENDER_UNDEFINED;
	}
	else {
		std::vector<double> weights = { mType->info.gender_ratio.male, mType->info.gender_ratio.female };
		std::mt19937 generator(static_cast<unsigned int>(std::time(0)));
		std::discrete_distribution<> distribution(weights.begin(), weights.end());
		int selection = distribution(generator);

		if (selection == 0) {
			pInfo.gender = GENDER_MALE;
		}
		else {
			pInfo.gender = GENDER_FEMALE;
		}
	}
	if (options.gender >= 0) {
		pInfo.gender = static_cast<PokemonGenders_t>(std::clamp<int16_t>(options.gender, GENDER_NONE, GENDER_UNDEFINED));
	}

	return pInfo;
}

PokemonInfo_t Pokeball::createPokeballFromPokemon(Pokemon* pokemon)
{
	auto pInfo = PokemonInfo_t();
	pInfo.p_uid = 0;
	pInfo.name = pokemon->getName();
	pInfo.fainted = false;

	pInfo.ivs = pokemon->getIvs();
	pInfo.evs = pokemon->getEvs();
	pInfo.level = pokemon->getLevel();
	pInfo.experience = pokemon->getExperience();
	pInfo.nature = pokemon->getNature();

	pInfo.maxHealth = pokemon->getMaxHealth();
	pInfo.health = pokemon->getHealth();
	pInfo.number = pokemon->getNumber();
	pInfo.friendship = pokemon->getFriendship();
	pInfo.gender = (PokemonGenders_t)pokemon->getGender();
	pInfo.shiny = pokemon->isShiny();

	return pInfo;
}

#include "otpch.h"
#include "game.h"
#include "pokemon.h"

#include "pokeball.h"

extern Pokemons g_pokemons;

Pokeball::Pokeball(uint16_t id) : Item(id), active{ false }, pokemon{nullptr}
{
	//todo: read effect from somewhere else
	if (id == 26459)
		gobackEffect = 176;
	else if (id == 26460)
		gobackEffect = 177;
	else if (id == 26461)
		gobackEffect = 178;
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

PokemonInfo_t Pokeball::createNewPokemon(std::string pokemon)
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

	pInfo.level = 1;

	auto mType = g_pokemons.getPokemonType(pokemon);
	pInfo.maxHealth = std::floor((((2 * mType->info.base_stats.hp) + pInfo.ivs.hp + (pInfo.evs.hp / 4)) * pInfo.level) / 100) + pInfo.level + 10;
	pInfo.health = pInfo.maxHealth;
	pInfo.number = mType->info.number;


	pInfo.friendship = mType->info.base_friendship;

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

	return pInfo;
}

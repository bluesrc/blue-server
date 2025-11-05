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
	pInfo.maxHealth = g_pokemons.getPokemonType(pInfo.name)->info.healthMax;
}

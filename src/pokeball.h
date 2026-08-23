#pragma once

#include "item.h"
#include "pokemons.h"

class Pokeball final : public Item
{

public:

	explicit Pokeball(uint16_t id);

	Pokeball* getPokeball() override {
		return this;
	}
	const Pokeball* getPokeball() const override {
		return this;
	}

	Pokemon* getPokemon() { return pokemon; }
	void setPokemon(Pokemon* pokemon);

	void setActive(bool active = true) { active = active; }

	uint16_t getGobackEffect() { return gobackEffect; }

	PokemonInfo_t getPokemonInfo() { return pInfo;  }
	void setPokemonInfo(PokemonInfo_t info) { pInfo = info; }

	int32_t getPokemonHealth() { return pInfo.health; }
	void setPokemonHealth(int32_t health) { pInfo.health = health; }

	int32_t getPokemonMaxHealth() { return pInfo.maxHealth; }
	void setPokemonMaxHealth();

	std::string getPokemonName() { return pInfo.name; }
	void setPokemonName(std::string name) { pInfo.name = name; }
	uint8_t getPokemonFriendship() const { return pInfo.friendship; }
	uint8_t addPokemonFriendship(int32_t amount);
	void setPokemonCombatFriendshipTime(uint32_t time) { pInfo.combatFriendshipTime = time; }

	void setPokemonFullHealth() { pInfo.health = pInfo.maxHealth; }
	void setPokemonFainted() { pInfo.fainted = true; }
	void setPokemonAlive() { pInfo.fainted = false; }
	void setPokemonId(uint32_t id) { pInfo.p_id = id; }

	bool isPokemonFainted() { return pInfo.fainted; }

	static PokemonInfo_t createNewPokemon(std::string pokemon, uint8_t level = 1);
	static PokemonInfo_t createNewPokemon(std::string pokemon, const PokemonCreateOptions_t& options);
	static PokemonInfo_t createPokeballFromPokemon(Pokemon* pokemon);

private:
	bool active;
	uint16_t gobackEffect;
	Pokemon* pokemon;
	PokemonInfo_t pInfo;
};

class ThrowablePokeball final : public Item
{
public:
	explicit ThrowablePokeball(uint16_t itemId, uint8_t count = 1) : Item(itemId, count) {}

	ThrowablePokeball* getThrowablePokeball() override {
		return this;
	}
	const ThrowablePokeball* getThrowablePokeball() const override {
		return this;
	}
};

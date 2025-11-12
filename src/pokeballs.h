#pragma once

#include <map>
#include "item.h"

struct PokeballProperties {
	std::string name;
	uint16_t pokeballId;
	uint16_t throwEffect;
	uint16_t gobackEffect;
	uint16_t catchSuccessEffect;
	uint16_t catchFailEffect;

	float catchRate = 1.0f;

	//todo: special rates
	std::string specialScript;
	bool hasSpecialLogic = false;
};

class PokeballManager {
public:
	static std::map<uint16_t, PokeballProperties> pokeballData;

	static bool loadData();

	static const PokeballProperties* getProperties(uint16_t itemId);

	static const PokeballProperties* getPropertiesByPokeballId(uint16_t pokeballId);
};

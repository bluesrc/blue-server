#include "otpch.h"
#include "pugicast.h"

#include "pokeballs.h"

std::map<uint16_t, PokeballProperties> PokeballManager::pokeballData;

const PokeballProperties* PokeballManager::getProperties(uint16_t itemId)
{
	auto it = pokeballData.find(itemId);
	if (it != pokeballData.end()) {
		return &it->second;
	}
	return nullptr;
}

const PokeballProperties* PokeballManager::getPropertiesByPokeballId(uint16_t pokeballId)
{
	for (const auto& pair : pokeballData) {
		if (pair.second.pokeballId == pokeballId) {
			return &pair.second;
		}
	}
	return nullptr;
}

bool PokeballManager::loadData() {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file("data/XML/pokeballs.xml");
	if (!result) {
		printXMLError("Error - loadXMLPokeballs", "data/XML/pokeballs.xml", result);
		return false;
	}

	pokeballData.clear();

	for (auto stageNode : doc.child("pokeballs").children()) {

		uint16_t id = 0;
		PokeballProperties properties;
		if (auto itemId = stageNode.attribute("id")) {
			id = pugi::cast<uint16_t>(itemId.value());
		}

		if (id == 0) continue;

		if (auto name = stageNode.attribute("name")) {
			properties.name = pugi::cast<std::string>(name.value());
		}

		if (auto pokeballId = stageNode.attribute("pokeballId")) {
			properties.pokeballId = pugi::cast<uint16_t>(pokeballId.value());
		}

		if (auto goback = stageNode.attribute("goback")) {
			properties.gobackEffect = pugi::cast<uint16_t>(goback.value());
		}

		if (auto throwEffect = stageNode.attribute("missile")) {
			properties.throwEffect = pugi::cast<uint16_t>(throwEffect.value());
		}

		if (auto success = stageNode.attribute("success")) {
			properties.catchSuccessEffect = pugi::cast<uint16_t>(success.value());
		}

		if (auto fail = stageNode.attribute("fail")) {
			properties.catchFailEffect = pugi::cast<uint16_t>(fail.value());
		}

		if (auto rate = stageNode.attribute("rate")) {
			properties.catchRate = pugi::cast<float>(rate.value());
		}

		pokeballData[id] = properties;
	}

	return true;
}

// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "game.h"
#include "moves.h"
#include "player.h"
#include "pokeball.h"
#include "pokemon.h"

extern Game g_game;
extern Moves* g_moves;
extern Pokemons g_pokemons;

Moves::Moves()
{
	scriptInterface.initState();
}

Moves::~Moves()
{
	clear(false);
}

TalkActionResult_t Moves::playerSayMove(Player* player, std::string& words)
{
	trimString(words);
	const PokemonMoveType* pokemonMove = g_pokemons.getMoveByName(words);
	if (!pokemonMove) {
		return TALKACTION_CONTINUE;
	}

	Pokeball* pokeball = player->getActivePokemon();
	Pokemon* pokemon = pokeball ? pokeball->getPokemon() : nullptr;
	if (!pokemon) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, "You need an active Pokemon to use this move.");
		return TALKACTION_FAILED;
	}

	pokemon->refreshAvailableMoves();
	const auto& knownMoves = pokemon->getMoves();
	const auto stateIt = std::find_if(knownMoves.begin(), knownMoves.end(), [pokemonMove](const PokemonMoveState& state) {
		return state.moveId == pokemonMove->id;
	});
	if (stateIt == knownMoves.end()) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, "Your active Pokemon does not know this move.");
		return TALKACTION_FAILED;
	}
	if (stateIt->activeSlot == 0) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, "This move is not in an active slot.");
		return TALKACTION_FAILED;
	}

	PokemonMoveEffect* effect = g_moves->getMoveByName(pokemonMove->effect);
	Creature* target = player->getAttackedCreature();
	if (effect && effect->getNeedTarget() && !target) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, "Select a target before using a Pokemon move.");
		return TALKACTION_FAILED;
	}

	if (!pokemon->useMove(stateIt->activeSlot, target)) {
		player->sendTextMessage(MESSAGE_STATUS_SMALL, effect && effect->getNeedTarget() ?
			"The move is on cooldown or the target is out of range." :
			"The move is on cooldown or could not be used.");
		return TALKACTION_FAILED;
	}

	return TALKACTION_SILENT_BREAK;
}

void Moves::clear(bool fromLua)
{
	if (!fromLua) {
		effects.clear();
	}
	reInitState(fromLua);
}

Event_ptr Moves::getEvent(const std::string& nodeName)
{
	if (strcasecmp(nodeName.c_str(), "move") == 0) {
		return Event_ptr(new PokemonMoveEffect(&scriptInterface));
	}
	return nullptr;
}

bool Moves::registerEvent(Event_ptr event, const pugi::xml_node&)
{
	auto* effect = dynamic_cast<PokemonMoveEffect*>(event.get());
	if (!effect) {
		return false;
	}

	const std::string key = asLowerCaseString(effect->getName());
	auto result = effects.emplace(key, std::move(*effect));
	if (!result.second) {
		std::cout << "[Warning - Moves::registerEvent] Duplicate Pokemon move effect: " << effect->getName() << std::endl;
	}
	return result.second;
}

PokemonMoveEffect* Moves::getMoveByName(const std::string& name)
{
	const auto it = effects.find(asLowerCaseString(name));
	return it != effects.end() ? &it->second : nullptr;
}

Position Moves::getCasterPosition(Creature* creature, Direction dir)
{
	return getNextPosition(dir, creature->getPosition());
}

bool PokemonMoveEffect::configureEvent(const pugi::xml_node& node)
{
	const pugi::xml_attribute nameAttribute = node.attribute("name");
	if (!nameAttribute) {
		std::cout << "[Error - PokemonMoveEffect::configureEvent] Effect without name." << std::endl;
		return false;
	}

	name = nameAttribute.as_string();
	needTarget = node.attribute("needtarget").as_bool() ||
		asLowerCaseString(node.attribute("targetmode").as_string()) == "target";
	needDirection = node.attribute("direction").as_bool();
	return true;
}

bool PokemonMoveEffect::castMove(Creature* creature)
{
	LuaVariant var;
	var.type = VARIANT_POSITION;
	var.pos = needDirection ? Moves::getCasterPosition(creature, creature->getDirection()) : creature->getPosition();
	return executeCastMove(creature, var);
}

bool PokemonMoveEffect::castMove(Creature* creature, Creature* target)
{
	if (!needTarget) {
		return castMove(creature);
	}

	LuaVariant var;
	var.type = VARIANT_NUMBER;
	var.number = target->getID();
	return executeCastMove(creature, var);
}

bool PokemonMoveEffect::executeCastMove(Creature* creature, const LuaVariant& var)
{
	if (!scriptInterface->reserveScriptEnv()) {
		std::cout << "[Error - PokemonMoveEffect::executeCastMove] Call stack overflow." << std::endl;
		return false;
	}

	ScriptEnvironment* env = scriptInterface->getScriptEnv();
	env->setScriptId(scriptId, scriptInterface);
	lua_State* L = scriptInterface->getLuaState();
	scriptInterface->pushFunction(scriptId);
	LuaScriptInterface::pushUserdata<Creature>(L, creature);
	LuaScriptInterface::setCreatureMetatable(L, -1, creature);
	LuaScriptInterface::pushVariant(L, var);
	return scriptInterface->callFunction(2);
}

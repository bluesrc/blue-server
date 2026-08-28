// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#ifndef FS_MOVES_H_D78A7CCB7080406E8CAA6B1D31D3DA71
#define FS_MOVES_H_D78A7CCB7080406E8CAA6B1D31D3DA71

#include "baseevents.h"
#include "luascript.h"
#include "talkaction.h"

class Player;

class PokemonMoveEffect final : public Event
{
	public:
		explicit PokemonMoveEffect(LuaScriptInterface* interface) : Event(interface) {}

		bool configureEvent(const pugi::xml_node& node) override;
		bool castMove(Creature* creature);
		bool castMove(Creature* creature, Creature* target);

		const std::string& getName() const { return name; }
		bool getNeedTarget() const { return needTarget; }

	private:
		std::string getScriptEventName() const override { return "onCastMove"; }
		bool executeCastMove(Creature* creature, const LuaVariant& var);

		std::string name;
		bool needTarget = false;
		bool needDirection = false;
};

class Moves final : public BaseEvents
{
	public:
		Moves();
		~Moves();

		Moves(const Moves&) = delete;
		Moves& operator=(const Moves&) = delete;

		PokemonMoveEffect* getMoveByName(const std::string& name);
		TalkActionResult_t playerSayMove(Player* player, std::string& words);

		static Position getCasterPosition(Creature* creature, Direction dir);
		std::string getScriptBaseName() const override { return "moves"; }
		void clear(bool fromLua) override final;

	private:
		LuaScriptInterface& getScriptInterface() override { return scriptInterface; }
		Event_ptr getEvent(const std::string& nodeName) override;
		bool registerEvent(Event_ptr event, const pugi::xml_node& node) override;

		std::map<std::string, PokemonMoveEffect> effects;
		LuaScriptInterface scriptInterface { "Pokemon Move Interface" };

};

#endif

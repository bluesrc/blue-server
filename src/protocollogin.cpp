// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "protocollogin.h"

#include "outputmessage.h"
#include "tasks.h"

#include "configmanager.h"
#include "iologindata.h"
#include "ban.h"
#include "game.h"

#include <fmt/format.h>

extern ConfigManager g_config;
extern Game g_game;

namespace {
	constexpr uint32_t LOGIN_CUSTOM_ACTION_MAGIC = 0x42524347;
	constexpr uint8_t LOGIN_ACTION_CREATE_ACCOUNT = 1;
	constexpr uint8_t LOGIN_ACTION_CREATE_CHARACTER = 2;
	constexpr uint8_t LOGIN_ACTION_CREATE_CHARACTER_WITH_WORLD = 3;
	constexpr uint8_t LOGIN_SERVER_CREATION_RESULT = 0x66;
	constexpr uint8_t LOGIN_SERVER_RICH_CHARACTER_LIST = 0x67;
	std::unordered_map<uint32_t, time_t> accountCreationAttempts;
}

void ProtocolLogin::disconnectClient(const std::string& message, uint16_t version)
{
	auto output = OutputMessagePool::getOutputMessage();

	output->addByte(version >= 1076 ? 0x0B : 0x0A);
	output->addString(message);
	send(output);

	disconnect();
}

void ProtocolLogin::sendCreationResult(uint8_t action, bool success, const std::string& message)
{
	auto output = OutputMessagePool::getOutputMessage();
	output->addByte(LOGIN_SERVER_CREATION_RESULT);
	output->addByte(action);
	output->addByte(success ? 1 : 0);
	output->addString(message);
	send(output);
	disconnect();
}

void ProtocolLogin::createAccount(const std::string& accountName, const std::string& password, uint32_t clientIp)
{
	if (!g_config.getBoolean(ConfigManager::ENABLE_CLIENT_ACCOUNT_CREATION)) {
		sendCreationResult(LOGIN_ACTION_CREATE_ACCOUNT, false, "Account creation is currently disabled.");
		return;
	}

	const time_t now = time(nullptr);
	const time_t cooldown = std::max<int32_t>(0, g_config.getNumber(ConfigManager::ACCOUNT_CREATION_COOLDOWN));
	auto attempt = accountCreationAttempts.find(clientIp);
	if (attempt != accountCreationAttempts.end() && now - attempt->second < cooldown) {
		sendCreationResult(LOGIN_ACTION_CREATE_ACCOUNT, false, "Please wait before creating another account.");
		return;
	}
	std::string message;
	const bool success = IOLoginData::createAccount(accountName, password, message);
	if (success) {
		accountCreationAttempts[clientIp] = now;
		if (accountCreationAttempts.size() > 10000) {
			for (auto it = accountCreationAttempts.begin(); it != accountCreationAttempts.end();) {
				if (now - it->second > std::max<time_t>(cooldown, 3600)) {
					it = accountCreationAttempts.erase(it);
				} else {
					++it;
				}
			}
		}
	}
	sendCreationResult(LOGIN_ACTION_CREATE_ACCOUNT, success, message);
}

void ProtocolLogin::createCharacter(const std::string& accountName, const std::string& password,
	const std::string& token, std::string characterName, uint8_t sex, const std::string& worldName)
{
	if (!g_config.getBoolean(ConfigManager::ENABLE_CLIENT_CHARACTER_CREATION)) {
		sendCreationResult(LOGIN_ACTION_CREATE_CHARACTER, false, "Character creation is currently disabled.");
		return;
	}
	if (worldName != g_config.getString(ConfigManager::SERVER_NAME)) {
		sendCreationResult(LOGIN_ACTION_CREATE_CHARACTER, false, "Selected world is not available.");
		return;
	}

	Account account;
	if (!IOLoginData::loginserverAuthentication(accountName, password, account)) {
		sendCreationResult(LOGIN_ACTION_CREATE_CHARACTER, false, "Account name or password is not correct.");
		return;
	}
	if (!account.key.empty()) {
		const uint32_t ticks = time(nullptr) / AUTHENTICATOR_PERIOD;
		if (token.empty() || !(token == generateToken(account.key, ticks) || token == generateToken(account.key, ticks - 1) || token == generateToken(account.key, ticks + 1))) {
			sendCreationResult(LOGIN_ACTION_CREATE_CHARACTER, false, "Invalid authenticator token.");
			return;
		}
	}

	std::string message;
	const uint32_t maxCharacters = std::max<int32_t>(1, g_config.getNumber(ConfigManager::MAX_CHARACTERS_PER_ACCOUNT));
	const bool success = IOLoginData::createCharacter(account.id, std::move(characterName), static_cast<PlayerSex_t>(sex), maxCharacters, message);
	sendCreationResult(LOGIN_ACTION_CREATE_CHARACTER, success, message);
}

void ProtocolLogin::getCharacterList(const std::string& accountName, const std::string& password, const std::string& token,
	uint16_t version, bool richCharacterList)
{
	Account account;
	if (!IOLoginData::loginserverAuthentication(accountName, password, account, richCharacterList)) {
		disconnectClient("Account name or password is not correct.", version);
		return;
	}

	uint32_t ticks = time(nullptr) / AUTHENTICATOR_PERIOD;

	auto output = OutputMessagePool::getOutputMessage();
	if (!account.key.empty()) {
		if (token.empty() || !(token == generateToken(account.key, ticks) || token == generateToken(account.key, ticks - 1) || token == generateToken(account.key, ticks + 1))) {
			output->addByte(0x0D);
			output->addByte(0);
			send(output);
			disconnect();
			return;
		}
		output->addByte(0x0C);
		output->addByte(0);
	}

	const std::string& motd = g_config.getString(ConfigManager::MOTD);
	if (!motd.empty()) {
		//Add MOTD
		output->addByte(0x14);
		output->addString(fmt::format("{:d}\n{:s}", g_game.getMotdNum(), motd));
	}

	//Add session key
	output->addByte(0x28);
	output->addString(accountName + "\n" + password + "\n" + token + "\n" + std::to_string(ticks));

	//Add char list
	output->addByte(richCharacterList ? LOGIN_SERVER_RICH_CHARACTER_LIST : 0x64);

	uint8_t size = std::min<size_t>(std::numeric_limits<uint8_t>::max(), account.characters.size());

	if (g_config.getBoolean(ConfigManager::ONLINE_OFFLINE_CHARLIST)) {
		output->addByte(2); // number of worlds

		for (uint8_t i = 0; i < 2; i++) {
			output->addByte(i); // world id
			output->addString(i == 0 ? "Offline" : "Online");
			output->addString(g_config.getString(ConfigManager::IP));
			output->add<uint16_t>(g_config.getNumber(ConfigManager::GAME_PORT));
			output->addByte(0);
		}
	} else {
		output->addByte(1); // number of worlds
		output->addByte(0); // world id
		output->addString(g_config.getString(ConfigManager::SERVER_NAME));
		output->addString(g_config.getString(ConfigManager::IP));
		output->add<uint16_t>(g_config.getNumber(ConfigManager::GAME_PORT));
		output->addByte(0);
	}

	output->addByte(size);
	for (uint8_t i = 0; i < size; i++) {
		const AccountCharacter& character = account.characters[i];
		if (g_config.getBoolean(ConfigManager::ONLINE_OFFLINE_CHARLIST)) {
			output->addByte(g_game.getPlayerByName(character.name) ? 1 : 0);
		} else {
			output->addByte(0);
		}
		output->addString(character.name);
		if (richCharacterList) {
			output->add<uint32_t>(character.level);
			output->add<uint16_t>(character.lookType);
			output->addByte(character.lookHead);
			output->addByte(character.lookBody);
			output->addByte(character.lookLegs);
			output->addByte(character.lookFeet);
			output->addByte(character.lookAddons);
			for (uint16_t pokemonNumber : character.pokemonNumbers) {
				output->add<uint16_t>(pokemonNumber);
			}
		}
	}

	//Add premium days
	output->addByte(0);
	if (g_config.getBoolean(ConfigManager::FREE_PREMIUM)) {
		output->addByte(1);
		output->add<uint32_t>(0);
	} else {
		output->addByte(account.premiumEndsAt > time(nullptr) ? 1 : 0);
		output->add<uint32_t>(account.premiumEndsAt);
	}

	send(output);

	disconnect();
}

void ProtocolLogin::onRecvFirstMessage(NetworkMessage& msg)
{
	if (g_game.getGameState() == GAME_STATE_SHUTDOWN) {
		disconnect();
		return;
	}

	msg.skipBytes(2); // client OS

	uint16_t version = msg.get<uint16_t>();
	if (version >= 971) {
		msg.skipBytes(17);
	} else {
		msg.skipBytes(12);
	}
	/*
	 * Skipped bytes:
	 * 4 bytes: protocolVersion
	 * 12 bytes: dat, spr, pic signatures (4 bytes each)
	 * 1 byte: 0
	 */

	if (version <= 760) {
		disconnectClient(fmt::format("Only clients with protocol {:s} allowed!", CLIENT_VERSION_STR), version);
		return;
	}

	if (!Protocol::RSA_decrypt(msg)) {
		disconnect();
		return;
	}

	xtea::key key;
	key[0] = msg.get<uint32_t>();
	key[1] = msg.get<uint32_t>();
	key[2] = msg.get<uint32_t>();
	key[3] = msg.get<uint32_t>();
	enableXTEAEncryption();
	setXTEAKey(std::move(key));

	if (version < CLIENT_VERSION_MIN || version > CLIENT_VERSION_MAX) {
		disconnectClient(fmt::format("Only clients with protocol {:s} allowed!", CLIENT_VERSION_STR), version);
		return;
	}

	if (g_game.getGameState() == GAME_STATE_STARTUP) {
		disconnectClient("Gameworld is starting up. Please wait.", version);
		return;
	}

	if (g_game.getGameState() == GAME_STATE_MAINTAIN) {
		disconnectClient("Gameworld is under maintenance.\nPlease re-connect in a while.", version);
		return;
	}

	BanInfo banInfo;
	auto connection = getConnection();
	if (!connection) {
		return;
	}

	if (IOBan::isIpBanned(connection->getIP(), banInfo)) {
		if (banInfo.reason.empty()) {
			banInfo.reason = "(none)";
		}

		disconnectClient(fmt::format("Your IP has been banned until {:s} by {:s}.\n\nReason specified:\n{:s}", formatDateShort(banInfo.expiresAt), banInfo.bannedBy, banInfo.reason), version);
		return;
	}

	std::string accountName = msg.getString();
	if (accountName.empty()) {
		disconnectClient("Invalid account name.", version);
		return;
	}

	std::string password = msg.getString();
	if (password.empty()) {
		disconnectClient("Invalid password.", version);
		return;
	}

	uint8_t loginAction = 0;
	bool richCharacterList = false;
	std::string characterName;
	uint8_t characterSex = PLAYERSEX_FEMALE;
	std::string characterWorld = g_config.getString(ConfigManager::SERVER_NAME);
	if (msg.get<uint32_t>() == LOGIN_CUSTOM_ACTION_MAGIC) {
		richCharacterList = true;
		loginAction = msg.getByte();
		if (loginAction == LOGIN_ACTION_CREATE_CHARACTER || loginAction == LOGIN_ACTION_CREATE_CHARACTER_WITH_WORLD) {
			characterName = msg.getString();
			characterSex = msg.getByte();
			if (loginAction == LOGIN_ACTION_CREATE_CHARACTER_WITH_WORLD) {
				characterWorld = msg.getString();
			}
		}
	}

	// read authenticator token and stay logged in flag from last 128 bytes
	msg.skipBytes((msg.getLength() - 128) - msg.getBufferPosition());
	if (!Protocol::RSA_decrypt(msg)) {
		disconnectClient("Invalid authentication token.", version);
		return;
	}

	std::string authToken = msg.getString();

	if (loginAction == LOGIN_ACTION_CREATE_ACCOUNT) {
		auto thisPtr = std::static_pointer_cast<ProtocolLogin>(shared_from_this());
		g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::createAccount, thisPtr, accountName, password, connection->getIP())));
		return;
	}
	if (loginAction == LOGIN_ACTION_CREATE_CHARACTER || loginAction == LOGIN_ACTION_CREATE_CHARACTER_WITH_WORLD) {
		auto thisPtr = std::static_pointer_cast<ProtocolLogin>(shared_from_this());
		g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::createCharacter, thisPtr, accountName, password,
			authToken, std::move(characterName), characterSex, std::move(characterWorld))));
		return;
	}

	auto thisPtr = std::static_pointer_cast<ProtocolLogin>(shared_from_this());
	g_dispatcher.addTask(createTask(std::bind(&ProtocolLogin::getCharacterList, thisPtr, accountName, password, authToken, version, richCharacterList)));
}

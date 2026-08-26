// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "depotchest.h"
#include "creature.h"
#include "tools.h"

namespace {
	const std::string BOX_SLOT_ATTRIBUTE = "box_slot";
	const std::string LEGACY_POKEMON_BOX_SLOT_ATTRIBUTE = "pokemon_box_slot";
}

DepotChest::DepotChest(uint16_t type, bool paginated /*= true*/, uint16_t depotId /*= NO_DEPOT_ID*/) :
	Container{type, items[type].maxItems, true, paginated}, depotId{depotId} {}

bool DepotChest::isItemAllowed(const Item& item) const
{
	if (depotId <= 4) {
		if (item.getPokeball() != nullptr) {
			return false;
		}

		if (const Container* container = item.getContainer()) {
			for (ContainerIterator it = container->iterator(); it.hasNext(); it.advance()) {
				if ((*it)->getPokeball() != nullptr) {
					return false;
				}
			}
		}

		return true;
	}

	if (isPokemonBox()) {
		return item.getPokeball() != nullptr;
	}

	return true;
}

bool DepotChest::canModify(const Creature* actor) const
{
	return depotId == NO_DEPOT_ID || actor == nullptr || actor->getZone() == ZONE_PROTECTION;
}

int32_t DepotChest::getBoxSlot(const Item& item) const
{
	if (!isPlayerBox()) {
		return -1;
	}

	const ItemAttributes::CustomAttribute* attribute = item.getCustomAttribute(BOX_SLOT_ATTRIBUTE);
	if (!attribute) {
		attribute = item.getCustomAttribute(LEGACY_POKEMON_BOX_SLOT_ATTRIBUTE);
	}
	if (!attribute) {
		return -1;
	}

	const int64_t* slot = boost::get<int64_t>(&attribute->value);
	if (!slot || *slot < 0 || *slot > std::numeric_limits<uint16_t>::max() ||
			!isValidBoxSlot(static_cast<int32_t>(*slot))) {
		return -1;
	}
	return static_cast<int32_t>(*slot);
}

Item* DepotChest::getItemByBoxSlot(int32_t slot) const
{
	if (!isValidBoxSlot(slot)) {
		return nullptr;
	}

	for (Item* item : itemlist) {
		if (getBoxSlot(*item) == slot) {
			return item;
		}
	}
	return nullptr;
}

void DepotChest::setBoxSlot(Item& item, int32_t slot) const
{
	item.removeCustomAttribute(LEGACY_POKEMON_BOX_SLOT_ATTRIBUTE);
	if (!isPlayerBox() || !isValidBoxSlot(slot)) {
		item.removeCustomAttribute(BOX_SLOT_ATTRIBUTE);
		return;
	}

	std::string attributeName = BOX_SLOT_ATTRIBUTE;
	item.setCustomAttribute(attributeName, static_cast<int64_t>(slot));
}

void DepotChest::normalizeBoxSlots()
{
	if (!isPlayerBox()) {
		return;
	}

	std::unordered_set<int32_t> usedSlots;
	std::vector<Item*> unassignedItems;
	for (Item* item : itemlist) {
		const int32_t slot = getBoxSlot(*item);
		if (slot >= 0 && usedSlots.emplace(slot).second) {
		} else {
			setBoxSlot(*item, -1);
			unassignedItems.push_back(item);
		}
	}

	int32_t freeSlot = 0;
	for (Item* item : unassignedItems) {
		while (isValidBoxSlot(freeSlot) && usedSlots.find(freeSlot) != usedSlots.end()) {
			++freeSlot;
		}
		if (!isValidBoxSlot(freeSlot)) {
			break;
		}

		setBoxSlot(*item, freeSlot);
		usedSlots.emplace(freeSlot);
	}
}

int32_t DepotChest::getNextBoxSlot()
{
	normalizeBoxSlots();
	std::unordered_set<int32_t> usedSlots;
	int32_t lastUsedSlot = -1;
	for (Item* item : itemlist) {
		const int32_t slot = getBoxSlot(*item);
		if (slot >= 0) {
			usedSlots.emplace(slot);
			lastUsedSlot = std::max(lastUsedSlot, slot);
		}
	}

	for (int32_t slot = lastUsedSlot + 1; isValidBoxSlot(slot); ++slot) {
		if (usedSlots.find(slot) == usedSlots.end()) {
			return slot;
		}
	}
	for (int32_t slot = 0; slot <= lastUsedSlot; ++slot) {
		if (usedSlots.find(slot) == usedSlots.end()) {
			return slot;
		}
	}
	return -1;
}

ReturnValue DepotChest::queryAdd(int32_t index, const Thing& thing, uint32_t count,
		uint32_t flags, Creature* actor/* = nullptr*/) const
{
	const Item* item = thing.getItem();
	if (item == nullptr) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	if (!canModify(actor)) {
		return RETURNVALUE_NOTPOSSIBLE;
	}

	if (!isItemAllowed(*item)) {
		return RETURNVALUE_ITEMCANNOTBEMOVEDTHERE;
	}

	bool skipLimit = hasBitSet(FLAG_NOLIMIT, flags);
	if (!skipLimit) {
		int32_t addCount = 0;

		if ((item->isStackable() && item->getItemCount() != count)) {
			addCount = 1;
		}

		if (item->getTopParent() != this) {
			if (const Container* container = item->getContainer()) {
				addCount = container->getItemHoldingCount() + 1;
			} else {
				addCount = 1;
			}
		}

		if (getItemHoldingCount() + addCount > maxDepotItems) {
			return RETURNVALUE_DEPOTISFULL;
		}
	}

	return Container::queryAdd(index, thing, count, flags, actor);
}

void DepotChest::postAddNotification(Thing* thing, const Cylinder* oldParent, int32_t index, cylinderlink_t)
{
	Cylinder* parent = getParent();
	if (parent != nullptr) {
		parent->postAddNotification(thing, oldParent, index, LINK_PARENT);
	}
}

void DepotChest::postRemoveNotification(Thing* thing, const Cylinder* newParent, int32_t index, cylinderlink_t)
{
	Cylinder* parent = getParent();
	if (parent != nullptr) {
		parent->postRemoveNotification(thing, newParent, index, LINK_PARENT);
	}
}

Cylinder* DepotChest::getParent() const
{
	if (parent) {
		return parent->getParent();
	}
	return nullptr;
}

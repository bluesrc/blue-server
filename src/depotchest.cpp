// Copyright 2022 The Forgotten Server Authors. All rights reserved.
// Use of this source code is governed by the GPL-2.0 License that can be found in the LICENSE file.

#include "otpch.h"

#include "depotchest.h"
#include "creature.h"
#include "tools.h"

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

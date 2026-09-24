#pragma once

#include "ChatEntryManager.h"
#include "imgui.h"
#include <algorithm>

#undef min
#undef max

ChatEntryManager::ChatEntryManager()
{
	entries.reserve(MAX_PAGESIZE);
}

void ChatEntryManager::observe(int entryId, const CRect& rect)
{
	for (auto& entry : entries)
	{
		if (entry.entryId != entryId) continue;
		entry.rect.x1 = std::min(entry.rect.x1, rect.x1);
		entry.rect.y1 = std::min(entry.rect.y1, rect.y1);
		entry.rect.x2 = std::max(entry.rect.x2, rect.x2);
		entry.rect.y2 = std::max(entry.rect.y2, rect.y2);
		return;
	}
	entries.push_back({rect, entryId});
}

int ChatEntryManager::getEntryIdByScreenCoords(
	int xPos,
	int yPos
) const
{
	for (const auto& entry : entries)
	{
		if (xPos >= static_cast<int>(entry.rect.x1) &&
			xPos < static_cast<int>(entry.rect.x2) &&
			yPos >= static_cast<int>(entry.rect.y1) &&
			yPos < static_cast<int>(entry.rect.y2))
		{
			return entry.entryId;
		}
	}

	return -1;
}

ChatEntryPosition* ChatEntryManager::getByEntryId(int entryId)
{
	for (auto& entry : entries)
	{
		if (entry.entryId == entryId)
			return &entry;
	}
	return nullptr;
}

void ChatEntryManager::clear()
{
	entries.clear();
}

#pragma once

#include "ChatEntryManager.h"
#include "imgui.h"

ChatEntryManager::ChatEntryManager()
{
	entries.reserve(MAX_PAGESIZE);
}

void ChatEntryManager::push(int entryId, int screenIndex, const CRect& rect)
{
	if (screenIndex < 0)
		return;

	if (entries.size() <= static_cast<size_t>(screenIndex))
		entries.resize(screenIndex + 1);

	entries[screenIndex] = { rect, entryId, screenIndex };
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

void ChatEntryManager::setChatPointer(CChat* ptr)
{
	if (pChat != nullptr) return;
	pChat = ptr;
}

CChat* ChatEntryManager::getChatPointer()
{
	return pChat;
}
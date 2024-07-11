#pragma once

#include "ChatEntryManager.h"
#include "imgui.h"

ChatEntryManager::ChatEntryManager()
{
	entries.reserve(MAX_PAGESIZE);
}

void ChatEntryManager::setChatPointer(CChat* pChat)
{
	this->pChat = pChat;
}

void ChatEntryManager::push(CRect rect)
{
	if (pChat == nullptr) return;
	
	const uint32_t endLineRenderIndex = this->getLastLineIndex();

	if (curLineRenderIndex < 0) {
		entries.clear();
		curLineRenderIndex = static_cast<int>(this->getCurrentScrollbarPos());
		while (curLineRenderIndex < 99 && pChat->m_entry[curLineRenderIndex].m_szText[0] == '\0') curLineRenderIndex++;
	}

	// TODO: Fix calculations according to possible chat movement

	// Change string clickable area width
	static const auto& io = ImGui::GetIO();
	const auto x2 = static_cast<int>(static_cast<double>(io.DisplaySize.x) * (1024.0 / 1920));
	rect.x2 = x2;

	// Indent for timestamp
	if (pChat->m_bTimestamp) rect.x1 -= pChat->m_nTimestampWidth + static_cast<int>(static_cast<double>(io.DisplaySize.x) * (5.0 / 1920));

	// Correct Y
	auto& position = get(curLineRenderIndex - 1);
	position.rect.y2 = rect.y1;
	
	entries.push_back({ rect, curLineRenderIndex });
	curLineRenderIndex++;

	if (static_cast<uint32_t>(curLineRenderIndex) > endLineRenderIndex) curLineRenderIndex = -1;
}

ChatEntryPosition& ChatEntryManager::get(size_t lineIndex) {
	static ChatEntryPosition dummyPosition{ CRect{20000, 0, 0, 0}, -1 };

	int i = static_cast<int>(entries.size()) - 1;
	while (i >= 0) {
		ChatEntryPosition& entry = entries[i];
		if (static_cast<size_t>(entry.index) == lineIndex) {
			return entry;
		}
		i--;
	}

	return dummyPosition;
}

int ChatEntryManager::getLineIndexInScreenCoords(const size_t& xPos, const size_t& yPos) const
{
	if (pChat == nullptr) return -1;

	// Skip, if chat is empty
	if (pChat->m_entry[getLastLineIndex()].m_szText[0] == '\0') return -1;

	for (const auto& entry : entries) {
		if (xPos >= entry.rect.x1 && xPos <= entry.rect.x2 && yPos >= entry.rect.y1 && yPos <= entry.rect.y2) {
			return entry.index;
		}
	}
	
	return -1;
}

uint32_t ChatEntryManager::getCurrentScrollbarPos() const
{
	if (pChat == nullptr) return 0;
	return *reinterpret_cast<uint32_t*>(pChat->m_pScrollbar + 142);
}

int ChatEntryManager::getLastLineIndex() const
{
	const auto& pChat = this->pChat;
	if (pChat == nullptr) return 99;
	return int(getCurrentScrollbarPos() + pChat->m_nPageSize) - 1;
}
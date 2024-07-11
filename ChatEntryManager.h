#ifndef CHAT_ENTRY_MANAGER_H
#define CHAT_ENTRY_MANAGER_H

#define MAX_PAGESIZE 64
#include "GameStructures.h"
#include <vector>

struct ChatEntryPosition {
	CRect rect;
	int index;

	ChatEntryPosition()
	{
		CRect rect;
		rect.x1 = -1;
		rect.y1 = -1;
		rect.x2 = -1;
		rect.y2 = -1;
		this->rect = rect;

		index = -1;
	}

	ChatEntryPosition(const CRect rect, const int index)
	{
		this->rect = rect;
		this->index = index;
	}
};

class ChatEntryManager
{
private:
	CChat*                              pChat = nullptr;
	int                                 curLineRenderIndex = -1;
	std::vector<ChatEntryPosition>      entries;

public:
	ChatEntryManager();

	void                                push(CRect rect);
	ChatEntryPosition&                  get(size_t lineIndex);

	int                                 getCurrentLineRenderIndex() { return curLineRenderIndex; }

	void                                setChatPointer(CChat* pChat);
	[[nodiscard]] int                   getLastLineIndex() const;
	[[nodiscard]] int                   getLineIndexInScreenCoords(const size_t& xPos, const size_t& yPos) const;
	[[nodiscard]] uint32_t              getCurrentScrollbarPos() const;
};

#endif

#ifndef CHAT_ENTRY_MANAGER_H
#define CHAT_ENTRY_MANAGER_H

#define MAX_PAGESIZE 64

#include "GameStructures.h"
#include <vector>

struct ChatEntryPosition
{
    CRect rect{};
    int   entryId;     // absolute row ID
    int   screenIndex; // screen ID
};

class ChatEntryManager
{
private:
    CChat* pChat = nullptr;
    std::vector<ChatEntryPosition> entries;

public:
    ChatEntryManager();

    void clear();
    void push(int entryId, int screenIndex, const CRect& rect);

    [[nodiscard]]
    int getEntryIdByScreenCoords(int xPos, int yPos) const;

    void setChatPointer(CChat* ptr);
    CChat* getChatPointer();
    ChatEntryPosition* getByEntryId(int entryId);
};

#endif // CHAT_ENTRY_MANAGER_H

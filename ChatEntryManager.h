#ifndef CHAT_ENTRY_MANAGER_H
#define CHAT_ENTRY_MANAGER_H

#define MAX_PAGESIZE 64

#include "GameStructures.h"
#include <vector>

struct ChatEntryPosition
{
    CRect rect{};
    int   entryId;     // absolute row ID
};

class ChatEntryManager
{
private:
    std::vector<ChatEntryPosition> entries;

public:
    ChatEntryManager();

    void clear();
    void translate(int dx, int dy);
    void observe(int entryId, const CRect& rect);

    [[nodiscard]]
    int getEntryIdByScreenCoords(int xPos, int yPos) const;

    ChatEntryPosition* getByEntryId(int entryId);
};

#endif // CHAT_ENTRY_MANAGER_H

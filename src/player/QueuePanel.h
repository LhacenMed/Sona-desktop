#pragma once

#include "widgets/Stack.h"

class Queue;
struct BarContext;

// Figma "Queue" panel (463:831; the "Controls" queue, bookmarks and chapters variants): tabs over the
// queue, the playing media's bookmarks and its chapters. A click on a row plays it or jumps to it
// and selects it, − or Delete removes the selected row, and queue rows drag to reorder.
class QueuePanel : public Column
{
public:
    static constexpr float width = 400;

    QueuePanel(const BarContext &context, Queue &queue);
};

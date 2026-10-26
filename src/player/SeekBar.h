#pragma once

#include <vector>

#include "core/Signal.h"
#include "widgets/View.h"

class Bookmarks;
class Player;

// Figma seek track (1908:7387): elapsed fill and handle over a dark track. Click or drag to seek.
// The playing media's bookmarks are marked on it as the subtitle timeline marks cues (3744:12361):
// hovering a mark names it, clicking it goes there.
class SeekBar : public View
{
public:
    SeekBar(Player &player, Bookmarks &bookmarks);

    bool takesMouse() const override;
    Cursor cursor() const override { return Cursor::Hand; }
    void layout() override;
    void paint(Canvas &canvas) override;
    // The handle goes over the marks.
    void paintTree(Canvas &canvas) override;
    void mousePressed(Point point) override;
    void mouseDragged(Point point) override;

private:
    class Mark;

    float progress() const;
    // Where the handle's left edge is at `progress` (0–1).
    float handleX(float progress) const;
    void seekTo(float x);
    void updateMarks();

    Player &m_player;
    Bookmarks &m_bookmarks;
    std::vector<Mark *> m_marks;
    // The duration the marks were placed for.
    double m_markedDuration = 0;
    // Where the pointer holds the handle while pressed, ahead of libVLC catching up.
    float m_pressedProgress = 0;
    Subscription m_bookmarksSubscription;
    Subscription m_playerSubscription;
};

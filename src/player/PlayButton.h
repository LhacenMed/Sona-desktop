#pragma once

#include "widgets/View.h"

class Player;

// Figma "play" (848:1712): shows "play" while paused and "pause" while playing.
class PlayButton : public View
{
public:
    explicit PlayButton(Player &player);

    bool takesMouse() const override { return true; }
    bool showsHandCursor() const override { return true; }
    void paint(Canvas &canvas) override;
    void mouseReleased(Point point, bool inside) override;

private:
    Player &m_player;
};

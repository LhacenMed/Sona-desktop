#pragma once

#include "widgets/View.h"

class Player;

// Figma seek track (1908:7387): elapsed fill and handle over a dark track. Click or drag to seek.
class SeekBar : public View
{
public:
    explicit SeekBar(Player &player);

    bool takesMouse() const override;
    bool showsHandCursor() const override { return true; }
    void paint(Canvas &canvas) override;
    void mousePressed(Point point) override;
    void mouseDragged(Point point) override;

private:
    float progress() const;
    void seekTo(float x);

    Player &m_player;
    // Where the pointer holds the handle while pressed, ahead of libVLC catching up.
    float m_pressedProgress = 0;
};

#pragma once

#include "widgets/View.h"

class Player;

// Figma "volume" (244:184): a wedge whose gradient fill is revealed up to the current level (0–200%).
class VolumeBar : public View
{
public:
    explicit VolumeBar(Player &player);

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mousePressed(Point point) override;
    void mouseDragged(Point point) override;

private:
    void setVolumeAt(float x);

    Player &m_player;
};

#pragma once

#include "widgets/View.h"

class Player;

// Figma "volume" (244:184): a wedge whose gradient fill is revealed up to the current level (0–200%).
// While it's dragged, the level shows above it (Figma "audio value" 2245:9805).
class VolumeBar : public View
{
public:
    explicit VolumeBar(Player &player);

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }
    void paint(Canvas &canvas) override;
    void mousePressed(Point point) override;
    void mouseDragged(Point point) override;
    void mouseReleased(Point point, bool inside) override;

private:
    void setVolumeAt(float x);

    Player &m_player;
};

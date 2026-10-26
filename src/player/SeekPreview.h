#pragma once

#include "core/Signal.h"
#include "widgets/View.h"

class Player;

// Figma "thumbnail" (2151:9673): the picture at a moment of the media, with its time, on the seek
// bar above where the pointer is (Player::previewAt). Until the picture arrives, its box is empty.
class SeekPreview : public View
{
public:
    explicit SeekPreview(Player &player);

    void show(double seconds);

    void paint(Canvas &canvas) override;

private:
    Player &m_player;
    double m_seconds = 0;
    Subscription m_previewSubscription;
};

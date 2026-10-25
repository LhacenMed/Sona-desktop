#pragma once

#include "core/Signal.h"
#include "widgets/View.h"

class Player;

// The video viewport. While a video plays it leaves its pixels clear, so the video window below
// shows through. Its frame reproduces Figma 6586:17371: a 20px centre-aligned stroke with a 19px
// radius, whose outer half runs under the bars and whose inner half rounds the video's corners.
class VideoView : public View
{
public:
    explicit VideoView(Player &player);

    void paint(Canvas &canvas) override;

private:
    Player &m_player;
    Subscription m_playerSubscription;
};

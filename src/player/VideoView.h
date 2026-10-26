#pragma once

#include "core/Signal.h"
#include "widgets/View.h"

class Player;

// The video viewport. While a video plays it leaves its pixels clear, so the video window below
// shows through. Its frame reproduces Figma 6586:17371: a 20px centre-aligned stroke with a 19px
// radius around the part of the video the bars leave visible, so the video's corners stay rounded
// against the bars as they slide; its outer half runs under the bars.
class VideoView : public View
{
public:
    explicit VideoView(Player &player);

    // The visible part, in this view's coordinates; the whole view until set.
    void setVisibleArea(const Rect &area) { m_visibleArea = area; }

    void paint(Canvas &canvas) override;

private:
    Player &m_player;
    Rect m_visibleArea;
    Subscription m_playerSubscription;
};

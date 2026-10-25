#pragma once

#include "core/Signal.h"
#include "widgets/Row.h"

class IconButton;
class Player;

// Figma "Controls" (491:1779): transport, seek, volume and panel buttons.
class ControlBar : public Row
{
public:
    explicit ControlBar(Player &player);

    void paint(Canvas &canvas) override;

private:
    const wchar_t *volumeIcon() const;

    Player &m_player;
    IconButton *m_volumeButton;
    Subscription m_playerSubscription;
};

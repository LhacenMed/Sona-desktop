#pragma once

#include "core/Signal.h"
#include "platform/Timer.h"
#include "player/AutoHideBar.h"
#include "player/BarItems.h"
#include "widgets/View.h"

class ControlBar;
class TitleBar;
class VideoView;

// Figma "Player" (247:445) and "Player - hide UI" (3328:11554): the video inside its rounded frame,
// with the title bar above and the control bar below. The bars either stay fixed, with the video
// between them, or auto-hide (Hide UI): the video fills the window and each bar slides over it on its
// own edge (AutoHideBar), staying in while there's no video to watch. Neither moves while a press or
// a popup is under way. The video's rounded frame follows the bars as they slide. Over playing video
// with the bars out, the pointer hides after a moment at rest.
class PlayerView : public View
{
public:
    explicit PlayerView(BarContext context);
    ~PlayerView() override;

    VideoView &video() const { return m_video; }
    TitleBar &titleBar() const { return m_titleBar; }
    bool barsAutoHide() const { return m_barsAutoHide; }
    void setBarsAutoHide(bool autoHide);
    // The pointer moved or left, or a press or popup that held the bars ended.
    void pointerChanged();

    void layout() override;
    void paint(Canvas &canvas) override;
    void paintTree(Canvas &canvas) override;

private:
    void updateBars();
    // The bars where their reveals put them for this frame, and the video's frame between them.
    void placeBars();

    BarContext m_context;
    VideoView &m_video;
    TitleBar &m_titleBar;
    ControlBar &m_controlBar;
    AutoHideBar m_titleReveal;
    AutoHideBar m_controlsReveal;
    bool m_barsAutoHide = false;
    Timer m_pointerRest;
    Subscription m_playerSubscription;
};

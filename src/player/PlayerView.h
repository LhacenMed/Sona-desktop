#pragma once

#include <memory>

#include "core/Signal.h"
#include "platform/Timer.h"
#include "player/AutoHideBar.h"
#include "player/BarItems.h"
#include "widgets/View.h"

class ControlBar;
class Queue;
class TitleBar;
class VideoView;
enum class TrackKind;

// Figma "Player" (247:445) and "Player - hide UI" (3328:11554): the video inside its rounded frame,
// with the title bar above and the control bar below. The bars either stay fixed, with the video
// between them, or auto-hide (Hide UI): the video fills the window and each bar slides over it on its
// own edge (AutoHideBar), staying in while there's no video to watch. Neither moves while a press or
// a popup is under way. The video's rounded frame follows the bars as they slide. Over playing video
// with the bars out, the pointer hides after a moment at rest.
class PlayerView : public View
{
public:
    PlayerView(BarContext context, Queue &queue);
    ~PlayerView() override;

    const BarContext &barContext() const { return m_context; }
    VideoView &video() const { return m_video; }
    TitleBar &titleBar() const { return m_titleBar; }
    bool barsAutoHide() const { return m_barsAutoHide; }
    void setBarsAutoHide(bool autoHide);
    // The pointer moved or left, or a press or popup that held the bars ended.
    void pointerChanged();
    // The panels over the video above the control bar's right end, one at a time: the queue
    // (QueuePanel) and the tracks (TracksPanel), open at a kind's tab.
    bool isQueueOpen() const;
    void toggleQueue();
    bool isTracksOpen(TrackKind tab) const;
    // Opens the tracks at `tab`, shows `tab` if they're open at another, or closes them.
    void toggleTracks(TrackKind tab);

    void layout() override;
    void paint(Canvas &canvas) override;
    void paintTree(Canvas &canvas) override;

private:
    // The panel open over the video, if any.
    View *openPanel() const;
    // Opens `panel` in place of any popup, `height` tall at most.
    void showPanel(std::unique_ptr<View> panel, float width, float height);
    void updateBars();
    // The bars where their reveals put them for this frame, and the video's frame between them.
    void placeBars();

    BarContext m_context;
    Queue &m_queue;
    VideoView &m_video;
    TitleBar &m_titleBar;
    ControlBar &m_controlBar;
    AutoHideBar m_titleReveal;
    AutoHideBar m_controlsReveal;
    // Wakes when the first bar is due to leave; bars due together leave in the same pass.
    Timer m_barsLeave;
    bool m_barsAutoHide = false;
    Timer m_pointerRest;
    Subscription m_playerSubscription;
};

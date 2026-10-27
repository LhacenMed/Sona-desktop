#include "player/PlayerView.h"

#include <algorithm>

#include "graphics/Canvas.h"
#include "platform/PopupWindow.h"
#include "playback/Player.h"
#include "player/ControlBar.h"
#include "player/QueuePanel.h"
#include "player/TitleBar.h"
#include "player/VideoView.h"
#include "style/Surface.h"
#include "style/Theme.h"

namespace {

// Figma: the control bar is 54 tall; the video's frame is 10 wide on every side, and the video
// starts 1 below the title bar.
constexpr float controlBarHeight = 54;
constexpr float videoInset = 10;
constexpr float titleGap = 1;
// Panels sit 20 in from the window's right edge, 20 above the control bar and 20 below the title bar.
constexpr float panelMargin = 20;
// How long the pointer rests over playing video before it hides.
constexpr std::chrono::milliseconds pointerRestDuration{2000};

} // namespace

PlayerView::PlayerView(BarContext context, Queue &queue)
    : m_context(std::move(context))
    , m_queue(queue)
    , m_video(add<VideoView>(m_context.player))
    , m_titleBar(add<TitleBar>(m_context, defaultBarLayout))
    , m_controlBar(add<ControlBar>(m_context))
    , m_titleReveal(*this)
    , m_controlsReveal(*this)
    , m_barsLeave([this] { updateBars(); })
    , m_pointerRest([this] {
        if (m_titleReveal.reveal() == 0 && m_controlsReveal.reveal() == 0 && m_context.player.isPlaying())
            window()->hidePointer();
    })
{
    // A video starting or ending changes whether the bars must stay.
    m_playerSubscription = m_context.player.changed.connect([this] { updateBars(); });
}

PlayerView::~PlayerView() = default;

void PlayerView::setBarsAutoHide(bool autoHide)
{
    m_barsAutoHide = autoHide;
    layout();
    updateBars();
}

void PlayerView::pointerChanged()
{
    updateBars();
    if (window()->pointerPosition())
        m_pointerRest.start(pointerRestDuration, false);
    else
        m_pointerRest.stop();
}

bool PlayerView::isQueueOpen() const
{
    const Window *host = window();
    const auto *popup = host ? dynamic_cast<const PopupWindow *>(host->popup()) : nullptr;
    return popup && dynamic_cast<const QueuePanel *>(&popup->content());
}

void PlayerView::toggleQueue()
{
    Window &host = *window();
    const bool wasOpen = isQueueOpen();
    // Opening one panel closes any other popup.
    host.closePopup();
    if (wasOpen)
        return;
    const float top = m_titleBar.frame().height + panelMargin;
    const float height = std::max(0.0f, frame().height - controlBarHeight - panelMargin - top);
    const Rect panel = mapToWindow({frame().width - panelMargin - QueuePanel::width, top, QueuePanel::width, height});
    host.openWindow<PopupWindow>(host, panel, PopupPlacement::Docked, Surfaces::panel, std::make_unique<QueuePanel>(m_context, m_queue),
                                 PopupKeys::Passed);
}

void PlayerView::updateBars()
{
    Window *host = window();
    if (!host || host->isPressing() || host->hasPopup())
        return;
    if (!m_barsAutoHide || !m_context.player.hasVideo()) {
        m_titleReveal.show();
        m_controlsReveal.show();
    } else {
        std::optional<float> fromTop;
        std::optional<float> fromBottom;
        if (const auto pointer = host->pointerPosition()) {
            const Point local = mapFromWindow(*pointer);
            fromTop = local.y;
            fromBottom = frame().height - local.y;
        }
        const AutoHideBar::Clock::time_point now = AutoHideBar::Clock::now();
        m_titleReveal.follow(fromTop, m_titleBar.frame().height, now);
        m_controlsReveal.follow(fromBottom, controlBarHeight, now);
    }

    // One wake-up for the first bar due to leave.
    const auto title = m_titleReveal.leavesAt();
    const auto controls = m_controlsReveal.leavesAt();
    if (!title && !controls) {
        m_barsLeave.stop();
        return;
    }
    const auto due = title && controls ? std::min(*title, *controls) : title ? *title : *controls;
    const auto wait = std::chrono::ceil<std::chrono::milliseconds>(due - AutoHideBar::Clock::now());
    m_barsLeave.start(std::max(wait, std::chrono::milliseconds(1)), false);
}

void PlayerView::layout()
{
    const float width = frame().width;
    const float height = frame().height;
    // The title bar is as tall as Windows' caption buttons, which change with the window state.
    const float titleHeight = window() ? window()->captionHeight() : 0;
    // Under auto-hiding bars the video fills the window; beside fixed ones it's between them.
    const float videoTop = m_barsAutoHide ? videoInset : titleHeight + titleGap;
    const float videoBottom = height - (m_barsAutoHide ? videoInset : controlBarHeight);
    m_video.setFrame({videoInset, videoTop, width - 2 * videoInset, std::max(0.0f, videoBottom - videoTop)});
    m_titleBar.setFrame({0, 0, width, titleHeight});
    m_controlBar.setFrame({0, height - controlBarHeight, width, controlBarHeight});
    placeBars();
}

void PlayerView::placeBars()
{
    const float title = m_titleReveal.reveal();
    const float controls = m_controlsReveal.reveal();
    const float titleBottom = m_titleBar.frame().height * title;
    const float controlsTop = frame().height - controlBarHeight * controls;
    m_titleBar.moveTo({0, titleBottom - m_titleBar.frame().height});
    m_controlBar.moveTo({0, controlsTop});
    // Fully out, a bar takes no clicks.
    m_titleBar.setVisible(title > 0);
    m_controlBar.setVisible(controls > 0);

    // The video's rounded frame runs between the bars, so its corners stay rounded against them.
    const Rect &video = m_video.frame();
    const float top = std::max(video.y, titleBottom + titleGap);
    const float bottom = std::min(video.bottom(), controlsTop);
    m_video.setVisibleArea({0, top - video.y, video.width, std::max(0.0f, bottom - top)});
}

void PlayerView::paint(Canvas &canvas)
{
    canvas.fillRect(bounds(), canvas.palette().bgA3);
}

void PlayerView::paintTree(Canvas &canvas)
{
    placeBars();
    View::paintTree(canvas);
}

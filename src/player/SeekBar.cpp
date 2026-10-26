#include "player/SeekBar.h"

#include <algorithm>

#include "core/TimeText.h"
#include "data/Bookmarks.h"
#include "graphics/Canvas.h"
#include "playback/Player.h"
#include "style/Theme.h"

namespace {

constexpr float inset = 2;
constexpr float handleWidth = 8;
// A bookmark's mark: a line as tall as the fill, in a wider strip that takes the pointer.
constexpr float markWidth = 2;
constexpr float markHitWidth = 10;

} // namespace

class SeekBar::Mark : public View
{
public:
    Mark(Player &player, const Bookmark &bookmark)
        : m_player(player)
        , m_time(bookmark.time)
    {
        setTooltip(bookmark.name + L"  ·  " + clockText(bookmark.time));
    }

    double time() const { return m_time; }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        canvas.fillRoundedRect({(frame().width - markWidth) / 2, inset, markWidth, frame().height - 2 * inset}, 1,
                               Color::mix(palette.fgA2, palette.fg1, hoverAmount()));
    }

    void mouseReleased(Point, bool inside) override
    {
        if (inside)
            m_player.seek(m_time);
    }

private:
    Player &m_player;
    double m_time;
};

SeekBar::SeekBar(Player &player, Bookmarks &bookmarks)
    : m_player(player)
    , m_bookmarks(bookmarks)
{
    setPreferredSize({0, 28});
    m_bookmarksSubscription = bookmarks.changed.connect([this] { updateMarks(); });
    // Marks are placed by time, so they move once the duration is known.
    m_playerSubscription = player.changed.connect([this] {
        if (m_player.duration() != m_markedDuration)
            layout();
    });
    updateMarks();
}

bool SeekBar::takesMouse() const
{
    return m_player.duration() > 0;
}

float SeekBar::progress() const
{
    if (isPressed())
        return m_pressedProgress;
    return m_player.duration() > 0 ? static_cast<float>(m_player.position() / m_player.duration()) : 0;
}

float SeekBar::handleX(float progress) const
{
    return inset + progress * (frame().width - 2 * inset - handleWidth);
}

void SeekBar::updateMarks()
{
    for (Mark *mark : m_marks)
        remove(*mark);
    m_marks.clear();
    for (const Bookmark &bookmark : m_bookmarks.list())
        m_marks.push_back(&add<Mark>(m_player, bookmark));
    layout();
    invalidate();
}

void SeekBar::layout()
{
    m_markedDuration = m_player.duration();
    for (Mark *mark : m_marks) {
        mark->setVisible(m_markedDuration > 0);
        if (m_markedDuration <= 0)
            continue;
        const float center = handleX(static_cast<float>(std::clamp(mark->time() / m_markedDuration, 0.0, 1.0))) + handleWidth / 2;
        mark->setFrame({center - markHitWidth / 2, 0, markHitWidth, frame().height});
    }
}

void SeekBar::paint(Canvas &canvas)
{
    const Palette &palette = canvas.palette();
    const float handle = handleX(progress());
    canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
    canvas.fillRoundedRect({inset, inset, handle + handleWidth - inset, frame().height - 2 * inset}, 4, palette.bgAccentBright);
}

void SeekBar::paintTree(Canvas &canvas)
{
    View::paintTree(canvas);
    if (!isVisible())
        return;
    canvas.save();
    canvas.translate(frame().x, frame().y);
    canvas.fillRoundedRect({handleX(progress()), inset, handleWidth, frame().height - 2 * inset}, 4, canvas.palette().accentMain);
    canvas.restore();
}

void SeekBar::mousePressed(Point point)
{
    seekTo(point.x);
}

void SeekBar::mouseDragged(Point point)
{
    seekTo(point.x);
}

void SeekBar::seekTo(float x)
{
    const float travel = frame().width - 2 * inset - handleWidth;
    m_pressedProgress = std::clamp((x - inset - handleWidth / 2) / travel, 0.0f, 1.0f);
    m_player.seek(m_pressedProgress * m_player.duration());
}

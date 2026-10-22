#include "PlayerController.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <mpv/client.h>

#include <vector>

namespace {

// Observed mpv properties, identified by the reply id mpv echoes back in each change event.
enum class Observed : quint64 { Pause = 1, IdleActive, TimePos, Duration, Volume, Mute, MediaTitle, Path };

struct Observation
{
    Observed id;
    const char *name;
    mpv_format format;
};

constexpr Observation observations[] = {
    {Observed::Pause, "pause", MPV_FORMAT_FLAG},
    {Observed::IdleActive, "idle-active", MPV_FORMAT_FLAG},
    {Observed::TimePos, "time-pos", MPV_FORMAT_DOUBLE},
    {Observed::Duration, "duration", MPV_FORMAT_DOUBLE},
    {Observed::Volume, "volume", MPV_FORMAT_DOUBLE},
    {Observed::Mute, "mute", MPV_FORMAT_FLAG},
    {Observed::MediaTitle, "media-title", MPV_FORMAT_STRING},
    {Observed::Path, "path", MPV_FORMAT_STRING},
};

// An unavailable property (e.g. `duration` with nothing loaded) arrives as MPV_FORMAT_NONE.
bool flagValue(const mpv_event_property &property)
{
    return property.format == MPV_FORMAT_FLAG && *static_cast<int *>(property.data);
}

double doubleValue(const mpv_event_property &property)
{
    return property.format == MPV_FORMAT_DOUBLE ? *static_cast<double *>(property.data) : 0.0;
}

QString stringValue(const mpv_event_property &property)
{
    return property.format == MPV_FORMAT_STRING ? QString::fromUtf8(*static_cast<char **>(property.data)) : QString();
}

QString folderNameOf(const QString &path)
{
    const QUrl url = QUrl::fromUserInput(path);
    return url.isLocalFile() ? QFileInfo(url.toLocalFile()).absoluteDir().dirName() : QString();
}

template<typename T>
void assign(T &field, const T &value, PlayerController *owner, void (PlayerController::*changed)())
{
    if (field == value)
        return;
    field = value;
    emit (owner->*changed)();
}

} // namespace

PlayerController::PlayerController(QObject *parent)
    : QObject(parent)
    , m_mpv(mpv_create())
{
    mpv_set_option_string(m_mpv, "vo", "libmpv");
    mpv_set_option_string(m_mpv, "hwdec", "auto-safe");
    mpv_set_option_string(m_mpv, "keep-open", "yes");
    mpv_set_option_string(m_mpv, "volume-max", "200");
    const QByteArray snapshotFolder =
        (QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QStringLiteral("/Sona")).toUtf8();
    mpv_set_option_string(m_mpv, "screenshot-dir", snapshotFolder.constData());
    mpv_initialize(m_mpv);

    for (const Observation &observation : observations)
        mpv_observe_property(m_mpv, static_cast<quint64>(observation.id), observation.name, observation.format);

    mpv_set_wakeup_callback(m_mpv, [](void *context) {
        QMetaObject::invokeMethod(static_cast<PlayerController *>(context), &PlayerController::processEvents,
                                  Qt::QueuedConnection);
    }, this);
}

PlayerController::~PlayerController()
{
    mpv_set_wakeup_callback(m_mpv, nullptr, nullptr);
    mpv_terminate_destroy(m_mpv);
}

void PlayerController::setVolume(double volume)
{
    mpv_set_property_async(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE, &volume);
    int unmute = 0;
    mpv_set_property_async(m_mpv, 0, "mute", MPV_FORMAT_FLAG, &unmute);
}

void PlayerController::open(const QUrl &url)
{
    const QByteArray target = (url.isLocalFile() ? url.toLocalFile() : url.toString()).toUtf8();
    command({"loadfile", target.constData()});
    int resume = 0;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &resume);
}

void PlayerController::togglePause()
{
    command({"cycle", "pause"});
}

void PlayerController::toggleMute()
{
    command({"cycle", "mute"});
}

void PlayerController::seek(double seconds)
{
    const QByteArray target = QByteArray::number(seconds);
    command({"seek", target.constData(), "absolute"});
}

void PlayerController::seekBy(double seconds)
{
    const QByteArray offset = QByteArray::number(seconds);
    command({"seek", offset.constData(), "relative"});
}

void PlayerController::stepFrame(bool forward)
{
    command({forward ? "frame-step" : "frame-back-step"});
}

void PlayerController::cycleSubtitleTrack()
{
    command({"cycle", "sub"});
}

void PlayerController::cycleAudioTrack()
{
    command({"cycle", "audio"});
}

void PlayerController::takeSnapshot()
{
    command({"screenshot"});
}

void PlayerController::command(std::initializer_list<const char *> args)
{
    std::vector<const char *> argv(args);
    argv.push_back(nullptr);
    mpv_command_async(m_mpv, 0, argv.data());
}

void PlayerController::processEvents()
{
    while (true) {
        const mpv_event *event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE)
            return;
        if (event->event_id == MPV_EVENT_PROPERTY_CHANGE)
            applyPropertyChange(event->reply_userdata, *static_cast<mpv_event_property *>(event->data));
    }
}

void PlayerController::applyPropertyChange(quint64 id, const mpv_event_property &property)
{
    const bool wasPlaying = playing();

    switch (static_cast<Observed>(id)) {
    case Observed::Pause:
        m_paused = flagValue(property);
        break;
    case Observed::IdleActive:
        m_hasMedia = !flagValue(property);
        break;
    case Observed::TimePos:
        assign(m_position, doubleValue(property), this, &PlayerController::positionChanged);
        break;
    case Observed::Duration:
        assign(m_duration, doubleValue(property), this, &PlayerController::durationChanged);
        break;
    case Observed::Volume:
        assign(m_volume, doubleValue(property), this, &PlayerController::volumeChanged);
        break;
    case Observed::Mute:
        assign(m_muted, flagValue(property), this, &PlayerController::mutedChanged);
        break;
    case Observed::MediaTitle:
        assign(m_title, stringValue(property), this, &PlayerController::titleChanged);
        break;
    case Observed::Path:
        assign(m_folderName, folderNameOf(stringValue(property)), this, &PlayerController::folderNameChanged);
        break;
    }

    if (playing() != wasPlaying)
        emit playingChanged();
}

#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

struct mpv_handle;
struct mpv_event_property;

// Owns the libmpv core and mirrors its playback state to QML as the `Player` singleton.
// mpv is the single source of truth: every property here is updated only from mpv's
// property-change events, and every action is forwarded to mpv as a command.
class PlayerController : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Player)
    QML_SINGLETON

    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(double position READ position NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted NOTIFY mutedChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString folderName READ folderName NOTIFY folderNameChanged)

public:
    explicit PlayerController(QObject *parent = nullptr);
    ~PlayerController() override;

    mpv_handle *handle() const { return m_mpv; }

    bool playing() const { return m_hasMedia && !m_paused; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    double volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    QString title() const { return m_title; }
    QString folderName() const { return m_folderName; }

    void setVolume(double volume);

    Q_INVOKABLE void open(const QUrl &url);
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void seek(double seconds);
    Q_INVOKABLE void seekBy(double seconds);
    Q_INVOKABLE void stepFrame(bool forward);
    Q_INVOKABLE void cycleSubtitleTrack();
    Q_INVOKABLE void cycleAudioTrack();
    Q_INVOKABLE void takeSnapshot();

signals:
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void titleChanged();
    void folderNameChanged();

private:
    void processEvents();
    void applyPropertyChange(quint64 id, const mpv_event_property &property);
    void command(std::initializer_list<const char *> args);

    mpv_handle *m_mpv = nullptr;
    bool m_hasMedia = false;
    bool m_paused = false;
    double m_position = 0;
    double m_duration = 0;
    double m_volume = 100;
    bool m_muted = false;
    QString m_title;
    QString m_folderName;
};

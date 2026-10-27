#pragma once

#include <windows.h>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "core/Picture.h"
#include "core/Signal.h"
#include "platform/WorkerThread.h"

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_event_t;
struct libvlc_media_t;

struct Chapter
{
    std::wstring name;
    // Seconds from the start.
    double time;

    bool operator==(const Chapter &) const = default;
};

// The playback engine: owns libVLC and mirrors the player's state for the UI. libVLC is the single
// source of truth; every action is sent to it as a command, and the state below is re-read from it
// whenever it reports a change.
//
// libVLC loads on a worker thread and every command runs there, in order, so the window appears at
// once and the UI never waits on playback. It's also what keeps the app from deadlocking: libVLC
// tears its video window down synchronously, which needs the UI thread free to process messages.
// The state is read, and `changed` emitted, on the UI thread.
class Player
{
public:
    static constexpr int maxVolume = 200;

    Player();
    ~Player();
    Player(const Player &) = delete;
    Player &operator=(const Player &) = delete;

    bool isPlaying() const { return m_playing; }
    double position() const { return m_position; }
    double duration() const { return m_duration; }
    // 0–200%.
    int volume() const { return m_volume; }
    bool isMuted() const { return m_muted; }
    bool hasVideo() const { return m_hasVideo; }
    const std::wstring &title() const { return m_title; }
    // The playing media's artist, when it names one.
    const std::wstring &artist() const { return m_artist; }
    // The playing media's file path or URL; empty before anything opens.
    const std::wstring &location() const { return m_location; }
    // From open until the media plays: until then the state above is still the previous media's.
    bool isOpening() const { return m_opening; }
    const std::wstring &folderName() const { return m_folderName; }
    // The playing media's chapters, in order; empty for media without any.
    const std::vector<Chapter> &chapters() const { return m_chapters; }

    // The window libVLC draws video into.
    void setVideoWindow(HWND window);
    // Plays a file path or a URL, from `startTime` seconds in.
    void open(const std::wstring &location, double startTime = 0);
    void togglePause();
    void toggleMute();
    // Seeks exactly. Seeks made faster than libVLC takes them collapse into the latest.
    void seek(double seconds);
    void seekBy(double seconds);
    // The seek bar's preview of a moment: a second, silent player decodes it small, at the nearest
    // keyframe, which is quick even where an exact seek isn't. preparePreview opens the media in it
    // ahead (the pointer is on the seek bar), previewAt asks for a moment (the latest asked wins),
    // and previewChanged says its picture, preview(), arrived.
    void preparePreview();
    void previewAt(double seconds);
    // A picture of the moment, small.
    const Picture &preview() const { return m_preview; }
    void setVolume(int percent);
    void nextFrame();
    void cycleSubtitleTrack();
    void cycleAudioTrack();
    // Saves the current frame to Pictures\Sona.
    void takeSnapshot();
    // Stops playback and releases the video window, then runs `done` on the UI thread.
    void stop(std::function<void()> done);
    // Reads a media's duration in the background, without playing it, and gives it to `done` on the
    // UI thread; while a media is opening, it waits until it plays, so it never slows its start.
    // `done` isn't called when the duration can't be known that way.
    void readDuration(const std::wstring &location, std::function<void(double seconds)> done);

    Signal<> changed;
    Signal<> previewChanged;
    // The media played to its end.
    Signal<> ended;
    // The media couldn't be played.
    Signal<> failed;

private:
    struct DurationRequest;

    static void onEvent(const libvlc_event_t *event, void *player);
    // The preview player's picture: sized to fit the preview, then decoded into memory.
    static unsigned previewFormat(void **player, char *chroma, unsigned *width, unsigned *height, unsigned *pitches,
                                  unsigned *lines);
    static void *previewLock(void *player, void **planes);
    static void previewDisplay(void *player, void *picture);
    static void onParsed(const libvlc_event_t *event, void *request);

    void startEngine();
    void handleEvent(int type, int videoOutputCount);
    void sync();
    void applyAudio();
    void readChapters();
    libvlc_media_t *newMedia(const std::wstring &location) const;
    // Opens `location` in the preview player, paused at `seconds`, unless it's open there already.
    void openPreviewMedia(const std::wstring &location, double seconds);
    void closePreview();
    // Lets go of a duration request's media once it has been read, or when the engine stops.
    void releaseDurationRequest(DurationRequest *request);

    // Created and released on the engine thread.
    libvlc_instance_t *m_vlc = nullptr;
    std::atomic<libvlc_media_player_t *> m_player = nullptr;
    std::atomic<libvlc_media_player_t *> m_previewPlayer = nullptr;
    std::wstring m_previewMedia;
    // Decoded on libVLC's thread, then handed to the UI thread as m_preview.
    Picture m_previewDecoding;
    std::vector<std::unique_ptr<DurationRequest>> m_durationRequests;
    // Only the latest target of a seek or preview is sent: one command is queued at a time.
    std::atomic<long long> m_seekTarget = 0;
    std::atomic<bool> m_seekQueued = false;
    std::atomic<long long> m_previewTarget = 0;
    std::atomic<bool> m_previewQueued = false;

    bool m_playing = false;
    double m_position = 0;
    double m_duration = 0;
    int m_volume = 100;
    bool m_muted = false;
    bool m_hasVideo = false;
    std::wstring m_title;
    std::wstring m_artist;
    std::wstring m_folderName;
    std::vector<Chapter> m_chapters;
    std::wstring m_location;
    // From open until the media plays (or fails), with the duration reads waiting for it.
    bool m_opening = false;
    std::vector<std::pair<std::wstring, std::function<void(double)>>> m_waitingDurations;
    Picture m_preview;
    // Last, so it ends (running the commands still queued) before the state they use goes.
    WorkerThread m_engine;
};

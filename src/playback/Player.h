#pragma once

#include <windows.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

#include "core/Signal.h"

struct libvlc_instance_t;
struct libvlc_media_player_t;
struct libvlc_event_t;

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
    const std::wstring &folderName() const { return m_folderName; }

    // The window libVLC draws video into.
    void setVideoWindow(HWND window);
    // Plays a file path or a URL.
    void open(const std::wstring &location);
    void togglePause();
    void toggleMute();
    void seek(double seconds);
    void seekBy(double seconds);
    void setVolume(int percent);
    void nextFrame();
    void cycleSubtitleTrack();
    void cycleAudioTrack();
    // Saves the current frame to Pictures\Sona.
    void takeSnapshot();
    // Stops playback and releases the video window, then runs `done` on the UI thread.
    void stop(std::function<void()> done);

    Signal<> changed;

private:
    static void onEvent(const libvlc_event_t *event, void *player);
    static LRESULT CALLBACK messageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    void runEngine();
    void startEngine();
    void post(std::function<void()> command);
    void runOnUiThread(std::function<void()> task);
    void handleEvent(int type, int videoOutputCount);
    void sync();
    void applyAudio();

    HWND m_messageWindow = nullptr;
    std::thread m_engine;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::deque<std::function<void()>> m_commands;
    bool m_quit = false;
    // Created and released on the engine thread.
    libvlc_instance_t *m_vlc = nullptr;
    std::atomic<libvlc_media_player_t *> m_player = nullptr;

    bool m_playing = false;
    double m_position = 0;
    double m_duration = 0;
    int m_volume = 100;
    bool m_muted = false;
    bool m_hasVideo = false;
    std::wstring m_title;
    std::wstring m_folderName;
};

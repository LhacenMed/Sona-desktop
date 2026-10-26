#include "playback/Player.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <memory>
#include <utility>

#include "core/Text.h"
#include "platform/KnownFolders.h"
#include "playback/LibVlc.h"
#include "playback/MediaFiles.h"

namespace {

constexpr UINT uiTaskMessage = WM_APP + 1;

constexpr libvlc_event_e watchedEvents[] = {
    libvlc_MediaPlayerOpening, libvlc_MediaPlayerPlaying,      libvlc_MediaPlayerPaused,
    libvlc_MediaPlayerStopped, libvlc_MediaPlayerEndReached,   libvlc_MediaPlayerEncounteredError,
    libvlc_MediaPlayerTimeChanged, libvlc_MediaPlayerLengthChanged, libvlc_MediaPlayerVout,
    libvlc_MediaPlayerAudioVolume,
};

// Selects the track after the current one in a libVLC track list (which starts with "Disable"),
// wrapping around.
template <typename Describe, typename Get, typename Select>
void selectNextTrack(libvlc_media_player_t *player, Describe describe, Get get, Select select)
{
    libvlc_track_description_t *tracks = describe(player);
    if (!tracks)
        return;
    const int current = get(player);
    int next = tracks->i_id;
    for (const libvlc_track_description_t *track = tracks; track; track = track->p_next) {
        if (track->i_id == current) {
            next = track->p_next ? track->p_next->i_id : tracks->i_id;
            break;
        }
    }
    select(player, next);
    libvlc_track_description_list_release(tracks);
}

} // namespace

struct Player::DurationRequest
{
    Player *player;
    libvlc_media_t *media;
    std::function<void(double)> done;
    // Set by its first parse event; the events of one media arrive one at a time.
    bool answered = false;
};

Player::Player()
{
    static const ATOM messageClass = [] {
        WNDCLASSW description{};
        description.lpfnWndProc = messageProc;
        description.hInstance = GetModuleHandleW(nullptr);
        description.lpszClassName = L"SonaPlayerEvents";
        return RegisterClassW(&description);
    }();
    m_messageWindow = CreateWindowExW(0, MAKEINTATOM(messageClass), nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                      GetModuleHandleW(nullptr), nullptr);
    m_engine = std::thread([this] { runEngine(); });
    post([this] { startEngine(); });
}

Player::~Player()
{
    post([this] {
        if (libvlc_media_player_t *player = m_player.exchange(nullptr)) {
            libvlc_event_manager_t *events = libvlc_media_player_event_manager(player);
            for (const libvlc_event_e event : watchedEvents)
                libvlc_event_detach(events, event, &Player::onEvent, this);
            libvlc_media_player_stop(player);
            libvlc_media_player_release(player);
        }
        while (!m_durationRequests.empty())
            releaseDurationRequest(m_durationRequests.back().get());
        if (m_vlc)
            libvlc_release(m_vlc);
    });
    {
        const std::lock_guard lock(m_mutex);
        m_quit = true;
    }
    m_wake.notify_one();
    m_engine.join();
    DestroyWindow(m_messageWindow);
}

void Player::runEngine()
{
    for (;;) {
        std::function<void()> command;
        {
            std::unique_lock lock(m_mutex);
            m_wake.wait(lock, [this] { return m_quit || !m_commands.empty(); });
            // Quits only once every command has run.
            if (m_commands.empty())
                return;
            command = std::move(m_commands.front());
            m_commands.pop_front();
        }
        command();
    }
}

void Player::startEngine()
{
    const char *const arguments[] = {"--no-video-title-show", "--no-snapshot-preview", "--no-osd"};
    m_vlc = libvlc_new(static_cast<int>(std::size(arguments)), arguments);
    libvlc_media_player_t *player = m_vlc ? libvlc_media_player_new(m_vlc) : nullptr;
    if (!player) {
        runOnUiThread([] {
            MessageBoxW(nullptr, L"The playback engine (libVLC) could not be loaded.", SONA_APP_NAME, MB_ICONERROR);
        });
        return;
    }
    // Clicks and keys go to the app's views, not to libVLC's video window.
    libvlc_video_set_mouse_input(player, 0);
    libvlc_video_set_key_input(player, 0);
    libvlc_event_manager_t *events = libvlc_media_player_event_manager(player);
    for (const libvlc_event_e event : watchedEvents)
        libvlc_event_attach(events, event, &Player::onEvent, this);
    m_player = player;
}

void Player::post(std::function<void()> command)
{
    {
        const std::lock_guard lock(m_mutex);
        m_commands.push_back(std::move(command));
    }
    m_wake.notify_one();
}

void Player::runOnUiThread(std::function<void()> task)
{
    // messageProc takes ownership of the task on the UI thread.
    auto *posted = new std::function<void()>(std::move(task));
    if (!PostMessageW(m_messageWindow, uiTaskMessage, 0, reinterpret_cast<LPARAM>(posted)))
        delete posted;
}

LRESULT CALLBACK Player::messageProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != uiTaskMessage)
        return DefWindowProcW(hwnd, message, wParam, lParam);
    const std::unique_ptr<std::function<void()>> task(reinterpret_cast<std::function<void()> *>(lParam));
    (*task)();
    return 0;
}

void Player::onEvent(const libvlc_event_t *event, void *opaque)
{
    // Runs on a libVLC thread.
    auto *self = static_cast<Player *>(opaque);
    const int type = event->type;
    const int videoOutputCount = type == libvlc_MediaPlayerVout ? event->u.media_player_vout.new_count : 0;
    self->runOnUiThread([self, type, videoOutputCount] { self->handleEvent(type, videoOutputCount); });
}

void Player::handleEvent(int type, int videoOutputCount)
{
    libvlc_media_player_t *player = m_player;
    if (!player)
        return;
    switch (type) {
    case libvlc_MediaPlayerPlaying:
        // libVLC opens the audio output at its own volume; the app's applies from now on.
        applyAudio();
        if (libvlc_media_t *media = libvlc_media_player_get_media(player)) {
            if (char *title = libvlc_media_get_meta(media, libvlc_meta_Title)) {
                m_title = fromUtf8(title);
                libvlc_free(title);
            }
            libvlc_media_release(media);
        }
        readChapters();
        break;
    case libvlc_MediaPlayerAudioVolume:
        if (libvlc_audio_get_volume(player) != m_volume)
            applyAudio();
        break;
    case libvlc_MediaPlayerVout:
        m_hasVideo = videoOutputCount > 0;
        break;
    }
    sync();
    changed.emit();
    if (type == libvlc_MediaPlayerEndReached)
        ended.emit();
    else if (type == libvlc_MediaPlayerEncounteredError)
        failed.emit();
}

void Player::sync()
{
    libvlc_media_player_t *player = m_player;
    const libvlc_state_t state = libvlc_media_player_get_state(player);
    m_playing = state == libvlc_Opening || state == libvlc_Buffering || state == libvlc_Playing;
    m_duration = std::max<libvlc_time_t>(libvlc_media_player_get_length(player), 0) / 1000.0;
    m_position = std::max<libvlc_time_t>(libvlc_media_player_get_time(player), 0) / 1000.0;
}

void Player::readChapters()
{
    m_chapters.clear();
    libvlc_chapter_description_t **chapters = nullptr;
    // The current title's chapters.
    const int count = libvlc_media_player_get_full_chapter_descriptions(m_player, -1, &chapters);
    for (int i = 0; i < count; ++i) {
        const libvlc_chapter_description_t &chapter = *chapters[i];
        m_chapters.push_back({chapter.psz_name ? fromUtf8(chapter.psz_name) : L"Chapter " + std::to_wstring(i + 1),
                              chapter.i_time_offset / 1000.0});
    }
    if (count > 0)
        libvlc_chapter_descriptions_release(chapters, count);
}

libvlc_media_t *Player::newMedia(const std::wstring &location) const
{
    const std::string utf8 = toUtf8(location);
    return isUrl(location) ? libvlc_media_new_location(m_vlc, utf8.c_str()) : libvlc_media_new_path(m_vlc, utf8.c_str());
}

void Player::applyAudio()
{
    post([this, volume = m_volume, muted = m_muted] {
        if (libvlc_media_player_t *player = m_player) {
            libvlc_audio_set_volume(player, volume);
            libvlc_audio_set_mute(player, muted);
        }
    });
}

void Player::setVideoWindow(HWND window)
{
    post([this, window] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_set_hwnd(player, window);
    });
}

void Player::open(const std::wstring &location)
{
    m_title = displayName(location);
    m_folderName = isUrl(location) ? std::wstring() : std::filesystem::path(location).parent_path().filename().wstring();
    m_chapters.clear();
    changed.emit();

    post([this, location] {
        libvlc_media_player_t *player = m_player;
        if (!player)
            return;
        libvlc_media_t *media = newMedia(location);
        if (!media)
            return;
        libvlc_media_player_set_media(player, media);
        libvlc_media_release(media);
        libvlc_media_player_play(player);
    });
}

void Player::togglePause()
{
    post([this] {
        libvlc_media_player_t *player = m_player;
        if (!player)
            return;
        const libvlc_state_t state = libvlc_media_player_get_state(player);
        // A finished or stopped file plays again from the start.
        if (state == libvlc_Ended || state == libvlc_Stopped)
            libvlc_media_player_play(player);
        else
            libvlc_media_player_pause(player);
    });
}

void Player::toggleMute()
{
    m_muted = !m_muted;
    changed.emit();
    applyAudio();
}

void Player::seek(double seconds)
{
    if (m_duration <= 0)
        return;
    // Shown at once; libVLC confirms with its next time change.
    m_position = std::clamp(seconds, 0.0, m_duration);
    changed.emit();
    post([this, milliseconds = std::llround(m_position * 1000)] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_set_time(player, milliseconds);
    });
}

void Player::seekBy(double seconds)
{
    seek(m_position + seconds);
}

void Player::setVolume(int percent)
{
    m_volume = std::clamp(percent, 0, maxVolume);
    changed.emit();
    applyAudio();
}

void Player::nextFrame()
{
    post([this] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_next_frame(player);
    });
}

void Player::cycleSubtitleTrack()
{
    post([this] {
        if (libvlc_media_player_t *player = m_player)
            selectNextTrack(player, libvlc_video_get_spu_description, libvlc_video_get_spu, libvlc_video_set_spu);
    });
}

void Player::cycleAudioTrack()
{
    post([this] {
        if (libvlc_media_player_t *player = m_player)
            selectNextTrack(player, libvlc_audio_get_track_description, libvlc_audio_get_track, libvlc_audio_set_track);
    });
}

void Player::takeSnapshot()
{
    const std::filesystem::path folder = std::filesystem::path(knownFolderPath(FOLDERID_Pictures)) / L"Sona";
    std::error_code ignored;
    std::filesystem::create_directories(folder, ignored);
    // A folder path lets libVLC name the file.
    post([this, folder = toUtf8(folder.wstring() + L"\\")] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_video_take_snapshot(player, 0, folder.c_str(), 0, 0);
    });
}

void Player::stop(std::function<void()> done)
{
    post([this, done = std::move(done)] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_stop(player);
        runOnUiThread(done);
    });
}

void Player::readDuration(const std::wstring &location, std::function<void(double seconds)> done)
{
    post([this, location, done = std::move(done)]() mutable {
        if (!m_vlc)
            return;
        libvlc_media_t *media = newMedia(location);
        if (!media)
            return;
        DurationRequest *request = m_durationRequests.emplace_back(new DurationRequest{this, media, std::move(done)}).get();
        libvlc_event_attach(libvlc_media_event_manager(media), libvlc_MediaParsedChanged, &Player::onParsed, request);
        // Local files only: a URL isn't fetched to learn its duration.
        libvlc_media_parse_with_options(media, libvlc_media_parse_local, -1);
    });
}

void Player::onParsed(const libvlc_event_t *event, void *opaque)
{
    // Runs on a libVLC thread.
    auto *request = static_cast<DurationRequest *>(opaque);
    if (std::exchange(request->answered, true))
        return;
    Player *self = request->player;
    if (event->u.media_parsed_changed.new_status == libvlc_media_parsed_status_done) {
        if (const libvlc_time_t duration = libvlc_media_get_duration(request->media); duration > 0)
            self->runOnUiThread([done = request->done, seconds = duration / 1000.0] { done(seconds); });
    }
    // A media can't be released from its own event.
    self->post([self, request] { self->releaseDurationRequest(request); });
}

void Player::releaseDurationRequest(DurationRequest *request)
{
    // Already released when the engine stopped first.
    const auto found = std::ranges::find_if(m_durationRequests, [request](const auto &entry) { return entry.get() == request; });
    if (found == m_durationRequests.end())
        return;
    // Waits for an event being delivered to finish.
    libvlc_event_detach(libvlc_media_event_manager(request->media), libvlc_MediaParsedChanged, &Player::onParsed, request);
    libvlc_media_release(request->media);
    m_durationRequests.erase(found);
}

#include "playback/Player.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <memory>
#include <utility>

#include "core/Text.h"
#include "platform/KnownFolders.h"
#include "playback/LibVlc.h"
#include "playback/MediaFiles.h"

namespace {

constexpr libvlc_event_e watchedEvents[] = {
    libvlc_MediaPlayerOpening, libvlc_MediaPlayerPlaying,      libvlc_MediaPlayerPaused,
    libvlc_MediaPlayerStopped, libvlc_MediaPlayerEndReached,   libvlc_MediaPlayerEncounteredError,
    libvlc_MediaPlayerTimeChanged, libvlc_MediaPlayerLengthChanged, libvlc_MediaPlayerVout,
    libvlc_MediaPlayerAudioVolume,
};

// The largest preview picture, in pixels: twice Figma's 252×142 thumbnail, for high-DPI screens.
constexpr unsigned previewMaxWidth = 504;
constexpr unsigned previewMaxHeight = 284;

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
    m_engine.post([this] { startEngine(); });
}

Player::~Player()
{
    m_engine.post([this] {
        if (libvlc_media_player_t *player = m_player.exchange(nullptr)) {
            libvlc_event_manager_t *events = libvlc_media_player_event_manager(player);
            for (const libvlc_event_e event : watchedEvents)
                libvlc_event_detach(events, event, &Player::onEvent, this);
            libvlc_media_player_stop(player);
            libvlc_media_player_release(player);
        }
        if (libvlc_media_player_t *preview = m_previewPlayer.exchange(nullptr)) {
            libvlc_media_player_stop(preview);
            libvlc_media_player_release(preview);
        }
        while (!m_durationRequests.empty())
            releaseDurationRequest(m_durationRequests.back().get());
        if (m_vlc)
            libvlc_release(m_vlc);
    });
}

void Player::startEngine()
{
    const char *const arguments[] = {"--no-video-title-show", "--no-snapshot-preview", "--no-osd"};
    m_vlc = libvlc_new(static_cast<int>(std::size(arguments)), arguments);
    libvlc_media_player_t *player = m_vlc ? libvlc_media_player_new(m_vlc) : nullptr;
    if (!player) {
        m_engine.runOnUiThread([] {
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

    if (libvlc_media_player_t *preview = libvlc_media_player_new(m_vlc)) {
        libvlc_video_set_format_callbacks(preview, &Player::previewFormat, nullptr);
        libvlc_video_set_callbacks(preview, &Player::previewLock, nullptr, &Player::previewDisplay, this);
        m_previewPlayer = preview;
    }
}

void Player::onEvent(const libvlc_event_t *event, void *opaque)
{
    // Runs on a libVLC thread.
    auto *self = static_cast<Player *>(opaque);
    const int type = event->type;
    const int videoOutputCount = type == libvlc_MediaPlayerVout ? event->u.media_player_vout.new_count : 0;
    self->m_engine.runOnUiThread([self, type, videoOutputCount] { self->handleEvent(type, videoOutputCount); });
}

unsigned Player::previewFormat(void **opaque, char *chroma, unsigned *width, unsigned *height, unsigned *pitches, unsigned *lines)
{
    // Runs on a libVLC thread, as do previewLock and previewDisplay.
    auto *self = static_cast<Player *>(*opaque);
    // A video stored on its side (a phone's) comes rotated upright, its sides swapped.
    if (libvlc_media_t *media = libvlc_media_player_get_media(self->m_previewPlayer)) {
        libvlc_media_track_t **tracks = nullptr;
        const unsigned count = libvlc_media_tracks_get(media, &tracks);
        for (unsigned i = 0; i < count; ++i) {
            if (tracks[i]->i_type == libvlc_track_video && tracks[i]->video->i_orientation >= libvlc_video_orient_left_top)
                std::swap(*width, *height);
        }
        libvlc_media_tracks_release(tracks, count);
        libvlc_media_release(media);
    }
    const double scale = std::min({1.0, static_cast<double>(previewMaxWidth) / *width, static_cast<double>(previewMaxHeight) / *height});
    *width = std::max(1u, static_cast<unsigned>(*width * scale));
    *height = std::max(1u, static_cast<unsigned>(*height * scale));
    std::memcpy(chroma, "RV32", 4);
    *pitches = *width * 4;
    *lines = *height;
    self->m_previewDecoding = {static_cast<int>(*width), static_cast<int>(*height), std::vector<std::uint32_t>(*width * *height)};
    return 1;
}

void *Player::previewLock(void *opaque, void **planes)
{
    *planes = static_cast<Player *>(opaque)->m_previewDecoding.pixels.data();
    return nullptr;
}

void Player::previewDisplay(void *opaque, void *)
{
    auto *self = static_cast<Player *>(opaque);
    // One picture per moment asked for: it rests until the next.
    self->m_engine.post([self] {
        if (libvlc_media_player_t *preview = self->m_previewPlayer)
            libvlc_media_player_set_pause(preview, 1);
    });
    self->m_engine.runOnUiThread([self, frame = self->m_previewDecoding] {
        self->m_preview = std::move(frame);
        self->previewChanged.emit();
    });
}

void Player::handleEvent(int type, int videoOutputCount)
{
    libvlc_media_player_t *player = m_player;
    if (!player)
        return;
    // Duration reads waited for the media to start.
    if (m_opening && (type == libvlc_MediaPlayerPlaying || type == libvlc_MediaPlayerEncounteredError)) {
        m_opening = false;
        for (auto &[location, done] : std::exchange(m_waitingDurations, {}))
            readDuration(location, std::move(done));
    }
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
    m_engine.post([this, volume = m_volume, muted = m_muted] {
        if (libvlc_media_player_t *player = m_player) {
            libvlc_audio_set_volume(player, volume);
            libvlc_audio_set_mute(player, muted);
        }
    });
}

void Player::setVideoWindow(HWND window)
{
    m_engine.post([this, window] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_set_hwnd(player, window);
    });
}

void Player::open(const std::wstring &location)
{
    m_location = location;
    m_opening = true;
    closePreview();
    m_title = displayName(location);
    m_folderName = isUrl(location) ? std::wstring() : std::filesystem::path(location).parent_path().filename().wstring();
    m_chapters.clear();
    changed.emit();

    m_engine.post([this, location] {
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
    m_engine.post([this] {
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
    m_seekTarget = std::llround(m_position * 1000);
    if (m_seekQueued.exchange(true))
        return;
    m_engine.post([this] {
        m_seekQueued = false;
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_set_time(player, m_seekTarget);
    });
}

void Player::preparePreview()
{
    if (!m_location.empty())
        m_engine.post([this, location = m_location, seconds = m_position] { openPreviewMedia(location, seconds); });
}

void Player::previewAt(double seconds)
{
    if (m_duration <= 0)
        return;
    m_previewTarget = std::llround(std::clamp(seconds, 0.0, m_duration) * 1000);
    if (m_previewQueued.exchange(true))
        return;
    m_engine.post([this, location = m_location] {
        m_previewQueued = false;
        openPreviewMedia(location, m_previewTarget / 1000.0);
        if (libvlc_media_player_t *preview = m_previewPlayer) {
            libvlc_media_player_set_time(preview, m_previewTarget);
            // A paused player decoding into memory shows nothing new; it plays to its next picture.
            libvlc_media_player_set_pause(preview, 0);
        }
    });
}

void Player::openPreviewMedia(const std::wstring &location, double seconds)
{
    libvlc_media_player_t *preview = m_previewPlayer;
    if (!preview || location == m_previewMedia)
        return;
    libvlc_media_t *media = newMedia(location);
    if (!media)
        return;
    // Seeks to the nearest keyframe, which shows at once; silent, and still at its first picture.
    libvlc_media_add_option(media, ":input-fast-seek");
    libvlc_media_add_option(media, ":no-audio");
    libvlc_media_add_option(media, ":start-paused");
    libvlc_media_add_option(media, (":start-time=" + std::to_string(seconds)).c_str());
    libvlc_media_player_set_media(preview, media);
    libvlc_media_release(media);
    libvlc_media_player_play(preview);
    m_previewMedia = location;
}

void Player::closePreview()
{
    // A picture of the previous media is no preview of this one.
    m_preview = {};
    m_engine.post([this] {
        if (libvlc_media_player_t *preview = m_previewPlayer)
            libvlc_media_player_stop(preview);
        m_previewMedia.clear();
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
    m_engine.post([this] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_next_frame(player);
    });
}

void Player::cycleSubtitleTrack()
{
    m_engine.post([this] {
        if (libvlc_media_player_t *player = m_player)
            selectNextTrack(player, libvlc_video_get_spu_description, libvlc_video_get_spu, libvlc_video_set_spu);
    });
}

void Player::cycleAudioTrack()
{
    m_engine.post([this] {
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
    m_engine.post([this, folder = toUtf8(folder.wstring() + L"\\")] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_video_take_snapshot(player, 0, folder.c_str(), 0, 0);
    });
}

void Player::stop(std::function<void()> done)
{
    m_engine.post([this, done = std::move(done)] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_stop(player);
        m_engine.runOnUiThread(done);
    });
}

void Player::readDuration(const std::wstring &location, std::function<void(double seconds)> done)
{
    if (m_opening) {
        m_waitingDurations.emplace_back(location, std::move(done));
        return;
    }
    m_engine.post([this, location, done = std::move(done)]() mutable {
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
            self->m_engine.runOnUiThread([done = request->done, seconds = duration / 1000.0] { done(seconds); });
    }
    // A media can't be released from its own event.
    self->m_engine.post([self, request] { self->releaseDurationRequest(request); });
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

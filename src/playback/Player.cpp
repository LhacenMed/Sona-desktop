#include "playback/Player.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <cmath>
#include <cstdio>
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
    libvlc_MediaPlayerAudioVolume, libvlc_MediaPlayerESAdded,     libvlc_MediaPlayerESDeleted,
    libvlc_MediaPlayerESSelected,  libvlc_MediaPlayerAudioDevice,
};

// The largest preview picture, in pixels: twice Figma's 252×142 thumbnail, for high-DPI screens.
constexpr unsigned previewMaxWidth = 504;
constexpr unsigned previewMaxHeight = 284;

// How long preference changes libVLC reads only when it starts must rest before it restarts with them.
constexpr std::chrono::milliseconds restartDelay{400};

// A ratio as libVLC takes it, from the name the panel shows: "16:9", or "2.21:1" as "221:100".
std::string ratioArgument(std::wstring_view name)
{
    const std::string ratio = toUtf8(std::wstring(name));
    const size_t colon = ratio.find(':');
    const size_t point = ratio.find('.');
    if (point == std::string::npos || point > colon)
        return ratio;
    const std::string whole = ratio.substr(0, point) + ratio.substr(point + 1, colon - point - 1);
    return whole + ":" + ratio.substr(colon + 1) + std::string(colon - point - 1, '0');
}

// What the tracks panel calls libVLC's "Disable" (Figma).
constexpr wchar_t noTrackName[] = L"None";

// A file's URI, as libVLC takes a slave's: file:///C:/My%20Subtitles/a.srt, or file://server/share/a.srt.
std::string fileUri(const std::wstring &path)
{
    const std::string utf8 = toUtf8(path);
    std::string uri = utf8.starts_with("\\\\") ? "file:" : "file:///";
    for (const char c : utf8) {
        if (c == '\\') {
            uri += '/';
        } else if (std::isalnum(static_cast<unsigned char>(c)) || std::strchr("-._~:/", c)) {
            uri += c;
        } else {
            char escaped[4];
            std::snprintf(escaped, sizeof escaped, "%%%02X", static_cast<unsigned char>(c));
            uri += escaped;
        }
    }
    return uri;
}

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
    std::wstring location;
    libvlc_media_t *media;
    std::function<void(double)> done;
    // Set by its first parse event; the events of one media arrive one at a time.
    bool answered = false;
};

Player::Player()
    : m_restartTimer([this] { restartEngine(true); })
{
    m_engine.post([this, arguments = engineArguments(m_preferences)] { startEngine(arguments); });
}

Player::~Player()
{
    m_engine.post([this] { stopEngine(m_player.exchange(nullptr)); });
}

std::vector<std::string> Player::engineArguments(const PreferenceValues &preferences)
{
    using enum Preference;
    const auto set = [&preferences](Preference preference) { return preferences[preference] != preferenceInfo(preference).fallback; };
    const auto on = [&preferences](Preference preference) { return preferences[preference] != 0; };
    const auto number = [&preferences](Preference preference) { return std::format("{}", preferences[preference]); };
    const auto integer = [&preferences](Preference preference) { return std::format("{}", std::lround(preferences[preference])); };

    std::vector<std::string> arguments{"--no-video-title-show", "--no-snapshot-preview", "--no-osd"};
    std::vector<std::string> filters;
    if (on(Flip)) {
        filters.emplace_back("transform");
        arguments.push_back(preferences[Flip] == 1 ? "--transform-type=hflip" : "--transform-type=vflip");
    }
    if (on(Rotate)) {
        filters.emplace_back("rotate");
        arguments.push_back("--rotate-angle=" + number(Rotate));
    }
    if (on(Sharpen)) {
        filters.emplace_back("sharpen");
        arguments.push_back("--sharpen-sigma=" + number(Sharpen));
    }
    if (on(BandingRemoval)) {
        filters.emplace_back("gradfun");
        arguments.push_back("--gradfun-radius=" + integer(BandingRemoval));
    }
    if (on(FilmGrain)) {
        filters.emplace_back("grain");
        arguments.push_back("--grain-variance=" + number(FilmGrain));
    }
    if (on(ColorExtraction)) {
        filters.emplace_back("extract");
        arguments.push_back("--extract-component=" + integer(ExtractedColor));
    }
    if (on(ColorThreshold)) {
        filters.emplace_back("colorthres");
        arguments.push_back("--colorthres-color=" + integer(ThresholdColor));
        arguments.push_back("--colorthres-saturationthres=" + integer(ThresholdSaturation));
        arguments.push_back("--colorthres-similaritythres=" + integer(ThresholdSimilarity));
    }
    if (on(NegateColors))
        filters.emplace_back("invert");
    if (on(Posterize))
        filters.emplace_back("posterize");
    if (on(Gradient)) {
        constexpr const char *modes[] = {"gradient", "edge", "hough"};
        filters.emplace_back("gradient");
        arguments.push_back(std::string("--gradient-mode=") + modes[std::lround(preferences[GradientMode])]);
        arguments.push_back("--gradient-type=" + integer(GradientColor));
        arguments.push_back(on(Cartoon) ? "--gradient-cartoon" : "--no-gradient-cartoon");
    }
    if (on(SepiaIntensity)) {
        filters.emplace_back("sepia");
        arguments.push_back("--sepia-intensity=" + integer(SepiaIntensity));
    }
    if (!filters.empty()) {
        std::string chain = filters.front();
        for (size_t i = 1; i < filters.size(); ++i)
            chain += ":" + filters[i];
        arguments.push_back("--video-filter=" + chain);
    }

    if (on(TwoPass))
        arguments.emplace_back("--equalizer-2pass");
    if (on(SubtitleSpeed))
        arguments.push_back("--sub-fps=" + number(SubtitleSpeed));
    if (on(SubtitleDurationFactor)) {
        arguments.emplace_back("--sub-filter=subsdelay");
        arguments.emplace_back("--subsdelay-mode=1");
        arguments.push_back("--subsdelay-factor=" + number(SubtitleDurationFactor));
    }
    if (!on(EmbeddedSubFormat))
        arguments.emplace_back("--no-subsdec-formatted");
    if (on(SubtitleFont))
        arguments.push_back("--freetype-font=" + toUtf8(subtitleFonts[std::lround(preferences[SubtitleFont])]));
    if (set(TextColor))
        arguments.push_back("--freetype-color=" + integer(TextColor));
    if (set(OutlineColor))
        arguments.push_back("--freetype-outline-color=" + integer(OutlineColor));
    if (on(SubtitleOffset))
        arguments.push_back("--sub-margin=" + integer(SubtitleOffset));
    if (preferences[SizeValues] == 0 && on(RelativeFontSize)) {
        // libVLC's relative sizes, from Smaller to Larger: the text is 1/n of the picture's height.
        constexpr int relativeSizes[] = {20, 18, 16, 12, 6};
        arguments.push_back(std::format("--freetype-rel-fontsize={}", relativeSizes[std::lround(preferences[RelativeFontSize]) - 1]));
    } else if (preferences[SizeValues] == 1 && on(PixelFontSize)) {
        arguments.push_back("--freetype-fontsize=" + integer(PixelFontSize));
    }
    if (set(OutlineThickness)) {
        // None, Thin, Normal, Thick.
        constexpr int thicknesses[] = {0, 2, 4, 6};
        arguments.push_back(std::format("--freetype-outline-thickness={}", thicknesses[std::lround(preferences[OutlineThickness])]));
    }
    if (on(BackgroundOpacity))
        arguments.push_back(std::format("--freetype-background-opacity={}", std::lround(preferences[BackgroundOpacity] * 2.55)));
    return arguments;
}

void Player::startEngine(const std::vector<std::string> &arguments)
{
    std::vector<const char *> argv;
    for (const std::string &argument : arguments)
        argv.push_back(argument.c_str());
    m_vlc = libvlc_new(static_cast<int>(argv.size()), argv.data());
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
    if (m_videoWindow)
        libvlc_media_player_set_hwnd(player, m_videoWindow);
    m_player = player;

    if (libvlc_media_player_t *preview = libvlc_media_player_new(m_vlc)) {
        libvlc_video_set_format_callbacks(preview, &Player::previewFormat, nullptr);
        libvlc_video_set_callbacks(preview, &Player::previewLock, nullptr, &Player::previewDisplay, this);
        m_previewPlayer = preview;
    }
}

void Player::stopEngine(libvlc_media_player_t *player)
{
    if (player) {
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
    m_previewMedia.clear();
    while (!m_durationRequests.empty())
        releaseDurationRequest(m_durationRequests.back().get());
    if (m_vlc) {
        libvlc_release(m_vlc);
        m_vlc = nullptr;
    }
}

void Player::restartEngine(bool resume)
{
    m_restartTimer.stop();
    libvlc_media_player_t *player = m_player.exchange(nullptr);
    const libvlc_state_t state = player ? libvlc_media_player_get_state(player) : libvlc_NothingSpecial;
    // A media opening may not have reached libVLC yet (its command waits behind libVLC's start), so
    // that it's opening is known here, not from libVLC.
    resume = resume && !m_location.empty() &&
             (m_opening || state == libvlc_Buffering || state == libvlc_Playing || state == libvlc_Paused);

    // The subtitle files go back once the media plays again, the one showing shown.
    const auto shown = std::ranges::find(m_externalSubtitles, currentTrack(TrackKind::Subtitle), &Track::id);
    m_subtitleFileToShow.clear();
    for (const std::wstring &file : m_subtitleFiles) {
        if (shown != m_externalSubtitles.end() && std::filesystem::path(file).filename() == shown->name)
            m_subtitleFileToShow = file;
    }
    m_subtitleFilesToRestore = resume ? std::exchange(m_subtitleFiles, {}) : std::vector<std::wstring>();
    m_externalSubtitles.clear();
    m_loadingSubtitles.clear();
    // In what plays: where, whether paused, and its tracks once they're known (a subtitle file's
    // comes back with it).
    std::vector<std::string> options;
    if (resume) {
        if (!m_opening) {
            m_startTime = m_position;
            options.push_back(std::format(":video-track-id={}", currentTrack(TrackKind::Video)));
            options.push_back(std::format(":audio-track-id={}", currentTrack(TrackKind::Audio)));
            if (shown == m_externalSubtitles.end() && currentTrack(TrackKind::Subtitle) >= 0)
                options.push_back(std::format(":sub-track-id={}", currentTrack(TrackKind::Subtitle)));
        }
        options.push_back(std::format(":start-time={}", m_startTime));
        if (state == libvlc_Paused)
            options.emplace_back(":start-paused");
        m_opening = true;
        m_preferencesApplied = false;
    }

    m_engine.post([this, player, arguments = engineArguments(m_preferences), resume, location = m_location, options] {
        // Duration reads under way start again on the new engine.
        std::vector<std::pair<std::wstring, std::function<void(double)>>> durations;
        for (const auto &request : m_durationRequests) {
            if (!request->answered)
                durations.emplace_back(request->location, request->done);
        }
        stopEngine(player);
        startEngine(arguments);
        if (libvlc_media_player_t *restarted = m_player; restarted && resume) {
            if (libvlc_media_t *media = newMedia(location)) {
                addMediaOptions(media);
                for (const std::string &option : options)
                    libvlc_media_add_option(media, option.c_str());
                libvlc_media_player_set_media(restarted, media);
                libvlc_media_release(media);
                libvlc_media_player_play(restarted);
            }
        }
        m_engine.runOnUiThread([this, durations] {
            for (const auto &[location, done] : durations)
                readDuration(location, done);
        });
    });
}

void Player::applyPreferences(const PreferenceValues &values)
{
    const PreferenceValues previous = std::exchange(m_preferences, values);
    applyLivePreferences(&previous);
    using enum Preference;
    if (engineArguments(values) == engineArguments(previous) && values[AudioLanguage] == previous[AudioLanguage] &&
        values[SubtitleLanguage] == previous[SubtitleLanguage])
        return;
    if (m_inOpen)
        restartEngine(false);
    else
        m_restartTimer.start(restartDelay, false);
}

void Player::applyLivePreferences(const PreferenceValues *previous)
{
    using enum Preference;
    const PreferenceValues values = m_preferences;
    const auto differs = [&values, previous](std::initializer_list<Preference> preferences) {
        return !previous || std::ranges::any_of(preferences, [&](Preference preference) { return values[preference] != (*previous)[preference]; });
    };
    const bool speed = differs({Speed});
    const bool adjust = differs({Hue, Brightness, Contrast, Saturation, Gamma});
    const bool zoom = differs({Zoom});
    const bool aspectRatio = differs({AspectRatio});
    const bool crop = differs({Crop, CropTop, CropBottom, CropLeft, CropRight});
    const bool audioDelay = differs({AudioDelay});
    const bool subtitleDelay = differs({SubtitleDelay});
    const bool equalizer =
        differs({EqualizerPreset, Preamp, Band60, Band170, Band310, Band600, Band1k, Band3k, Band6k, Band12k, Band14k, Band16k});
    // Left alone until chosen: on media with more than two channels, setting it mixes them down.
    const bool stereoMode = previous ? values[StereoMode] != (*previous)[StereoMode]
                                     : values[StereoMode] != preferenceInfo(StereoMode).fallback;
    if (!(speed || adjust || zoom || aspectRatio || crop || audioDelay || subtitleDelay || equalizer || stereoMode))
        return;

    m_engine.post([this, values, speed, adjust, zoom, aspectRatio, crop, audioDelay, subtitleDelay, equalizer, stereoMode] {
        libvlc_media_player_t *player = m_player;
        if (!player)
            return;
        const auto microseconds = [&values](Preference preference) { return std::llround(values[preference] * 1'000'000); };
        if (speed)
            libvlc_media_player_set_rate(player, static_cast<float>(values[Speed]));
        if (adjust) {
            const bool any = std::ranges::any_of(std::initializer_list<Preference>{Hue, Brightness, Contrast, Saturation, Gamma},
                                                 [&values](Preference preference) { return values[preference] != preferenceInfo(preference).fallback; });
            libvlc_video_set_adjust_int(player, libvlc_adjust_Enable, any);
            libvlc_video_set_adjust_float(player, libvlc_adjust_Hue, static_cast<float>(values[Hue]));
            libvlc_video_set_adjust_float(player, libvlc_adjust_Brightness, static_cast<float>(values[Brightness]));
            libvlc_video_set_adjust_float(player, libvlc_adjust_Contrast, static_cast<float>(values[Contrast]));
            libvlc_video_set_adjust_float(player, libvlc_adjust_Saturation, static_cast<float>(values[Saturation]));
            libvlc_video_set_adjust_float(player, libvlc_adjust_Gamma, static_cast<float>(values[Gamma]));
        }
        if (zoom) {
            constexpr float scales[] = {0, 0.25f, 0.5f, 1, 2};
            libvlc_video_set_scale(player, scales[std::lround(values[Zoom])]);
        }
        if (aspectRatio) {
            const long index = std::lround(values[AspectRatio]);
            const std::string ratio = index > 0 ? ratioArgument(preferenceInfo(AspectRatio).names[index]) : std::string();
            libvlc_video_set_aspect_ratio(player, index > 0 ? ratio.c_str() : nullptr);
        }
        if (crop) {
            // Borders, when any is set, or a ratio.
            std::string geometry;
            if (values[CropTop] > 0 || values[CropBottom] > 0 || values[CropLeft] > 0 || values[CropRight] > 0) {
                geometry = std::format("{}+{}+{}+{}", std::lround(values[CropLeft]), std::lround(values[CropTop]),
                                       std::lround(values[CropRight]), std::lround(values[CropBottom]));
            } else if (const long index = std::lround(values[Crop]); index > 0) {
                geometry = ratioArgument(preferenceInfo(Crop).names[index]);
            }
            libvlc_video_set_crop_geometry(player, geometry.empty() ? nullptr : geometry.c_str());
        }
        if (audioDelay)
            libvlc_audio_set_delay(player, microseconds(AudioDelay));
        if (subtitleDelay)
            libvlc_video_set_spu_delay(player, microseconds(SubtitleDelay));
        if (stereoMode)
            libvlc_audio_set_channel(player, static_cast<int>(std::lround(values[StereoMode])) + libvlc_AudioChannel_Stereo);
        if (equalizer) {
            constexpr Preference bands[] = {Band60, Band170, Band310, Band600, Band1k, Band3k, Band6k, Band12k, Band14k, Band16k};
            const bool flat = values[EqualizerPreset] == 0 && values[Preamp] == 0 &&
                              std::ranges::all_of(bands, [&values](Preference band) { return values[band] == 0; });
            libvlc_equalizer_t *settings = flat ? nullptr : libvlc_audio_equalizer_new();
            if (settings) {
                libvlc_audio_equalizer_set_preamp(settings, static_cast<float>(values[Preamp]));
                for (unsigned i = 0; i < std::size(bands); ++i)
                    libvlc_audio_equalizer_set_amp_at_index(settings, static_cast<float>(values[bands[i]]), i);
            }
            libvlc_media_player_set_equalizer(player, settings);
            if (settings)
                libvlc_audio_equalizer_release(settings);
        }
    });
}

std::vector<double> Player::equalizerPreset(int preset)
{
    std::vector<double> values;
    libvlc_equalizer_t *equalizer = libvlc_audio_equalizer_new_from_preset(static_cast<unsigned>(preset - 1));
    if (!equalizer)
        return values;
    values.push_back(libvlc_audio_equalizer_get_preamp(equalizer));
    for (unsigned i = 0; i < libvlc_audio_equalizer_get_band_count(); ++i)
        values.push_back(libvlc_audio_equalizer_get_amp_at_index(equalizer, i));
    libvlc_audio_equalizer_release(equalizer);
    return values;
}

void Player::addMediaOptions(libvlc_media_t *media) const
{
    const auto addLanguage = [this, media](Preference preference, const char *option) {
        const std::string_view code = trackLanguages[std::lround(m_preferences[preference])].code;
        if (!code.empty())
            libvlc_media_add_option(media, std::format(":{}={}", option, code).c_str());
    };
    addLanguage(Preference::AudioLanguage, "audio-language");
    addLanguage(Preference::SubtitleLanguage, "sub-language");
}

void Player::onEvent(const libvlc_event_t *event, void *opaque)
{
    // Runs on a libVLC thread.
    auto *self = static_cast<Player *>(opaque);
    const int type = event->type;
    const int videoOutputCount = type == libvlc_MediaPlayerVout ? event->u.media_player_vout.new_count : 0;
    const int addedSubtitle = type == libvlc_MediaPlayerESAdded && event->u.media_player_es_changed.i_type == libvlc_track_text
                                  ? event->u.media_player_es_changed.i_id
                                  : -1;
    self->m_engine.runOnUiThread(
        [self, type, videoOutputCount, addedSubtitle] { self->handleEvent(type, videoOutputCount, addedSubtitle); });
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

void Player::handleEvent(int type, int videoOutputCount, int addedSubtitle)
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
            if (char *artist = libvlc_media_get_meta(media, libvlc_meta_Artist)) {
                m_artist = fromUtf8(artist);
                libvlc_free(artist);
            }
            libvlc_media_release(media);
        }
        readChapters();
        readTracks();
        readAudioDevices();
        // Once per media: the live preferences, and the subtitle files a restart took away.
        if (!std::exchange(m_preferencesApplied, true))
            applyLivePreferences(nullptr);
        for (const std::wstring &file : std::exchange(m_subtitleFilesToRestore, {}))
            addSubtitleFile(file, file == m_subtitleFileToShow);
        break;
    case libvlc_MediaPlayerESAdded:
        // A subtitle file added is the next subtitle track to appear.
        if (addedSubtitle >= 0 && !m_loadingSubtitles.empty()) {
            m_externalSubtitles.push_back({addedSubtitle, std::move(m_loadingSubtitles.front())});
            m_loadingSubtitles.pop_front();
        }
        readTracks();
        break;
    case libvlc_MediaPlayerESDeleted:
    case libvlc_MediaPlayerESSelected:
        readTracks();
        break;
    case libvlc_MediaPlayerAudioDevice:
        readAudioDevices();
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

void Player::readTracks()
{
    libvlc_media_player_t *player = m_player;
    // In TrackKind's order.
    libvlc_track_description_t *const lists[] = {libvlc_video_get_track_description(player),
                                                  libvlc_audio_get_track_description(player),
                                                  libvlc_video_get_spu_description(player)};
    const int current[] = {libvlc_video_get_track(player), libvlc_audio_get_track(player), libvlc_video_get_spu(player)};
    std::vector<Track> external;
    for (size_t kind = 0; kind < m_tracks.size(); ++kind) {
        std::vector<Track> &tracks = m_tracks[kind];
        tracks = {{-1, noTrackName}};
        for (const libvlc_track_description_t *track = lists[kind]; track; track = track->p_next) {
            if (track->i_id < 0)
                continue;
            const auto added = std::ranges::find(m_externalSubtitles, track->i_id, &Track::id);
            if (added != m_externalSubtitles.end())
                external.push_back(*added);
            else
                tracks.push_back({track->i_id, track->psz_name ? fromUtf8(track->psz_name) : std::wstring()});
        }
        if (lists[kind])
            libvlc_track_description_list_release(lists[kind]);
        m_currentTracks[kind] = current[kind];
    }
    m_externalSubtitles = std::move(external);
}

void Player::readAudioDevices()
{
    libvlc_media_player_t *player = m_player;
    m_audioDevices.clear();
    libvlc_audio_output_device_t *devices = libvlc_audio_output_device_enum(player);
    for (const libvlc_audio_output_device_t *device = devices; device; device = device->p_next) {
        m_audioDevices.push_back({device->psz_device ? device->psz_device : "",
                                  device->psz_description ? fromUtf8(device->psz_description) : std::wstring()});
    }
    libvlc_audio_output_device_list_release(devices);
    // None selected means the default device.
    char *current = libvlc_audio_output_device_get(player);
    m_audioDevice = current ? current : "";
    libvlc_free(current);
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
        m_videoWindow = window;
        if (libvlc_media_player_t *player = m_player)
            libvlc_media_player_set_hwnd(player, window);
    });
}

void Player::open(const std::wstring &location, double startTime)
{
    m_location = location;
    m_opening = true;
    m_startTime = startTime;
    m_preferencesApplied = false;
    closePreview();
    m_title = displayName(location);
    m_artist.clear();
    m_folderName = isUrl(location) ? std::wstring() : std::filesystem::path(location).parent_path().filename().wstring();
    m_chapters.clear();
    m_tracks.fill({{-1, noTrackName}});
    m_currentTracks.fill(-1);
    m_externalSubtitles.clear();
    m_loadingSubtitles.clear();
    m_subtitleFiles.clear();
    m_subtitleFilesToRestore.clear();
    m_inOpen = true;
    changed.emit();
    m_inOpen = false;
    // A restart waiting for changes to settle happens now, so the media opens once, with them.
    if (m_restartTimer.isActive())
        restartEngine(false);

    m_engine.post([this, location, startTime] {
        libvlc_media_player_t *player = m_player;
        if (!player)
            return;
        libvlc_media_t *media = newMedia(location);
        if (!media)
            return;
        addMediaOptions(media);
        if (startTime > 0)
            libvlc_media_add_option(media, (":start-time=" + std::to_string(startTime)).c_str());
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

void Player::selectTrack(TrackKind kind, int id)
{
    // Shown at once; libVLC confirms as it switches.
    m_currentTracks[static_cast<size_t>(kind)] = id;
    changed.emit();
    m_engine.post([this, kind, id] {
        libvlc_media_player_t *player = m_player;
        if (!player)
            return;
        switch (kind) {
        case TrackKind::Video:
            libvlc_video_set_track(player, id);
            break;
        case TrackKind::Audio:
            libvlc_audio_set_track(player, id);
            break;
        case TrackKind::Subtitle:
            libvlc_video_set_spu(player, id);
            break;
        }
    });
}

void Player::addSubtitleFile(const std::wstring &path, bool show)
{
    const std::wstring name = std::filesystem::path(path).filename().wstring();
    m_loadingSubtitles.push_back(name);
    m_subtitleFiles.push_back(path);
    m_engine.post([this, name, show, uri = fileUri(path)] {
        libvlc_media_player_t *player = m_player;
        if (player && libvlc_media_player_add_slave(player, libvlc_media_slave_type_subtitle, uri.c_str(), show) == 0)
            return;
        // It won't load, so it names no track.
        m_engine.runOnUiThread([this, name] {
            if (const auto loading = std::ranges::find(m_loadingSubtitles, name); loading != m_loadingSubtitles.end())
                m_loadingSubtitles.erase(loading);
        });
    });
}

void Player::setAudioDevice(const std::string &id)
{
    // Shown at once; libVLC confirms as it moves.
    m_audioDevice = id;
    changed.emit();
    m_engine.post([this, id] {
        if (libvlc_media_player_t *player = m_player)
            libvlc_audio_output_device_set(player, nullptr, id.c_str());
    });
}

void Player::refreshAudioDevices()
{
    if (!m_player)
        return;
    readAudioDevices();
    changed.emit();
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
        DurationRequest *request = m_durationRequests.emplace_back(new DurationRequest{this, location, media, std::move(done)}).get();
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

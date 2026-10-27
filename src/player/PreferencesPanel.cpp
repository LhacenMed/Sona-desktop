#include "player/PreferencesPanel.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "commands/CommandRegistry.h"
#include "core/Signal.h"
#include "graphics/Canvas.h"
#include "platform/PopupWindow.h"
#include "platform/Window.h"
#include "playback/ActivePreset.h"
#include "playback/Player.h"
#include "player/BarItems.h"
#include "style/Theme.h"
#include "widgets/Button.h"
#include "widgets/DisabledArea.h"
#include "widgets/Dropdown.h"
#include "widgets/Expander.h"
#include "widgets/Label.h"
#include "widgets/Menu.h"
#include "widgets/ScrollView.h"
#include "widgets/Segmented.h"
#include "widgets/Slider.h"
#include "widgets/Tabs.h"
#include "widgets/TextField.h"
#include "widgets/Toggle.h"

namespace {

using enum Preference;

// Figma: a heading 20 below what's above it and 24 in; a section's settings 20 in from the sides
// and 5 apart, each 48 tall with its name 4 in; fields 140 wide, dropdowns 220, both at the end.
constexpr TextStyle headingStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_MEDIUM};
constexpr TextStyle labelStyle{.size = 14};
constexpr TextStyle choiceStyle{.size = 14, .elide = true};
constexpr float rowHeight = 48;
constexpr float labelRowHeight = 37;
constexpr float fieldWidth = 140;
constexpr float fieldHeight = 36;
constexpr float dropdownWidth = 220;
constexpr float dropdownHeight = 48;
constexpr float fieldTextInset = 12;
constexpr float swatchSize = 22;
constexpr float swatchInset = 7;

std::wstring disabledReason(PreferenceTiming timing)
{
    return timing == PreferenceTiming::Unavailable ? L"Not possible with this version of libVLC" : L"Not built yet";
}

bool isDisabled(const PreferenceInfo &info)
{
    return info.timing == PreferenceTiming::Unavailable || info.timing == PreferenceTiming::Later;
}

std::wstring formatValue(const PreferenceInfo &info, double value)
{
    if (info.control == PreferenceControl::Color)
        return std::format(L"#{:06X}", std::lround(value));
    return std::vformat(info.format, std::make_wformat_args(value));
}

// The number a field's text starts with ("1.5x" reads as 1.5), or a colour's #RRGGBB, kept in range.
std::optional<double> parseValue(const PreferenceInfo &info, const std::wstring &text)
{
    double value = 0;
    if (info.control == PreferenceControl::Color) {
        const std::wstring digits = text.starts_with(L'#') ? text.substr(1) : text;
        if (digits.size() != 6 || !std::ranges::all_of(digits, [](wchar_t digit) { return std::iswxdigit(digit) != 0; }))
            return std::nullopt;
        value = static_cast<double>(std::wcstoul(digits.c_str(), nullptr, 16));
    } else {
        wchar_t *end = nullptr;
        value = std::wcstod(text.c_str(), &end);
        if (end == text.c_str())
            return std::nullopt;
    }
    return std::clamp(value, info.minimum, info.maximum);
}

// Figma text_input (4046:12353): a value, typed. Each edit that reads as one sets it; once the field
// loses the focus it shows the value written its way again. A colour's shows its swatch.
class ValueField : public TextField
{
public:
    ValueField(const PreferenceInfo &info, std::function<void(double)> commit)
        : TextField(L"", TextFieldKind::Value)
        , m_info(info)
    {
        setPreferredSize({fieldWidth, fieldHeight});
        setOnChanged([this, commit = std::move(commit)] {
            if (const std::optional<double> value = parseValue(m_info, text())) {
                m_value = *value;
                commit(*value);
            }
        });
    }

    void show(double value)
    {
        m_value = value;
        if (!hasFocus())
            setText(formatValue(m_info, value));
        invalidate();
    }

    void focusChanged() override
    {
        TextField::focusChanged();
        if (!hasFocus())
            setText(formatValue(m_info, m_value));
    }

    void paint(Canvas &canvas) override
    {
        TextField::paint(canvas);
        if (m_info.control == PreferenceControl::Color) {
            canvas.fillRoundedRect({frame().width - swatchInset - swatchSize, (frame().height - swatchSize) / 2, swatchSize, swatchSize},
                                   2, Color::hex(static_cast<std::uint32_t>(std::lround(m_value))));
        }
    }

private:
    const PreferenceInfo &m_info;
    double m_value = 0;
};

// Figma text_input holding a choice (4046:12356): the chosen name; a click lists them all, the chosen
// one checked, to choose another.
class ChoiceField : public View
{
public:
    ChoiceField(const CommandRegistry &commands, std::function<std::vector<std::wstring>()> names, std::function<int()> chosen,
                std::function<void(int)> choose)
        : m_commands(commands)
        , m_names(std::move(names))
        , m_chosen(std::move(chosen))
        , m_choose(std::move(choose))
    {
        setPreferredSize({fieldWidth, fieldHeight});
    }

    bool takesMouse() const override { return true; }
    Cursor cursor() const override { return Cursor::Hand; }

    void paint(Canvas &canvas) override
    {
        const Palette &palette = canvas.palette();
        canvas.fillRoundedRect(bounds(), 6, palette.bgB2);
        canvas.fillRoundedRect(bounds(), 6, palette.hoverOverlay.faded(hoverAmount()));
        const std::vector<std::wstring> names = m_names();
        const int chosen = m_chosen();
        if (chosen >= 0 && chosen < static_cast<int>(names.size()))
            canvas.drawText(names[chosen], {fieldTextInset, 0, frame().width - 2 * fieldTextInset, frame().height}, choiceStyle,
                            palette.fgA2);
    }

    void mouseReleased(Point, bool inside) override
    {
        if (!inside)
            return;
        const std::vector<std::wstring> names = m_names();
        const int chosen = m_chosen();
        std::vector<MenuItem> items;
        for (int i = 0; i < static_cast<int>(names.size()); ++i)
            items.push_back({.label = names[i], .run = [choose = m_choose, i] { choose(i); }, .checked = i == chosen});
        Menu::open(*window(), mapToWindow(bounds()), PopupPlacement::Below, std::move(items), m_commands, frame().width);
    }

private:
    const CommandRegistry &m_commands;
    std::function<std::vector<std::wstring>()> m_names;
    std::function<int()> m_chosen;
    std::function<void(int)> m_choose;
};

// A setting's line (Figma "Frame 60"): its name 4 in, then its control at the end.
Row &labelledRow(Column &into, std::wstring_view label, float height = rowHeight)
{
    auto &row = into.add<Row>(10.0f);
    row.setPadding({.left = 4});
    row.setPreferredSize({0, height});
    row.setFillsWidth(true);
    row.add<Label>(std::wstring(label), labelStyle, &Palette::fgA3).setFillsWidth(true);
    return row;
}

// A tab's settings, scrolling, kept in step with the preset in force.
class PreferencesPage : public ScrollView
{
public:
    explicit PreferencesPage(const BarContext &context)
        : m_context(context)
        , m_preset(context.preset)
    {
        m_content = &setContent<Column>();
        m_content->setPadding({.bottom = 20});
        m_presetSubscription = m_preset.changed.connect([this] { sync(); });
    }

    // A heading (Figma "Defaults", 1711:4103), 20 below what's above it.
    void heading(std::wstring_view text)
    {
        auto &frame = m_content->add<Column>();
        frame.setPadding({.left = 24, .top = 20, .right = 24});
        frame.setFillsWidth(true);
        frame.add<Label>(std::wstring(text), headingStyle, &Palette::fgA1).setFillsWidth(true);
    }

    // The settings under a heading, 20 in from the sides.
    Column &section()
    {
        auto &section = m_content->add<Column>(5.0f);
        section.setPadding({.left = 20, .top = 10, .right = 20, .bottom = 10});
        section.setFillsWidth(true);
        return section;
    }

    // A preference's setting, drawn by its control, dimmed when it can't change.
    void setting(Column &into, Preference preference)
    {
        const PreferenceInfo &info = preferenceInfo(preference);
        build(isDisabled(info) ? disabledArea(into, disabledReason(info.timing)) : into, preference);
    }

    // A filter group (Figma "Essential" and the others, 1065:3941): its name, then its settings in an
    // Expander.
    Column &group(Column &into, std::wstring_view label, bool enabled = true)
    {
        Column &block = enabled ? into : disabledArea(into, disabledReason(PreferenceTiming::Later));
        labelledRow(block, label, labelRowHeight);
        return expander(block);
    }

    // Settings in an Expander; opening or closing it changes the page's height.
    Column &expander(Column &into)
    {
        auto &expander = into.add<Expander>();
        expander.setFillsWidth(true);
        expander.setOnToggled([this] { layout(); });
        return expander.content();
    }

    // A dimmed area for settings that can't change, saying why.
    Column &disabledArea(Column &into, std::wstring reason)
    {
        auto &area = into.add<DisabledArea>(std::move(reason));
        area.setFillsWidth(true);
        auto &content = area.setContent<Column>(5.0f);
        return content;
    }

    // Which track of `kind` plays (Figma "Audio Track", "Subtitle Track"); for subtitles, the files
    // added too.
    void trackChoice(Column &into, std::wstring_view label, TrackKind kind)
    {
        Player &player = m_context.player;
        const auto choices = [&player, kind] {
            std::vector<Track> tracks = player.tracks(kind);
            if (kind == TrackKind::Subtitle)
                tracks.insert(tracks.end(), player.externalSubtitles().begin(), player.externalSubtitles().end());
            return tracks;
        };
        auto &field = labelledRow(into, label).add<ChoiceField>(
            m_context.commands,
            [choices] {
                std::vector<std::wstring> names;
                for (const Track &track : choices())
                    names.push_back(track.name);
                return names;
            },
            [choices, &player, kind] {
                const std::vector<Track> tracks = choices();
                const auto found = std::ranges::find(tracks, player.currentTrack(kind), &Track::id);
                return found == tracks.end() ? -1 : static_cast<int>(found - tracks.begin());
            },
            [choices, &player, kind](int index) { player.selectTrack(kind, choices()[index].id); });
        m_trackSubscriptions.push_back(player.changed.connect([&field] { field.invalidate(); }));
    }

    // A toggle beside a colour typed in a field (Figma "Color Extraction" 614:1576).
    void colorToggle(Column &into, Preference toggle, Preference color)
    {
        auto &row = labelledRow(into, preferenceInfo(toggle).label);
        auto &field = row.add<ValueField>(preferenceInfo(color), [this, color](double value) { m_preset.set(color, value); });
        field.setPreferredSize({100, fieldHeight});
        auto &onOff = row.add<Toggle>(false);
        onOff.setOnChanged([this, toggle](bool on) { m_preset.set(toggle, on); });
        m_syncs.push_back([this, &field, &onOff, toggle, color] {
            field.show(m_preset.values()[color]);
            onOff.setOn(m_preset.values()[toggle] != 0);
        });
    }

    // Two settings side by side, as Figma's Crop group lays out its borders (543:995).
    void pair(Column &into, Preference left, Preference right)
    {
        auto &row = into.add<Row>(20.0f);
        row.setFillsWidth(true);
        for (const Preference preference : {left, right}) {
            auto &half = row.add<Column>();
            half.setFillsWidth(true);
            auto &line = labelledRow(half, preferenceInfo(preference).label);
            auto &field = line.add<ValueField>(preferenceInfo(preference),
                                               [this, preference](double value) { m_preset.set(preference, value); });
            field.setPreferredSize({76, fieldHeight});
            m_syncs.push_back([this, &field, preference] { field.show(m_preset.values()[preference]); });
        }
    }

    // A setting `indent` further in, as Figma's gradient mode sits under its toggle (673:1672).
    void indented(Column &into, Preference preference, float indent)
    {
        auto &inner = into.add<Column>();
        inner.setPadding({.left = indent});
        inner.setFillsWidth(true);
        setting(inner, preference);
    }

    // Shows only the settings `visible` says, as the font size follows Size Values.
    void showWhen(View &view, std::function<bool()> visible)
    {
        m_syncs.push_back([this, &view, visible = std::move(visible)] {
            if (view.isVisible() == visible())
                return;
            view.setVisible(visible());
            layout();
        });
    }

    ActivePreset &preset() const { return m_preset; }
    // Keeps something built outside the settings above in step with the preset.
    void addSync(std::function<void()> update) { m_syncs.push_back(std::move(update)); }

    void sync()
    {
        for (const auto &update : m_syncs)
            update();
    }

private:
    // Builds the setting's row into `into`, by its control.
    void build(Column &into, Preference preference)
    {
        const PreferenceInfo &info = preferenceInfo(preference);
        const auto set = [this, preference](double value) { m_preset.set(preference, value); };
        const auto value = [this, preference] { return m_preset.values()[preference]; };
        switch (info.control) {
        case PreferenceControl::Toggle: {
            auto &toggle = labelledRow(into, info.label).add<Toggle>(value() != 0);
            toggle.setOnChanged([set](bool on) { set(on); });
            m_syncs.push_back([&toggle, value] { toggle.setOn(value() != 0); });
            break;
        }
        case PreferenceControl::Dropdown: {
            // A dropdown's line has 20 more under it (Figma "Frame 114").
            Row &row = labelledRow(into, info.label, dropdownHeight + 20);
            row.setPadding({.left = 4, .bottom = 20});
            auto &dropdown = row.add<Dropdown>(info.names, static_cast<int>(value()));
            dropdown.setPreferredSize({dropdownWidth, dropdownHeight});
            dropdown.setOnChanged([set](int index) { set(index); });
            m_syncs.push_back([&dropdown, value] { dropdown.setSelected(static_cast<int>(value())); });
            break;
        }
        case PreferenceControl::Choice: {
            labelledRow(into, info.label)
                .add<ChoiceField>(
                    m_context.commands, [&info] { return info.names; }, [value] { return static_cast<int>(value()); },
                    [set](int index) { set(index); });
            break;
        }
        case PreferenceControl::Options: {
            // Two options sit at the end of the line (Figma "Size Values"); more take a line of their
            // own under the name (Figma "Flip").
            const bool inLine = info.names.size() <= 2;
            Segmented *options = nullptr;
            if (inLine) {
                options = &labelledRow(into, info.label, 50).add<Segmented>(info.names, static_cast<int>(value()));
                options->setPreferredSize({180, 38});
            } else {
                auto &block = into.add<Column>(20.0f);
                block.setPadding({.top = 6, .bottom = 6});
                block.setFillsWidth(true);
                auto &label = block.add<Label>(std::wstring(info.label), labelStyle, &Palette::fgA3);
                label.setPadding(4, 4);
                label.setFillsWidth(true);
                options = &block.add<Segmented>(info.names, static_cast<int>(value()));
                options->setPreferredSize({0, 44});
                options->setFillsWidth(true);
            }
            options->setOnChanged([set](int index) { set(index); });
            m_syncs.push_back([options, value] { options->setSelected(static_cast<int>(value())); });
            break;
        }
        case PreferenceControl::Number:
        case PreferenceControl::Color: {
            auto &field = labelledRow(into, info.label).add<ValueField>(info, set);
            field.show(value());
            m_syncs.push_back([&field, value] { field.show(value()); });
            break;
        }
        case PreferenceControl::Slider: {
            auto &block = into.add<Column>(5.0f);
            block.setFillsWidth(true);
            // Its value typed in a field, or its step's name, or neither (Figma's Essential sliders).
            Row &line = labelledRow(block, info.label, info.format.empty() ? labelRowHeight : 56);
            ValueField *field = nullptr;
            Label *stepName = nullptr;
            if (!info.format.empty()) {
                field = &line.add<ValueField>(info, set);
            } else if (!info.names.empty()) {
                line.setPadding({.left = 4, .right = 4});
                stepName = &line.add<Label>(L"", labelStyle, &Palette::fgA3);
            }
            auto &slider = block.add<Slider>(static_cast<float>(info.minimum), static_cast<float>(info.maximum),
                                             static_cast<float>(info.step), static_cast<float>(value()), info.ticks);
            slider.setFillsWidth(true);
            slider.setOnChanged([set](float changed) { set(changed); });
            m_syncs.push_back([&slider, field, stepName, &info, value] {
                slider.setValue(static_cast<float>(value()));
                if (field)
                    field->show(value());
                if (stepName)
                    stepName->setText(info.names[static_cast<size_t>(std::lround(value()))]);
            });
            break;
        }
        }
    }

    const BarContext &m_context;
    ActivePreset &m_preset;
    Column *m_content;
    std::vector<std::function<void()>> m_syncs;
    Subscription m_presetSubscription;
    std::vector<Subscription> m_trackSubscriptions;
};

void buildPlayback(PreferencesPage &page)
{
    Column &section = page.section();
    for (const Preference preference : {SubtitleTimeline, Statistics, Reverse, SleepTimer, AudioVisualizer, Speed})
        page.setting(section, preference);
}

void buildVideo(PreferencesPage &page)
{
    page.heading(L"Defaults");
    Column &defaults = page.section();
    for (const Preference preference : {Flip, Rotate, Zoom, Hdr, Resolution, AspectRatio, Crop, Render})
        page.setting(defaults, preference);

    page.heading(L"Filters");
    Column &filters = page.section();
    Column &essential = page.group(filters, L"Essential");
    for (const Preference preference : {Hue, Brightness, Contrast, Saturation, Gamma, Sharpen, BandingRemoval, FilmGrain})
        page.setting(essential, preference);
    Column &crop = page.group(filters, L"Crop");
    page.pair(crop, CropTop, CropBottom);
    page.pair(crop, CropLeft, CropRight);
    Column &colors = page.group(filters, L"Colors");
    page.colorToggle(colors, ColorExtraction, ExtractedColor);
    page.colorToggle(colors, ColorThreshold, ThresholdColor);
    page.setting(colors, ThresholdSaturation);
    page.setting(colors, ThresholdSimilarity);
    page.setting(colors, NegateColors);
    page.setting(colors, Posterize);
    page.setting(colors, Gradient);
    page.indented(colors, GradientMode, 18);
    page.setting(colors, GradientColor);
    page.setting(colors, Cartoon);
    page.setting(colors, SepiaIntensity);
    // Figma draws these groups closed only; what they hold comes with their design.
    for (const wchar_t *name : {L"Geometry", L"Overlay", L"Advanced"})
        page.group(filters, name, false);
}

void buildAudio(PreferencesPage &page)
{
    page.heading(L"Defaults");
    Column &defaults = page.section();
    page.trackChoice(defaults, L"Audio Track", TrackKind::Audio);
    page.setting(defaults, AudioLanguage);
    // Stereo Mode's test tone isn't something libVLC 3 plays (Figma "Test" 2250:9842).
    Row &stereo = labelledRow(defaults, preferenceInfo(StereoMode).label);
    auto &test = page.disabledArea(stereo.add<Column>(), disabledReason(PreferenceTiming::Unavailable))
                     .add<Button>(L"Test", ButtonKind::Secondary, TextStyle{.size = 14});
    test.setPreferredSize({68, 32});
    auto &stereoMode = defaults.add<Dropdown>(preferenceInfo(StereoMode).names, static_cast<int>(page.preset().values()[StereoMode]));
    stereoMode.setPreferredSize({0, dropdownHeight});
    stereoMode.setFillsWidth(true);
    stereoMode.setOnChanged([&page](int index) { page.preset().set(StereoMode, index); });

    page.heading(L"Synchronization");
    page.setting(page.section(), AudioDelay);

    page.heading(L"Filters");
    Column &filters = page.section();
    labelledRow(filters, preferenceInfo(EqualizerPreset).label, labelRowHeight);
    auto &equalizerPreset =
        filters.add<Dropdown>(preferenceInfo(EqualizerPreset).names, static_cast<int>(page.preset().values()[EqualizerPreset]));
    equalizerPreset.setPreferredSize({0, dropdownHeight});
    equalizerPreset.setFillsWidth(true);
    equalizerPreset.setOnChanged([&page](int index) { page.preset().set(EqualizerPreset, index); });
    Column &bands = page.expander(filters);
    for (const Preference preference :
         {Preamp, Band60, Band170, Band310, Band600, Band1k, Band3k, Band6k, Band12k, Band14k, Band16k})
        page.setting(bands, preference);
    page.setting(filters, TwoPass);
    // Each has its settings in a group under it, for when libVLC can apply it.
    for (const Preference preference : {Compressor, Spatializer, StereoWidener}) {
        Column &area = page.disabledArea(filters, disabledReason(PreferenceTiming::Unavailable));
        labelledRow(area, preferenceInfo(preference).label).add<Toggle>(false);
        area.add<Expander>().setFillsWidth(true);
    }
    page.setting(filters, Pitch);

    page.addSync([&page, &stereoMode, &equalizerPreset] {
        stereoMode.setSelected(static_cast<int>(page.preset().values()[StereoMode]));
        equalizerPreset.setSelected(static_cast<int>(page.preset().values()[EqualizerPreset]));
    });
}

void buildSubtitles(PreferencesPage &page)
{
    page.heading(L"Defaults");
    Column &defaults = page.section();
    page.trackChoice(defaults, L"Subtitle Track", TrackKind::Subtitle);
    page.setting(defaults, SubtitleLanguage);

    page.heading(L"Synchronization");
    Column &synchronization = page.section();
    for (const Preference preference : {SubtitleDelay, SubtitleSpeed, SubtitleDurationFactor})
        page.setting(synchronization, preference);

    page.heading(L"Appearance");
    Column &appearance = page.section();
    for (const Preference preference : {EmbeddedSubFormat, SubtitleFont, TextColor, OutlineColor, SubtitleOffset, SizeValues})
        page.setting(appearance, preference);
    // The font size is relative or in pixels, as Size Values says.
    for (const Preference preference : {RelativeFontSize, PixelFontSize}) {
        auto &holder = appearance.add<Column>();
        holder.setFillsWidth(true);
        page.setting(holder, preference);
        const double sizeValues = preference == RelativeFontSize ? 0 : 1;
        page.showWhen(holder, [&page, sizeValues] { return page.preset().values()[SizeValues] == sizeValues; });
    }
    for (const Preference preference : {OutlineThickness, BackgroundOpacity})
        page.setting(appearance, preference);
}

} // namespace

PreferencesPanel::PreferencesPanel(const BarContext &context, PreferencesTab tab)
    : Column(1)
{
    const CommandRegistry &commands = context.commands;
    ActivePreset &preset = context.preset;

    m_tabs = &add<Tabs>(
        std::vector<Tab>{{L"playback", L"Playback"}, {L"video", L"Video"}, {L"audio", L"Audio"}, {L"subtitles", L"Subtitles"}},
        static_cast<int>(tab));
    m_tabs->setFillsWidth(true);
    m_tabs->setOnChanged([this](int index) { showTab(static_cast<PreferencesTab>(index)); });

    const std::array builders{&buildPlayback, &buildVideo, &buildAudio, &buildSubtitles};
    for (size_t i = 0; i < m_pages.size(); ++i) {
        auto &page = add<PreferencesPage>(context);
        builders[i](page);
        page.sync();
        page.setFillsWidth(true);
        page.setFillsHeight(true);
        m_pages[i] = &page;
    }

    // Figma "Frame 68" (812:1580): the preset in force, then the settings window.
    auto &footer = add<Column>(10.0f);
    footer.setPadding({20, 20, 20, 20});
    footer.setFillsWidth(true);
    const auto presetOptions = [&preset] {
        std::vector<std::wstring> options = preset.names();
        const int chosen = static_cast<int>(std::ranges::find(options, preset.name()) - options.begin());
        options.emplace_back(L"New Preset");
        return std::pair{options, chosen};
    };
    const auto [options, chosen] = presetOptions();
    auto &presets = footer.add<Dropdown>(options, chosen);
    presets.setPreferredSize({0, 42});
    presets.setFillsWidth(true);
    presets.setOnChanged([&preset](int index) {
        const std::vector<std::wstring> names = preset.names();
        if (index < static_cast<int>(names.size()))
            preset.choose(names[index]);
        else
            preset.createPreset();
    });
    // A new preset joins the list; the dropdown shows the one in force.
    m_presetSubscription = preset.changed.connect([&presets, presetOptions] {
        auto [names, index] = presetOptions();
        presets.setOptions(std::move(names), index);
    });
    auto &advanced = footer.add<Button>(L"Open Advanced Settings", ButtonKind::Neutral, TextStyle{.size = 14}, L"settings");
    advanced.setPreferredSize({0, 32});
    advanced.setFillsWidth(true);
    advanced.setEnabled(commands.canRun(CommandId::Settings));
    advanced.setOnClick([&commands] { commands.run(CommandId::Settings); });

    showTab(tab);
}

PreferencesTab PreferencesPanel::tab() const
{
    return static_cast<PreferencesTab>(m_tabs->active());
}

void PreferencesPanel::showTab(PreferencesTab tab)
{
    m_tabs->setActive(static_cast<int>(tab));
    for (size_t i = 0; i < m_pages.size(); ++i)
        m_pages[i]->setVisible(i == static_cast<size_t>(tab));
    layout();
    // The control bar's settings button shows the panel is open.
    if (Window *host = window())
        host->rootWindow().invalidate();
}

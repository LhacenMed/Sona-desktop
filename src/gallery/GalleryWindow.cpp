#include "gallery/GalleryWindow.h"

#include <array>
#include <string>
#include <vector>

#include "graphics/Canvas.h"
#include "settings/AppSettings.h"
#include "style/Theme.h"
#include "widgets/Button.h"
#include "widgets/DialogWindow.h"
#include "widgets/Dropdown.h"
#include "widgets/IconButton.h"
#include "widgets/Label.h"
#include "widgets/ListRow.h"
#include "widgets/Overlay.h"
#include "widgets/Popover.h"
#include "widgets/ScrollView.h"
#include "widgets/Segmented.h"
#include "widgets/Slider.h"
#include "widgets/Stack.h"
#include "widgets/Tabs.h"
#include "widgets/TextField.h"
#include "widgets/Toggle.h"

namespace {

constexpr TextStyle headingStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_SEMI_BOLD};
constexpr TextStyle buttonStyle{.size = 14, .weight = DWRITE_FONT_WEIGHT_SEMI_BOLD};

template <typename T>
T &sized(T &view, Size size)
{
    view.setPreferredSize(size);
    return view;
}

// The queue rows of Figma 463:846, the first one playing.
class SampleQueue : public Column
{
public:
    SampleQueue() : Column(12)
    {
        constexpr std::array<std::array<const wchar_t *, 2>, 4> items{{
            {L"The Wand_S01E11.mp4", L"23:06"},
            {L"The Wand_S01E12.mp4", L"24:18"},
            {L"The Wand_S01E13.mp4", L"25:00"},
            {L"The Wand_S01E14.mp4", L"22:35"},
        }};
        for (const auto &[name, duration] : items) {
            ListRow &row = add<ListRow>(L"move", name, duration);
            row.setFillsWidth(true);
            row.setOnClick([this, &row] { select(row); });
            m_rows.push_back(&row);
        }
        m_rows.front()->setSelected(true);
    }

private:
    void select(ListRow &chosen)
    {
        for (ListRow *row : m_rows)
            row->setSelected(row == &chosen);
    }

    std::vector<ListRow *> m_rows;
};

// A control-bar panel: tabs over the queue.
class SamplePopover : public Popover
{
public:
    SamplePopover() : Popover(10)
    {
        setPreferredSize({400, 0});
        setPadding({0, 0, 0, 20});
        add<Tabs>(std::vector<Tab>{{L"playback", L"Playback"}, {L"audio", L"Audio"}, {L"subtitles", L"Subtitles"}}, 0)
            .setFillsWidth(true);
        auto &queue = add<SampleQueue>();
        queue.setPreferredSize({360, 0});
    }
};

// The band Windows draws its caption buttons in, which moves the window.
class CaptionBand : public View
{
public:
    bool takesMouse() const override { return true; }
    HitRole hitRole() const override { return HitRole::Caption; }
};

class GalleryView : public View
{
public:
    GalleryView(Window &window, Theme &theme, SettingsStore &settings)
    {
        m_caption = &add<CaptionBand>();
        auto &scroll = add<ScrollView>();
        m_scroll = &scroll;
        auto &page = scroll.setContent<Column>(28.0f);
        page.setPadding({32, 32, 32, 32});
        page.setAlignment(Align::Start);

        Row &themes = section(page, L"Theme (Segmented, 999:4556)");
        auto &theme3 = sized(themes.add<Segmented>(std::vector<std::wstring>{L"Light", L"Dark", L"System"},
                                                    static_cast<int>(settings.get(AppSettings::theme))),
                             {320, 36});
        theme3.setOnChanged([&settings](int index) { settings.set(AppSettings::theme, static_cast<ThemeMode>(index)); });

        Row &buttons = section(page, L"Buttons (6216:13749, 500:3063)");
        sized(buttons.add<Button>(L"Save", ButtonKind::Primary, buttonStyle), {100, 32});
        sized(buttons.add<Button>(L"Save", ButtonKind::Primary, buttonStyle), {100, 32}).setEnabled(false);
        sized(buttons.add<Button>(L"Cancel", ButtonKind::Secondary, buttonStyle), {100, 32});
        sized(buttons.add<Button>(L"Show Advanced Settings", ButtonKind::Neutral, TextStyle{.size = 14}, L"settings"), {360, 32});

        Row &toggles = section(page, L"Toggle (536:1014)");
        toggles.add<Toggle>(true);
        toggles.add<Toggle>(false);

        Row &segments = section(page, L"Segmented (804:1208, 999:4549)");
        sized(segments.add<Segmented>(std::vector<std::wstring>{L"Relative", L"Pixels"}, 0), {213, 44});
        sized(segments.add<Segmented>(std::vector<std::wstring>{L"1", L"2", L"3"}, 1), {213, 44});

        Row &dropdowns = section(page, L"Dropdown (2372:8300)");
        sized(dropdowns.add<Dropdown>(std::vector<std::wstring>{L"Disabled", L"Spectrometer", L"Scope", L"Spectrum", L"Waveform"}, 0),
              {220, 48});
        sized(dropdowns.add<Dropdown>(std::vector<std::wstring>{L"Off", L"15 minutes", L"30 minutes", L"1 hour", L"End of file"}, 0),
              {364, 48});

        Row &sliders = section(page, L"Slider (5187:24544)");
        sized(sliders.add<Slider>(0.25f, 2.0f, 0.25f, 1.0f,
                                  std::vector<std::wstring>{L"0.25", L"0.5", L"0.75", L"1", L"1.25", L"1.5", L"1.75", L"2"}),
              {360, 0});
        sized(sliders.add<Slider>(-10.0f, 10.0f, 0.1f, 0.0f), {360, 0});

        Row &tabs = section(page, L"Tabs (5187:24309)");
        sized(tabs.add<Tabs>(std::vector<Tab>{{L"playback", L"Playback"}, {L"audio", L"Audio"}, {L"subtitles", L"Subtitles"}}, 0),
              {400, 50});

        Row &rows = section(page, L"List rows (463:846)");
        sized(rows.add<SampleQueue>(), {360, 0});

        Row &fields = section(page, L"Text fields (1040:2512, 6251:13904)");
        sized(fields.add<TextField>(L"Search", TextFieldKind::Search), {200, 34});
        sized(fields.add<TextField>(L"e.g., Favorite TV Channels"), {400, 37});

        Row &layers = section(page, L"Tooltip, popover and dialog");
        layers.add<IconWrapper>(L"settings").setTooltip(L"Settings");
        auto &popoverButton = sized(layers.add<Button>(L"Open popover", ButtonKind::Secondary, buttonStyle), {140, 32});
        popoverButton.setOnClick([&popoverButton] { popoverButton.window()->overlay().openPopover<SamplePopover>(popoverButton); });
        auto &dialogButton = sized(layers.add<Button>(L"Open dialog", ButtonKind::Secondary, buttonStyle), {140, 32});
        dialogButton.setOnClick([&window, &theme] { openSampleDialog(window, theme); });
    }

    void layout() override
    {
        const float top = window() ? window()->captionHeight() : 0;
        m_caption->setFrame({0, 0, frame().width, top});
        m_scroll->setFrame({0, top, frame().width, frame().height - top});
    }

    void paint(Canvas &canvas) override
    {
        // Windows paints the caption; the page is below it.
        canvas.clear(m_caption->frame(), Color{});
        canvas.fillRect(m_scroll->frame(), canvas.palette().bgB1);
    }

private:
    // A heading and the row its samples go in.
    static Row &section(Column &page, const wchar_t *title)
    {
        auto &section = page.add<Column>(12.0f);
        section.setAlignment(Align::Start);
        section.add<Label>(title, headingStyle, &Palette::fg2);
        auto &samples = section.add<Row>(16.0f);
        samples.setAlignment(Align::Start);
        return samples;
    }

    // Figma's "Open Network Stream" dialog (6216:13614): Save is enabled once there's a URL.
    static void openSampleDialog(Window &window, Theme &theme)
    {
        auto &dialog = window.openWindow<DialogWindow>(window.graphics(), theme, window.hwnd(), L"Open Network Stream",
                                                       Size{1048, 631});
        Column &content = dialog.content();
        content.add<Label>(L"Network URL", TextStyle{.size = 16}, &Palette::fg2);
        auto &url = content.add<TextField>(L"e.g., https://www.example.com/stream.avi");
        url.setFillsWidth(true);
        url.setOnChanged([&dialog, &url] { dialog.setSaveEnabled(!url.text().empty()); });
        content.add<Label>(L"Playlist Name (optional)", TextStyle{.size = 16}, &Palette::fg2);
        content.add<TextField>(L"e.g., Favorite TV Channels").setFillsWidth(true);
        dialog.setSaveEnabled(false);
        dialog.show(false);
    }

    View *m_caption;
    ScrollView *m_scroll;
};

} // namespace

GalleryWindow::GalleryWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings)
    : Window(graphics, theme)
{
    create(L"Sona widgets", {1000, 800}, {640, 480}, WindowFrame::Resizable, nullptr);
    setRoot(std::make_unique<GalleryView>(*this, theme, settings));
    setNativeCaption(&Palette::bgB1);
    show(false);
}

void GalleryWindow::destroyed()
{
    PostQuitMessage(0);
}

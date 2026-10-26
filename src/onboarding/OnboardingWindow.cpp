#include "onboarding/OnboardingWindow.h"

#include <array>
#include <functional>

#include "graphics/Canvas.h"
#include "onboarding/ShortcutsPage.h"
#include "onboarding/ThemePage.h"
#include "onboarding/WelcomePage.h"
#include "settings/AppSettings.h"
#include "style/Theme.h"
#include "widgets/Button.h"

namespace {

// The empty band above every page moves the window, like a title bar.
class DragBand : public View
{
public:
    bool takesMouse() const override { return true; }
    HitRole hitRole() const override { return HitRole::Caption; }
};

// The steps, one at a time, with Back and Next at the bottom right (Figma 1902:3383, 6696:20706).
class OnboardingView : public View
{
public:
    OnboardingView(Theme &theme, SettingsStore &settings, Player &player, const CommandRegistry &commands,
                   std::function<void()> finish)
        : m_dragBand(add<DragBand>())
        , m_pages{&add<WelcomePage>(settings), &add<ThemePage>(theme, settings, player, commands), &add<ShortcutsPage>(commands)}
        , m_back(add<Button>(L"Back", ButtonKind::Secondary, buttonStyle))
        , m_next(add<Button>(L"Next", ButtonKind::Primary, buttonStyle))
    {
        m_back.setOnClick([this] { showPage(m_page - 1); });
        m_next.setOnClick([this, finish = std::move(finish)] {
            if (m_page == lastPage)
                finish();
            else
                showPage(m_page + 1);
        });
        showPage(0);
    }

    void layout() override
    {
        m_dragBand.setFrame({0, 0, frame().width, 48});
        for (View *page : m_pages)
            page->setFrame(bounds());
        m_back.setFrame({820, 671, 100, 32});
        m_next.setFrame({928, 671, 100, 32});
    }

    void paint(Canvas &canvas) override { canvas.fillRect(bounds(), canvas.palette().bgB1); }

private:
    static constexpr TextStyle buttonStyle{.size = 16, .weight = DWRITE_FONT_WEIGHT_SEMI_BOLD};
    static constexpr int lastPage = 2;

    void showPage(int page)
    {
        m_page = page;
        for (int i = 0; i <= lastPage; ++i)
            m_pages[i]->setVisible(i == page);
        m_back.setVisible(page > 0);
        m_next.setText(page == lastPage ? L"Finish" : L"Next");
    }

    DragBand &m_dragBand;
    std::array<View *, lastPage + 1> m_pages;
    Button &m_back;
    Button &m_next;
    int m_page = 0;
};

} // namespace

OnboardingWindow::OnboardingWindow(GraphicsDevice &graphics, Theme &theme, SettingsStore &settings, Player &player,
                                   const CommandRegistry &commands, HWND owner)
    : Window(graphics, theme)
{
    create(L"Welcome to " SONA_APP_NAME, {1090, 759}, {1090, 759}, WindowFrame::Fixed, owner);
    setRoot(std::make_unique<OnboardingView>(theme, settings, player, commands, [this, &settings] {
        settings.set(AppSettings::onboardingCompleted, true);
        close();
    }));
    show(false);
}

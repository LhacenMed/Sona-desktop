#include "WindowCommands.h"

#ifdef Q_OS_WIN
#include <windows.h>

namespace {

void sendSystemCommand(QWindow *window, WPARAM command)
{
    PostMessageW(reinterpret_cast<HWND>(window->winId()), WM_SYSCOMMAND, command, 0);
}

} // namespace
#endif

void WindowCommands::minimize(QWindow *window)
{
#ifdef Q_OS_WIN
    sendSystemCommand(window, SC_MINIMIZE);
#else
    window->showMinimized();
#endif
}

void WindowCommands::toggleMaximized(QWindow *window)
{
    const bool maximized = window->visibility() == QWindow::Maximized;
#ifdef Q_OS_WIN
    sendSystemCommand(window, maximized ? SC_RESTORE : SC_MAXIMIZE);
#else
    maximized ? window->showNormal() : window->showMaximized();
#endif
}

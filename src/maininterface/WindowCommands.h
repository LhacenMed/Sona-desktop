#pragma once

#include <QObject>
#include <QWindow>
#include <QtQml/qqmlregistration.h>

// Minimizes and maximizes windows the way the native caption buttons do, so the system applies
// its own behavior: the animations, and restoring a minimized window to the maximized state it left.
class WindowCommands : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    using QObject::QObject;

    Q_INVOKABLE void minimize(QWindow *window);
    Q_INVOKABLE void toggleMaximized(QWindow *window);
};

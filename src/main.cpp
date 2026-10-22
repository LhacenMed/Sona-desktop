#include <QDir>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QUrl>

#include <QWKQuick/qwkquickglobal.h>

#include <clocale>

#ifdef Q_OS_WIN
#include <shobjidl.h>
#endif

#include "player/PlayerController.h"
#include "style/ThemedSvgProvider.h"

int main(int argc, char *argv[])
{
    // libmpv renders through OpenGL, so the scene graph must use it too.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

#ifdef Q_OS_WIN
    // Groups the window, taskbar pins and installer shortcuts under one taskbar identity.
    SetCurrentProcessExplicitAppUserModelID(L"" SONA_APP_USER_MODEL_ID);
#endif

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral(SONA_PUBLISHER));
    QGuiApplication::setApplicationName(QStringLiteral(SONA_APP_NAME));
    QGuiApplication::setApplicationVersion(QStringLiteral(SONA_VERSION));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/qt/qml/Sona/assets/logo/sona-logo.svg")));

    // libmpv requires the C numeric locale.
    std::setlocale(LC_NUMERIC, "C");

    const QDir fonts(QStringLiteral(":/qt/qml/Sona/assets/fonts"));
    for (const QString &font : fonts.entryList({QStringLiteral("*.ttf")}))
        QFontDatabase::addApplicationFont(fonts.filePath(font));
    QGuiApplication::setFont(QFont(QStringLiteral("Inter")));

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("themed"), new ThemedSvgProvider);
    QWK::registerTypes(&engine);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    const QStringList arguments = QCoreApplication::arguments();
    const QUrl media = arguments.size() > 1
            ? QUrl::fromUserInput(arguments.at(1), QDir::currentPath(), QUrl::AssumeLocalFile)
            : QUrl();
    engine.setInitialProperties({{QStringLiteral("opensMaximized"), !media.isEmpty()}});
    engine.loadFromModule("Sona", "MainWindow");

    if (!media.isEmpty())
        engine.singletonInstance<PlayerController *>("Sona", "Player")->open(media);

    return app.exec();
}

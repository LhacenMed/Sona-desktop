#pragma once

#include <QQuickImageProvider>

// Serves bundled SVGs recolored for the active theme:
// image://themed/<asset path>?<FROM>=<TO>&... replaces each #FROM color with #TO.
class ThemedSvgProvider : public QQuickImageProvider
{
public:
    ThemedSvgProvider();

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
};

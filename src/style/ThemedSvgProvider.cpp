#include "ThemedSvgProvider.h"

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <QUrlQuery>

ThemedSvgProvider::ThemedSvgProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ThemedSvgProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const qsizetype queryStart = id.indexOf(u'?');
    QFile file(QStringLiteral(":/qt/qml/Sona/") + id.left(queryStart));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    QByteArray svg = file.readAll();

    // Figma exports colors as uppercase hex.
    const QUrlQuery substitutions(id.mid(queryStart + 1));
    for (const auto &[from, to] : substitutions.queryItems())
        svg.replace('#' + from.toUpper().toLatin1(), '#' + to.toUpper().toLatin1());

    QSvgRenderer renderer(svg);
    *size = renderer.defaultSize();

    QImage image(requestedSize.isValid() ? requestedSize : renderer.defaultSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    renderer.render(&painter);
    return image;
}

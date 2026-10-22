#pragma once

#include <QQuickFramebufferObject>
#include <QtQml/qqmlregistration.h>

#include "PlayerController.h"

// Draws the player's video output into the scene graph through libmpv's OpenGL render API.
class VideoSurface : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(PlayerController *player READ player WRITE setPlayer NOTIFY playerChanged REQUIRED)

public:
    explicit VideoSurface(QQuickItem *parent = nullptr);

    PlayerController *player() const { return m_player; }
    void setPlayer(PlayerController *player);

    Renderer *createRenderer() const override;

signals:
    void playerChanged();

private:
    PlayerController *m_player = nullptr;
};

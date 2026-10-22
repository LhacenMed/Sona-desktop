#include "VideoSurface.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>

#include <mpv/render_gl.h>

namespace {

// Lives on the scene graph's render thread; owns the mpv render context for one surface.
class VideoRenderer : public QQuickFramebufferObject::Renderer
{
public:
    VideoRenderer(mpv_handle *mpv, VideoSurface *surface)
    {
        mpv_opengl_init_params glInit{
            .get_proc_address = [](void *, const char *name) -> void * {
                return reinterpret_cast<void *>(QOpenGLContext::currentContext()->getProcAddress(name));
            },
        };
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_API_TYPE, const_cast<char *>(MPV_RENDER_API_TYPE_OPENGL)},
            {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
            {MPV_RENDER_PARAM_INVALID, nullptr},
        };
        mpv_render_context_create(&m_context, mpv, params);

        // mpv signals new frames from its own thread; repaint the item on the GUI thread.
        mpv_render_context_set_update_callback(m_context, [](void *item) {
            QMetaObject::invokeMethod(static_cast<VideoSurface *>(item), &QQuickItem::update, Qt::QueuedConnection);
        }, surface);
    }

    ~VideoRenderer() override { mpv_render_context_free(m_context); }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        return new QOpenGLFramebufferObject(size);
    }

    void render() override
    {
        QOpenGLFramebufferObject *target = framebufferObject();
        mpv_opengl_fbo fbo{
            .fbo = static_cast<int>(target->handle()),
            .w = target->width(),
            .h = target->height(),
        };
        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
            {MPV_RENDER_PARAM_INVALID, nullptr},
        };
        mpv_render_context_render(m_context, params);
        QQuickOpenGLUtils::resetOpenGLState();
    }

private:
    mpv_render_context *m_context = nullptr;
};

} // namespace

VideoSurface::VideoSurface(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
{
}

void VideoSurface::setPlayer(PlayerController *player)
{
    if (m_player == player)
        return;
    m_player = player;
    emit playerChanged();
}

QQuickFramebufferObject::Renderer *VideoSurface::createRenderer() const
{
    return new VideoRenderer(m_player->handle(), const_cast<VideoSurface *>(this));
}

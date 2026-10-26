#include "graphics/BackdropCapture.h"

#include <Unknwn.h>
#include <dwmapi.h>
#include <DispatcherQueue.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <winrt/Windows.System.h>

#include <chrono>
#include <system_error>

#include "graphics/GraphicsDevice.h"
#include "platform/HResult.h"

using Microsoft::WRL::ComPtr;
using winrt::Windows::Graphics::SizeInt32;
using winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool;
using winrt::Windows::Graphics::Capture::GraphicsCaptureItem;
using winrt::Windows::Graphics::Capture::GraphicsCaptureSession;
using winrt::Windows::Graphics::DirectX::DirectXPixelFormat;
using winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice;

namespace {

constexpr DirectXPixelFormat pixelFormat = DirectXPixelFormat::B8G8R8A8UIntNormalized;
// How often the window is captured while no popup shows it: enough to stay close to what's there.
constexpr std::chrono::milliseconds warmInterval{500};
// Live, as often as the window changes.
constexpr std::chrono::milliseconds liveInterval{1};

// Frames arrive on the thread that captures, through its dispatcher queue, which the app's message
// loop runs: the UI thread, so the device is only ever used there.
void ensureDispatcherQueue()
{
    static winrt::Windows::System::DispatcherQueueController controller{nullptr};
    if (controller)
        return;
    const DispatcherQueueOptions options{sizeof(options), DQTYPE_THREAD_CURRENT, DQTAT_COM_NONE};
    check(CreateDispatcherQueueController(
        options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController **>(winrt::put_abi(controller))));
}

bool hasCaptureProperty(const wchar_t *name)
{
    return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession",
                                                                                  name);
}

// Whether Windows can capture a window without outlining it (Windows 11).
bool canCaptureWithoutBorder()
{
    return GraphicsCaptureSession::IsSupported() && hasCaptureProperty(L"IsBorderRequired");
}

// Whether a capture can be slowed down, so it can stay warm (Windows 11 24H2).
bool canThrottle()
{
    return hasCaptureProperty(L"MinUpdateInterval");
}

} // namespace

struct BackdropCapture::Session
{
    ~Session()
    {
        // No frame reaches the capture once it's gone.
        if (pool)
            pool.FrameArrived(frameArrived);
        if (session)
            session.Close();
        if (pool)
            pool.Close();
    }

    IDirect3DDevice device{nullptr};
    Direct3D11CaptureFramePool pool{nullptr};
    winrt::event_token frameArrived{};
    GraphicsCaptureSession session{nullptr};
    SizeInt32 size{};
};

BackdropCapture::BackdropCapture(GraphicsDevice &graphics, HWND window)
    : m_graphics(graphics)
    , m_window(window)
{
    // A new device can't use frames of the old one.
    m_deviceReset = graphics.deviceReset.connect([this] {
        m_session.reset();
        m_image.Reset();
        if (m_live || canThrottle())
            start();
    });
    if (canThrottle())
        start();
}

void BackdropCapture::setLive(bool live)
{
    m_live = live;
    if (canThrottle())
        applyRate();
    else if (live)
        start();
    else
        m_session.reset();
}

void BackdropCapture::applyRate()
{
    if (m_session)
        m_session->session.MinUpdateInterval(m_live ? liveInterval : warmInterval);
}

BackdropCapture::~BackdropCapture() = default;

void BackdropCapture::start()
{
    if (m_session || !canCaptureWithoutBorder())
        return;
    // Capture can still be refused (by policy, for one): the surfaces are opaque then.
    try {
        ensureDispatcherQueue();
        auto session = std::make_unique<Session>();

        ComPtr<IDXGIDevice> dxgiDevice;
        check(m_graphics.d3d()->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
        winrt::com_ptr<::IInspectable> device;
        check(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.Get(), device.put()));
        session->device = device.as<IDirect3DDevice>();

        const auto interop = winrt::get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        GraphicsCaptureItem item{nullptr};
        check(interop->CreateForWindow(m_window, winrt::guid_of<GraphicsCaptureItem>(), winrt::put_abi(item)));
        session->size = item.Size();
        session->pool = Direct3D11CaptureFramePool::Create(session->device, pixelFormat, 2, session->size);
        session->frameArrived = session->pool.FrameArrived([this](auto &&, auto &&) { frameArrived(); });
        session->session = session->pool.CreateCaptureSession(item);
        session->session.IsBorderRequired(false);
        session->session.IsCursorCaptureEnabled(false);
        m_session = std::move(session);
        if (canThrottle())
            applyRate();
        m_session->session.StartCapture();
    } catch (const winrt::hresult_error &) {
    } catch (const std::system_error &) {
    }
}

void BackdropCapture::frameArrived()
{
    const auto frame = m_session->pool.TryGetNextFrame();
    if (!frame)
        return;
    // The window was resized: later frames come at the new size.
    const SizeInt32 size = frame.ContentSize();
    if (size.Width != m_session->size.Width || size.Height != m_session->size.Height) {
        m_session->size = size;
        m_session->pool.Recreate(m_session->device, pixelFormat, 2, size);
    }

    ComPtr<ID3D11Texture2D> source;
    const auto access = frame.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    check(access->GetInterface(IID_PPV_ARGS(&source)));

    // Kept in a texture of our own: the pool reuses its frames.
    D3D11_TEXTURE2D_DESC description{};
    if (m_image)
        m_image->GetDesc(&description);
    if (!m_image || description.Width != static_cast<UINT>(size.Width) || description.Height != static_cast<UINT>(size.Height)) {
        source->GetDesc(&description);
        description.Width = size.Width;
        description.Height = size.Height;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        description.CPUAccessFlags = 0;
        description.MiscFlags = 0;
        m_image.Reset();
        check(m_graphics.d3d()->CreateTexture2D(&description, nullptr, &m_image));
    }
    ComPtr<ID3D11DeviceContext> context;
    m_graphics.d3d()->GetImmediateContext(&context);
    const D3D11_BOX content{0, 0, 0, static_cast<UINT>(size.Width), static_cast<UINT>(size.Height), 1};
    context->CopySubresourceRegion(m_image.Get(), 0, 0, 0, 0, source.Get(), 0, &content);

    // The image is the window as it's seen, which DWM's frame bounds give on screen.
    RECT bounds{};
    DwmGetWindowAttribute(m_window, DWMWA_EXTENDED_FRAME_BOUNDS, &bounds, sizeof(bounds));
    m_origin = {bounds.left, bounds.top};
    changed.emit();
}

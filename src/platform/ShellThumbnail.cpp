#include "platform/ShellThumbnail.h"

#include <windows.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

std::optional<Picture> shellThumbnail(const std::wstring &path, int size)
{
    ComPtr<IShellItemImageFactory> factory;
    if (FAILED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&factory))))
        return {};
    HBITMAP bitmap = nullptr;
    if (FAILED(factory->GetImage({size, size}, SIIGBF_THUMBNAILONLY, &bitmap)))
        return {};

    BITMAP description{};
    GetObjectW(bitmap, sizeof(description), &description);
    Picture picture{description.bmWidth, description.bmHeight};
    picture.pixels.resize(static_cast<size_t>(picture.width) * picture.height);
    // Top row first, as 32-bit BGR.
    BITMAPINFO format{};
    format.bmiHeader = {sizeof(BITMAPINFOHEADER), picture.width, -picture.height, 1, 32, BI_RGB};
    HDC screen = GetDC(nullptr);
    const int rows = GetDIBits(screen, bitmap, 0, picture.height, picture.pixels.data(), &format, DIB_RGB_COLORS);
    ReleaseDC(nullptr, screen);
    DeleteObject(bitmap);
    if (rows != picture.height)
        return {};
    return picture;
}

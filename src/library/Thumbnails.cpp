#include "library/Thumbnails.h"

#include <objbase.h>

#include "platform/ShellThumbnail.h"

namespace {

// Pixels on a thumbnail's longer side: enough for the largest tile (316 wide) at 150% scale.
constexpr int thumbnailSize = 512;

} // namespace

Thumbnails::Thumbnails()
{
    m_loader.post([] { CoInitializeEx(nullptr, COINIT_MULTITHREADED); });
}

Thumbnails::~Thumbnails()
{
    m_loader.post([] { CoUninitialize(); });
}

std::shared_ptr<const Picture> Thumbnails::get(const std::wstring &location)
{
    const auto [found, added] = m_thumbnails.try_emplace(location);
    if (added) {
        m_loader.post([this, location] {
            std::optional<Picture> picture = shellThumbnail(location, thumbnailSize);
            if (!picture)
                return;
            m_loader.runOnUiThread([this, location, loaded = std::make_shared<const Picture>(std::move(*picture))] {
                m_thumbnails[location] = loaded;
                changed.emit();
            });
        });
    }
    return found->second;
}

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "core/Picture.h"
#include "core/Signal.h"
#include "platform/WorkerThread.h"

// The library's pictures of media, from Windows' thumbnail cache (shellThumbnail), loaded in the
// background the first time a tile asks for one, then kept.
class Thumbnails
{
public:
    Thumbnails();
    ~Thumbnails();

    // The location's thumbnail: null while it loads, and when there's none.
    std::shared_ptr<const Picture> get(const std::wstring &location);

    // A thumbnail arrived.
    Signal<> changed;

private:
    // Every location asked for, with its thumbnail once loaded.
    std::unordered_map<std::wstring, std::shared_ptr<const Picture>> m_thumbnails;
    // Last, so it ends before what its tasks use goes.
    WorkerThread m_loader;
};

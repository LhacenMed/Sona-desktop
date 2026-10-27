#pragma once

#include <optional>
#include <string>

#include "core/Picture.h"

// The picture Explorer shows for a file, from Windows' thumbnail cache: a video's frame, a song's
// cover art. At most `size` pixels on its longer side. None when Windows has no thumbnail for it (its
// file type's icon doesn't count). It can take a while: call it off the UI thread, with COM set up.
std::optional<Picture> shellThumbnail(const std::wstring &path, int size);

#include "platform/Resources.h"

#include <stdexcept>
#include <string>
#include <windows.h>

#include "core/Text.h"

std::span<const std::byte> asset(std::wstring_view path)
{
    const std::wstring name(path);
    HRSRC resource = FindResourceW(nullptr, name.c_str(), RT_RCDATA);
    if (!resource)
        throw std::runtime_error("Missing asset: " + toUtf8(path));
    const auto *data = static_cast<const std::byte *>(LockResource(LoadResource(nullptr, resource)));
    return {data, SizeofResource(nullptr, resource)};
}

#include "platform/KnownFolders.h"

std::wstring knownFolderPath(const KNOWNFOLDERID &folder)
{
    PWSTR path = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(folder, KF_FLAG_DEFAULT, nullptr, &path)))
        result = path;
    CoTaskMemFree(path);
    return result;
}

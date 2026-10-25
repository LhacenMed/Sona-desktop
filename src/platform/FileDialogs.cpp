#include "platform/FileDialogs.h"

#include <objbase.h>
#include <ShObjIdl.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {

std::optional<std::wstring> showOpenDialog(HWND owner, const std::wstring &startFolder, FILEOPENDIALOGOPTIONS extraOptions)
{
    ComPtr<IFileOpenDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
        return {};

    FILEOPENDIALOGOPTIONS options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_FORCEFILESYSTEM | extraOptions);

    ComPtr<IShellItem> folder;
    if (SUCCEEDED(SHCreateItemFromParsingName(startFolder.c_str(), nullptr, IID_PPV_ARGS(&folder))))
        dialog->SetFolder(folder.Get());

    // Fails when the user cancels.
    ComPtr<IShellItem> result;
    if (FAILED(dialog->Show(owner)) || FAILED(dialog->GetResult(&result)))
        return {};

    PWSTR path = nullptr;
    if (FAILED(result->GetDisplayName(SIGDN_FILESYSPATH, &path)))
        return {};
    std::wstring value(path);
    CoTaskMemFree(path);
    return value;
}

} // namespace

std::optional<std::wstring> chooseFile(HWND owner, const std::wstring &startFolder)
{
    return showOpenDialog(owner, startFolder, 0);
}

std::optional<std::wstring> chooseFolder(HWND owner, const std::wstring &startFolder)
{
    return showOpenDialog(owner, startFolder, FOS_PICKFOLDERS);
}

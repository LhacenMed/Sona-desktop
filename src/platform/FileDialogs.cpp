#include "platform/FileDialogs.h"

#include <objbase.h>
#include <ShObjIdl.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include <filesystem>
#include <iterator>

#include "platform/Window.h"

using Microsoft::WRL::ClassicCom;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;

namespace {

constexpr DWORD selectFolderButton = 1;

std::wstring pathOf(IShellItem *item)
{
    PWSTR path = nullptr;
    if (!item || FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
        return {};
    std::wstring value(path);
    CoTaskMemFree(path);
    return value;
}

// An open dialog in `startFolder`, listing files of `type` (or folders, with FOS_PICKFOLDERS).
ComPtr<IFileOpenDialog> createDialog(const std::wstring &startFolder, FILEOPENDIALOGOPTIONS extraOptions, const FileType *type)
{
    ComPtr<IFileOpenDialog> dialog;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
        return nullptr;

    FILEOPENDIALOGOPTIONS options = 0;
    dialog->GetOptions(&options);
    dialog->SetOptions(options | FOS_FORCEFILESYSTEM | extraOptions);

    if (type) {
        const COMDLG_FILTERSPEC types[] = {{type->name.c_str(), type->pattern.c_str()}, {L"All Files", L"*.*"}};
        dialog->SetFileTypes(static_cast<UINT>(std::size(types)), types);
    }

    ComPtr<IShellItem> folder;
    if (SUCCEEDED(SHCreateItemFromParsingName(startFolder.c_str(), nullptr, IID_PPV_ARGS(&folder))))
        dialog->SetFolder(folder.Get());
    return dialog;
}

// What the dialog was closed with: its chosen items, or none when the user cancelled.
std::vector<std::wstring> chosenPaths(IFileOpenDialog *dialog)
{
    ComPtr<IShellItemArray> results;
    if (FAILED(dialog->GetResults(&results)))
        return {};
    std::vector<std::wstring> paths;
    DWORD count = 0;
    results->GetCount(&count);
    for (DWORD i = 0; i < count; ++i) {
        ComPtr<IShellItem> result;
        if (SUCCEEDED(results->GetItemAt(i, &result)))
            paths.push_back(pathOf(result.Get()));
    }
    std::erase(paths, std::wstring());
    return paths;
}

// The Select Folder button of chooseFolderOrFiles: it closes the dialog with the folder selected in
// it, or else the one it shows, as Windows' own folder picker does.
class SelectFolderButton : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IFileDialogEvents, IFileDialogControlEvents>
{
public:
    const std::wstring &folder() const { return m_folder; }

    IFACEMETHODIMP OnButtonClicked(IFileDialogCustomize *customize, DWORD) override
    {
        ComPtr<IFileDialog> dialog;
        customize->QueryInterface(IID_PPV_ARGS(&dialog));
        ComPtr<IShellItem> item;
        dialog->GetCurrentSelection(&item);
        std::error_code error;
        if (m_folder = pathOf(item.Get()); !std::filesystem::is_directory(m_folder, error)) {
            item.Reset();
            dialog->GetFolder(&item);
            m_folder = pathOf(item.Get());
        }
        if (!m_folder.empty())
            dialog->Close(S_OK);
        return S_OK;
    }

    // Nothing else the dialog reports matters here.
    IFACEMETHODIMP OnItemSelected(IFileDialogCustomize *, DWORD, DWORD) override { return S_OK; }
    IFACEMETHODIMP OnCheckButtonToggled(IFileDialogCustomize *, DWORD, BOOL) override { return S_OK; }
    IFACEMETHODIMP OnControlActivating(IFileDialogCustomize *, DWORD) override { return S_OK; }
    IFACEMETHODIMP OnFileOk(IFileDialog *) override { return S_OK; }
    IFACEMETHODIMP OnFolderChanging(IFileDialog *, IShellItem *) override { return S_OK; }
    IFACEMETHODIMP OnFolderChange(IFileDialog *) override { return S_OK; }
    IFACEMETHODIMP OnSelectionChange(IFileDialog *) override { return S_OK; }
    IFACEMETHODIMP OnShareViolation(IFileDialog *, IShellItem *, FDE_SHAREVIOLATION_RESPONSE *) override { return E_NOTIMPL; }
    IFACEMETHODIMP OnTypeChange(IFileDialog *) override { return S_OK; }
    IFACEMETHODIMP OnOverwrite(IFileDialog *, IShellItem *, FDE_OVERWRITE_RESPONSE *) override { return E_NOTIMPL; }

private:
    std::wstring m_folder;
};

} // namespace

void chooseFiles(Window &owner, const std::wstring &startFolder, const FileType &type,
                 std::function<void(std::vector<std::wstring> files)> chosen)
{
    owner.runLater([&owner, startFolder, type, chosen = std::move(chosen)] {
        const ComPtr<IFileOpenDialog> dialog = createDialog(startFolder, FOS_ALLOWMULTISELECT, &type);
        if (!dialog || FAILED(dialog->Show(owner.hwnd())))
            return;
        if (std::vector<std::wstring> files = chosenPaths(dialog.Get()); !files.empty())
            chosen(std::move(files));
    });
}

void chooseFolderOrFiles(Window &owner, const std::wstring &startFolder, const FileType &type,
                         std::function<void(std::vector<std::wstring> locations)> chosen)
{
    owner.runLater([&owner, startFolder, type, chosen = std::move(chosen)] {
        const ComPtr<IFileOpenDialog> dialog = createDialog(startFolder, FOS_ALLOWMULTISELECT, &type);
        ComPtr<IFileDialogCustomize> customize;
        if (!dialog || FAILED(dialog.As(&customize)))
            return;
        dialog->SetTitle(L"Open Folder");
        customize->AddPushButton(selectFolderButton, L"Select Folder");
        const ComPtr<SelectFolderButton> button = Microsoft::WRL::Make<SelectFolderButton>();
        DWORD cookie = 0;
        dialog->Advise(button.Get(), &cookie);
        const HRESULT shown = dialog->Show(owner.hwnd());
        dialog->Unadvise(cookie);
        if (!button->folder().empty()) {
            chosen({button->folder()});
        } else if (SUCCEEDED(shown)) {
            if (std::vector<std::wstring> files = chosenPaths(dialog.Get()); !files.empty())
                chosen(std::move(files));
        }
    });
}

void chooseFolder(Window &owner, const std::wstring &startFolder, std::function<void(std::wstring folder)> chosen)
{
    owner.runLater([&owner, startFolder, chosen = std::move(chosen)] {
        const ComPtr<IFileOpenDialog> dialog = createDialog(startFolder, FOS_PICKFOLDERS, nullptr);
        if (!dialog || FAILED(dialog->Show(owner.hwnd())))
            return;
        if (std::vector<std::wstring> folders = chosenPaths(dialog.Get()); !folders.empty())
            chosen(std::move(folders.front()));
    });
}

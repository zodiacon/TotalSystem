#include "pch.h"
#include "DialogHelper.h"
#include <shobjidl_core.h>
#include <atlcomcli.h>

using namespace std;

wstring DialogHelper::GetOpenFileName(HWND hOwner, wstring_view title, initializer_list<FileDialogFilterItem> filter) {
    CComPtr<IFileOpenDialog> spDlg;
    spDlg.CoCreateInstance(CLSID_FileOpenDialog);
    assert(spDlg);
    if (!spDlg)
        return L"";

    spDlg->SetTitle(title.data());
    std::vector<COMDLG_FILTERSPEC> filters;
    for (auto& item : filter) {
        COMDLG_FILTERSPEC spec;
        spec.pszName = item.Text.data();
        spec.pszSpec = item.Pattern.data();
        filters.push_back(spec);
    }

    spDlg->SetFileTypes((UINT)filters.size(), filters.data());

    wstring result;
    if (S_OK == spDlg->Show(hOwner)) {
        CComPtr<IShellItem> spItem;
        spDlg->GetResult(&spItem);
        if (spItem) {
            PWSTR path = nullptr;
            spItem->GetDisplayName(SIGDN_FILESYSPATH, &path);
            if (path) {
                result = path;
                ::CoTaskMemFree(path);
            }
        }
    }
    return result;
}

FileDialogFilterItem::FileDialogFilterItem(std::wstring_view text, std::wstring_view pattern) : Text(text), Pattern(pattern) {
}

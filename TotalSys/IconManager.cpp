#include "pch.h"
#include "IconManager.h"

bool IconManager::Add(UINT id, int size, HINSTANCE hRes) {
    auto hIcon = (HICON)::LoadImage(hRes ? hRes : ::GetModuleHandle(nullptr), MAKEINTRESOURCE(id), 
        IMAGE_ICON, size, size, LR_COPYFROMRESOURCE | LR_CREATEDIBSECTION);
    if (!hIcon)
        return false;

    return AddIcon(id, hIcon);
}

bool IconManager::AddIcon(UINT id, HICON hIcon) {
    auto image = D3D11Image::FromIcon(hIcon);
    return m_Icons.insert({ id, std::move(image) }).second;
}

void* IconManager::GetImage(UINT id) const {
    if (auto it = m_Icons.find(id); it != m_Icons.end())
        return it->second.Get();
    assert(false && "No image with given ID");
    return nullptr;
}

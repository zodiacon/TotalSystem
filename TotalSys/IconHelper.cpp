#include "pch.h"
#include "IconHelper.h"

HICON IconHelper::LoadIconFromResource(UINT id, int size, HINSTANCE hInst) {
    return LoadIconFromResource(MAKEINTRESOURCE(id), size, hInst);
}

HICON IconHelper::LoadIconFromResource(PCWSTR id, int size, HINSTANCE hInst) {
    return (HICON)::LoadImage(hInst ? hInst : ::GetModuleHandle(nullptr), id, IMAGE_ICON, size, size, 0);
}

#include "pch.h"
#include "IconHelper.h"
#include <shellapi.h>

HICON IconHelper::GetIconFromImagePath(QString const& path) {
    if (path.isEmpty())
        return nullptr;

    std::string spath(path.toStdString());
    WORD index = 0;
    return ::ExtractAssociatedIconA(nullptr, spath.data(), &index);

}

#include "pch.h"
#include "PEHelper.h"

using namespace std;

wstring PEHelper::GetVersionObject(wstring const& path, wstring const& name) {
	if (path.empty())
		return L"";
	BYTE buffer[1 << 11];
	wstring result;
	if (::GetFileVersionInfo(path.c_str(), 0, sizeof(buffer), buffer)) {
		WORD* langAndCodePage;
		UINT len;
		if (::VerQueryValue(buffer, L"\\VarFileInfo\\Translation", (void**)&langAndCodePage, &len)) {
			auto text = format(L"\\StringFileInfo\\{:04x}{:04x}\\{}", langAndCodePage[0], langAndCodePage[1], name);
			WCHAR* desc;
			if (::VerQueryValue(buffer, text.c_str(), (void**)&desc, &len))
				result = desc;
		}
	}
	return result;
}

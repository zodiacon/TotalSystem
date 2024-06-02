#pragma once

struct FileDialogFilterItem {
	std::wstring_view Pattern;
	std::wstring_view Text;

	FileDialogFilterItem(std::wstring_view text, std::wstring_view pattern);
};

struct DialogHelper abstract final {
	static std::wstring GetOpenFileName(HWND hOwner, std::wstring_view title, std::initializer_list<FileDialogFilterItem> filter);
};


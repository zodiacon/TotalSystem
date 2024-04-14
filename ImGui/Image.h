#pragma once

#include <Windows.h>
#include <string_view>

class Image {
public:
	static Image FromIcon(HICON hIcon);
	static Image LoadFromFile(const std::wstring_view path);
};


#pragma once

struct IconHelper abstract final {
	static HICON LoadIconFromResource(UINT id, int size, HINSTANCE hInst = nullptr);
	static HICON LoadIconFromResource(PCWSTR id, int size, HINSTANCE hInst = nullptr);

};


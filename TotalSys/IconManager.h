#pragma once

#include <d3d11Image.h>

class IconManager final {
public:
	IconManager() = default;
	IconManager(IconManager const&) = delete;
	IconManager& operator =(IconManager const&) = delete;

	bool Add(UINT id, int size = 16, HINSTANCE hRes = nullptr);
	bool AddIcon(UINT id, HICON hIcon);

	void* GetImage(UINT id) const;

private:
	std::unordered_map<UINT, D3D11Image> m_Icons;
};


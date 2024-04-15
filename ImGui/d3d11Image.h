#pragma once

#include <Windows.h>
#include <string_view>
#include <wil\com.h>
#include <d3d11.h>

class D3D11Image {
public:
	D3D11Image() = default;
	D3D11Image(D3D11Image const&) = delete;
	D3D11Image(D3D11Image &&) = default;
	D3D11Image& operator=(D3D11Image const&) = delete;
	D3D11Image& operator=(D3D11Image&&) = default;

	static D3D11Image FromIcon(HICON hIcon);
	static D3D11Image FromFile(const std::wstring_view path);

	ID3D11ShaderResourceView* Get() const;
	operator bool() const;

private:
	D3D11Image(ID3D11ShaderResourceView* pRes);

	wil::com_ptr<ID3D11ShaderResourceView> m_spImage;
};


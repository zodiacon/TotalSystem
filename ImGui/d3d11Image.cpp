#include "d3d11Image.h"
#include <wincodec.h>
#include <assert.h>

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;

D3D11Image D3D11Image::FromIcon(HICON hIcon) {
	assert(hIcon);
	wil::com_ptr<IWICImagingFactory> spFactory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory);
	wil::com_ptr<IWICBitmap> spBitmap;
	spFactory->CreateBitmapFromHICON(hIcon, &spBitmap);
	UINT width, height;
	spBitmap->GetSize(&width, &height);

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; //DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	wil::com_ptr<ID3D11Texture2D> spTexture;
	D3D11_SUBRESOURCE_DATA subResource;
	wil::com_ptr<IWICBitmapLock> spLock;
	spBitmap->Lock(nullptr, WICBitmapLockRead, &spLock);
	UINT size;
	WICInProcPointer ptr;
	spLock->GetDataPointer(&size, &ptr);
	subResource.pSysMem = ptr;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	g_pd3dDevice->CreateTexture2D(&desc, &subResource, spTexture.addressof());

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	//DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	wil::com_ptr<ID3D11ShaderResourceView> spIcon;
	g_pd3dDevice->CreateShaderResourceView(spTexture.get(), &srvDesc, spIcon.addressof());
	return D3D11Image(spIcon.get());
}

ID3D11ShaderResourceView* D3D11Image::Get() const {
	return m_spImage.get();
}

D3D11Image::operator ImTextureID() const {
	return Get();
}

D3D11Image::operator bool() const {
	return m_spImage.get() != nullptr;
}

D3D11Image::D3D11Image(ID3D11ShaderResourceView* pRes) : m_spImage(pRes) {
}


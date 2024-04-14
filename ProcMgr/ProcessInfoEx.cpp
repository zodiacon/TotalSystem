#include "pch.h"
#include "ProcessInfoEx.h"
#include <WinLowLevel.h>
#include <shellapi.h>
//#include "colors.h"
#include "ProcessManager.h"
#include "Globals.h"
//#include "Settings.h"
//#include "ProcessColor.h"
#include <wincodec.h>
#include "resource.h"

using namespace std;
using namespace WinLL;

//std::pair<const ImVec4&, const ImVec4&> ProcessInfoEx::GetColors(WinSys::ProcessManager& pm) const {
//	using namespace ImGui;
//	auto& colors = Globals::Get().GetSettings().ProcessColors;
//
//	if (colors[Settings::DeletedObjects].Enabled && IsTerminated())
//		return { colors[Settings::DeletedObjects].Color, colors[Settings::DeletedObjects].TextColor };
//
//	if (colors[Settings::NewObjects].Enabled && IsNew())
//		return { colors[Settings::NewObjects].Color, colors[Settings::NewObjects].TextColor };
//
//	auto attributes = GetAttributes(pm);
//	if (colors[Settings::Manageed].Enabled && (attributes & ProcessAttributes::Managed) == ProcessAttributes::Managed) {
//		return { colors[Settings::Manageed].Color, colors[Settings::Manageed].TextColor };
//	}
//	if (colors[Settings::Immersive].Enabled && (attributes & ProcessAttributes::Immersive) == ProcessAttributes::Immersive) {
//		return { colors[Settings::Immersive].Color, colors[Settings::Immersive].TextColor };
//	}
//	if (colors[Settings::Secure].Enabled && (attributes & ProcessAttributes::Secure) == ProcessAttributes::Secure) {
//		return { colors[Settings::Secure].Color, colors[Settings::Secure].TextColor };
//	}
//	if (colors[Settings::Protected].Enabled && (attributes & ProcessAttributes::Protected) == ProcessAttributes::Protected) {
//		return { colors[Settings::Protected].Color, colors[Settings::Protected].TextColor };
//	}
//	if (colors[Settings::Services].Enabled && (attributes & ProcessAttributes::Service) == ProcessAttributes::Service) {
//		return { colors[Settings::Services].Color, colors[Settings::Services].TextColor };
//	}
//	if (colors[Settings::InJob].Enabled && (attributes & ProcessAttributes::InJob) == ProcessAttributes::InJob) {
//		return { colors[Settings::InJob].Color, colors[Settings::InJob].TextColor };
//	}
//
//	return { ImVec4(-1, 0, 0, 0), ImVec4() };
//}

ProcessAttributes ProcessInfoEx::Attributes() const {
	if (m_Attributes == ProcessAttributes::NotComputed) {
		m_Attributes = ProcessAttributes::None;
		auto parent = Globals::ProcessManager().GetProcessById(ParentId);
		if (parent && _wcsicmp(parent->GetImageName().c_str(), L"services.exe") == 0)
			m_Attributes |= ProcessAttributes::Service;

		Process process;
		if(process.Open(Id, ProcessAccessMask::QueryLimitedInformation)) {
			if (process.IsManaged())
				m_Attributes |= ProcessAttributes::Managed;
			if (process.IsProtected())
				m_Attributes |= ProcessAttributes::Protected;
			if (process.IsImmersive())
				m_Attributes |= ProcessAttributes::Immersive;
			if (process.IsSecure())
				m_Attributes |= ProcessAttributes::Secure;
			if (process.IsInJob())
				m_Attributes |= ProcessAttributes::InJob;
		}
	}
	return m_Attributes;
}

const std::wstring& ProcessInfoEx::UserName() const {
	if (m_UserName.empty()) {
		if (Id <= 4)
			m_UserName = L"NT AUTHORITY\\SYSTEM";
		else {
			Process process;
			if (process.Open(Id)) {
				m_UserName = process.GetUserName();
			}
			else if(::GetLastError() == ERROR_ACCESS_DENIED) {
				m_UserName = L"<access denied>";
			}
			else {
				m_UserName = L"<unknown>";
			}
		}
	}
	return m_UserName;
}

bool ProcessInfoEx::Update() {
	if (!m_IsNew && !m_IsTerminated)
		return false;

	bool term = m_IsTerminated;
	if (::GetTickCount64() > m_ExpiryTime) {
		m_IsTerminated = m_IsNew = false;
		return term;
	}
	return false;
}

void ProcessInfoEx::New(uint32_t ms) {
	m_IsNew = true;
	m_ExpiryTime = ::GetTickCount64() + ms;
}

void ProcessInfoEx::Term(uint32_t ms) {
	m_IsNew = false;
	m_IsTerminated = true;
	m_ExpiryTime = ::GetTickCount64() + ms;
}

const std::wstring& ProcessInfoEx::GetExecutablePath() const {
	if (m_ExecutablePath.empty() && Id != 0) {
		Process process;
		if (process.Open(Id, ProcessAccessMask::QueryLimitedInformation)) {
			m_ExecutablePath = process.GetFullImageName();
		}
	}
	return m_ExecutablePath;
}

ID3D11ShaderResourceView* ProcessInfoEx::Icon() const {
	if (m_spIcon == nullptr) {
		static HICON hAppIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
		extern ID3D11Device* g_pd3dDevice;
		extern ID3D11DeviceContext* g_pd3dDeviceContext;

		UINT width, height;
		wil::com_ptr<IWICImagingFactory> spFactory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory);
		wil::com_ptr<IWICBitmap> spBitmap;
		auto& path = GetExecutablePath();
		auto hIcon = ::ExtractIcon(nullptr, path.c_str(), 0);
		if (!hIcon)
			hIcon = hAppIcon;
		assert(hIcon);
		spFactory->CreateBitmapFromHICON(hIcon, &spBitmap);
		if (hIcon != hAppIcon)
			::DestroyIcon(hIcon);
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
		g_pd3dDevice->CreateShaderResourceView(spTexture.get(), &srvDesc, m_spIcon.addressof());
	}
	return m_spIcon.get();
}

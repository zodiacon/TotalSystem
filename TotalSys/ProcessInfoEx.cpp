#include "pch.h"
#include "ProcessInfoEx.h"
#include <shellapi.h>
#include "Globals.h"
#include "ProcessColor.h"
#include "TotalSysSettings.h"
#include <ShellScalingApi.h>

#pragma comment(lib, "Version.lib")

using namespace std;
using namespace WinLL;

std::pair<const ImVec4, const ImVec4> ProcessInfoEx::Colors(DefaultProcessManager& pm) const {
	using namespace ImGui;
	auto& colors = Globals::Settings().GetProcessColors();

	if (colors[TotalSysSettings::DeletedObjects].Enabled && IsTerminated())
		return { colors[TotalSysSettings::DeletedObjects].Color, colors[TotalSysSettings::DeletedObjects].TextColor };

	if (colors[TotalSysSettings::NewObjects].Enabled && IsNew())
		return { colors[TotalSysSettings::NewObjects].Color, colors[TotalSysSettings::NewObjects].TextColor };

	if (colors[TotalSysSettings::Suspended].Enabled && IsSuspended()) {
		return { colors[TotalSysSettings::Suspended].Color, colors[TotalSysSettings::Suspended].TextColor };
	}

	auto attributes = Attributes(pm);
	if (colors[TotalSysSettings::Manageed].Enabled && (attributes & ProcessAttributes::Managed) == ProcessAttributes::Managed) {
		return { colors[TotalSysSettings::Manageed].Color, colors[TotalSysSettings::Manageed].TextColor };
	}
	if (colors[TotalSysSettings::Immersive].Enabled && (attributes & ProcessAttributes::Immersive) == ProcessAttributes::Immersive) {
		return { colors[TotalSysSettings::Immersive].Color, colors[TotalSysSettings::Immersive].TextColor };
	}
	if (colors[TotalSysSettings::Secure].Enabled && (attributes & ProcessAttributes::Secure) == ProcessAttributes::Secure) {
		return { colors[TotalSysSettings::Secure].Color, colors[TotalSysSettings::Secure].TextColor };
	}
	if (colors[TotalSysSettings::Protected].Enabled && (attributes & ProcessAttributes::Protected) == ProcessAttributes::Protected) {
		return { colors[TotalSysSettings::Protected].Color, colors[TotalSysSettings::Protected].TextColor };
	}
	if (colors[TotalSysSettings::Services].Enabled && (attributes & ProcessAttributes::Service) == ProcessAttributes::Service) {
		return { colors[TotalSysSettings::Services].Color, colors[TotalSysSettings::Services].TextColor };
	}
	if (colors[TotalSysSettings::InJob].Enabled && (attributes & ProcessAttributes::InJob) == ProcessAttributes::InJob) {
		return { colors[TotalSysSettings::InJob].Color, colors[TotalSysSettings::InJob].TextColor };
	}

	return { ImVec4(-1, 0, 0, 0), ImVec4() };
}

ProcessAttributes ProcessInfoEx::Attributes(DefaultProcessManager& pm) const {
	if (m_Attributes == ProcessAttributes::NotComputed) {
		m_Attributes = ProcessAttributes::None;
		auto parent = pm.GetProcessById(ParentId);
		if (parent && _wcsicmp(parent->GetImageName().c_str(), L"services.exe") == 0)
			m_Attributes |= ProcessAttributes::Service;

		Process process;
		if (process.Open(Id, ProcessAccessMask::QueryLimitedInformation)) {
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

bool ProcessInfoEx::IsSuspended() const {
	return m_Suspended || AreAllThreadsSuspended();
}

bool ProcessInfoEx::SuspendResume() {
	Process p;
	if (!p.Open(Id, ProcessAccessMask::SuspendResume))
		return false;

	m_Suspended = !m_Suspended;
	return m_Suspended ? p.Suspend() : p.Resume();
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
	if (!m_Icon) {
		static HICON hAppIcon = ::LoadIcon(nullptr, IDI_APPLICATION);

		auto& path = GetExecutablePath();
		auto hIcon = ::ExtractIcon(nullptr, path.c_str(), 0);
		if (!hIcon)
			hIcon = hAppIcon;
		m_Icon = move(D3D11Image::FromIcon(hIcon));
		if (hIcon != hAppIcon)
			::DestroyIcon(hIcon);
	}
	return m_Icon.Get();
}

IntegrityLevel ProcessInfoEx::GetIntegrityLevel() const {
	if (Id <= 4)
		return IntegrityLevel::System;

	Process p;
	if (p.Open(Id, ProcessAccessMask::QueryLimitedInformation)) {
		return p.GetIntegrityLevel();
	}
	return IntegrityLevel::Error;
}

PVOID ProcessInfoEx::GetPeb() const {
	if (Id <= 4)
		return nullptr;

	if (!m_Peb) {
		Process p;
		if (p.Open(Id, ProcessAccessMask::QueryLimitedInformation))
			m_Peb = p.GetPeb();
	}
	return m_Peb;
}

WinLL::ProcessProtection ProcessInfoEx::GetProtection() const {
	Process p;
	if (p.Open(Id, ProcessAccessMask::QueryLimitedInformation))
		return p.GetProtection();
	return ProcessProtection();
}

const wstring& ProcessInfoEx::GetDescription() const {
	if (!m_DescChecked) {
		m_Description = GetVersionObject(L"FileDescription");
		m_DescChecked = true;
	}
	return m_Description;
}

const wstring& ProcessInfoEx::GetCompanyName() const {
	if (!m_CompanyChecked) {
		m_Company = GetVersionObject(L"CompanyName");
		m_CompanyChecked = true;
	}
	return m_Company;
}

DpiAwareness ProcessInfoEx::GetDpiAwareness() const {
	static const auto pGetProcessDpiAware = (decltype(::GetProcessDpiAwareness)*)::GetProcAddress(::GetModuleHandle(L"shcore"), "GetProcessDpiAwareness");
	DpiAwareness da = DpiAwareness::None;
	if (pGetProcessDpiAware == nullptr)
		return da;

	Process p;
	if (p.Open(Id, ProcessAccessMask::QueryLimitedInformation)) {
		pGetProcessDpiAware(p.Handle(), reinterpret_cast<PROCESS_DPI_AWARENESS*>(&da));
	}
	return da;
}

bool ProcessInfoEx::AreAllThreadsSuspended() const {
	if (GetThreads().empty())
		return false;

	size_t suspended = 0;
	for (auto& t : GetThreads()) {
		if (t->State != ThreadState::Waiting)
			return false;

		if (t->WaitReason == WaitReason::Suspended || t->WaitReason == WaitReason::WrSuspended)
			++suspended;
	}
	//
	// heuristic that should be good enough for immersive processes
	//
	return suspended >= GetThreads().size() / 2;
}

wstring ProcessInfoEx::GetVersionObject(const wstring& name) const {
	BYTE buffer[1 << 12];
	wstring result;
	const auto& exe = GetExecutablePath();
	if (::GetFileVersionInfo(exe.c_str(), 0, sizeof(buffer), buffer)) {
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

int ProcessInfoEx::GetBitness() const {
	if (m_Bitness == 0) {
		static SYSTEM_INFO si = { 0 };
		if (si.dwNumberOfProcessors == 0)
			::GetNativeSystemInfo(&si);
		Process p;
		if (p.Open(Id, ProcessAccessMask::QueryLimitedInformation)) {
			if (p.IsWow64Process())
				m_Bitness = 32;
			else
				m_Bitness = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM ? 32 : 64;
		}
		else {
			m_Bitness = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM ? 32 : 64;
		}
	}
	return m_Bitness;
}

int ProcessInfoEx::GetMemoryPriority() const {
	Process p;
	if (p.Open(Id, ProcessAccessMask::QueryLimitedInformation))
		return p.GetMemoryPriority();
	return -1;
}

WinLL::IoPriority ProcessInfoEx::GetIoPriority() const {
	Process p;
	if (p.Open(Id, ProcessAccessMask::QueryLimitedInformation))
		return p.GetIoPriority();
	return IoPriority::Unknown;
}

VirtualizationState ProcessInfoEx::GetVirtualizationState() const {
	Token token;
	if (!token.Open(TokenAccessMask::Query, Id)) {
		return VirtualizationState::Unknown;
	}

	return token.GetVirtualizationState();
}

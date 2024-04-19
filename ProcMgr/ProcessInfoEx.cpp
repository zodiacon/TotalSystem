#include "pch.h"
#include "ProcessInfoEx.h"
#include <WinLowLevel.h>
#include <shellapi.h>
#include "Globals.h"
#include "ProcessColor.h"
#include "ProcMgrSettings.h"

using namespace std;
using namespace WinLL;

std::pair<const ImVec4, const ImVec4> ProcessInfoEx::Colors() const {
	using namespace ImGui;
	auto& colors = Globals::Settings().ProcessColors;

	if (colors[ProcMgrSettings::DeletedObjects].Enabled && IsTerminated())
		return { colors[ProcMgrSettings::DeletedObjects].Color, colors[ProcMgrSettings::DeletedObjects].TextColor };

	if (colors[ProcMgrSettings::NewObjects].Enabled && IsNew())
		return { colors[ProcMgrSettings::NewObjects].Color, colors[ProcMgrSettings::NewObjects].TextColor };

	auto attributes = Attributes();
	if (colors[ProcMgrSettings::Manageed].Enabled && (attributes & ProcessAttributes::Managed) == ProcessAttributes::Managed) {
		return { colors[ProcMgrSettings::Manageed].Color, colors[ProcMgrSettings::Manageed].TextColor };
	}
	if (colors[ProcMgrSettings::Immersive].Enabled && (attributes & ProcessAttributes::Immersive) == ProcessAttributes::Immersive) {
		return { colors[ProcMgrSettings::Immersive].Color, colors[ProcMgrSettings::Immersive].TextColor };
	}
	if (colors[ProcMgrSettings::Secure].Enabled && (attributes & ProcessAttributes::Secure) == ProcessAttributes::Secure) {
		return { colors[ProcMgrSettings::Secure].Color, colors[ProcMgrSettings::Secure].TextColor };
	}
	if (colors[ProcMgrSettings::Protected].Enabled && (attributes & ProcessAttributes::Protected) == ProcessAttributes::Protected) {
		return { colors[ProcMgrSettings::Protected].Color, colors[ProcMgrSettings::Protected].TextColor };
	}
	if (colors[ProcMgrSettings::Services].Enabled && (attributes & ProcessAttributes::Service) == ProcessAttributes::Service) {
		return { colors[ProcMgrSettings::Services].Color, colors[ProcMgrSettings::Services].TextColor };
	}
	if (colors[ProcMgrSettings::InJob].Enabled && (attributes & ProcessAttributes::InJob) == ProcessAttributes::InJob) {
		return { colors[ProcMgrSettings::InJob].Color, colors[ProcMgrSettings::InJob].TextColor };
	}
	if (colors[ProcMgrSettings::Suspended].Enabled && IsSuspended()) {
		return { colors[ProcMgrSettings::Suspended].Color, colors[ProcMgrSettings::Suspended].TextColor };
	}

	return { ImVec4(-1, 0, 0, 0), ImVec4() };
}

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


bool ProcessInfoEx::IsSuspended() const {
	return m_Suspended;
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

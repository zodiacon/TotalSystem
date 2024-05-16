#include "pch.h"
#include "ProcessInfoEx.h"
#include <shellapi.h>
#include "Globals.h"
#include "ProcessColor.h"
#include "TotalSysSettings.h"
#include <ShellScalingApi.h>
#include "UI.h"

#pragma comment(lib, "Version.lib")

using namespace std;
using namespace WinLL;

ProcessInfoEx::ProcessInfoEx(uint32_t pid) : ProcessInfo(pid) {
	m_Process.Open(pid, ProcessAccessMask::QueryLimitedInformation);
	if (m_Process)
		m_Symbols.Load(m_Process.Handle());
}

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

ProcessAttributes ProcessInfoEx::Attributes(DefaultProcessManager const& pm) const {
	if (m_Attributes == ProcessAttributes::NotComputed) {
		m_Attributes = ProcessAttributes::None;
		auto parent = pm.GetProcessById(ParentId);
		if (parent && _wcsicmp(parent->GetImageName().c_str(), L"services.exe") == 0)
			m_Attributes |= ProcessAttributes::Service;

		if (m_Process) {
			if (m_Process.IsManaged())
				m_Attributes |= ProcessAttributes::Managed;
			if (m_Process.IsProtected())
				m_Attributes |= ProcessAttributes::Protected;
			if (m_Process.IsImmersive())
				m_Attributes |= ProcessAttributes::Immersive;
			if (m_Process.IsSecure())
				m_Attributes |= ProcessAttributes::Secure;
			if (m_Process.IsInJob())
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
	if (m_ExecutablePath.empty() && Id > 4 && m_Process) {
		m_ExecutablePath = m_Process.GetFullImageName();
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

	if (m_Process) {
		return m_Process.GetIntegrityLevel();
	}
	return IntegrityLevel::Error;
}

PVOID ProcessInfoEx::GetPeb() const {
	if (Id <= 4)
		return nullptr;

	if (!m_Peb && m_Process) {
		m_Peb = m_Process.GetPeb();
	}
	return m_Peb;
}

WinLL::ProcessProtection ProcessInfoEx::GetProtection() const {
	if (m_Process)
		return m_Process.GetProtection();
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

		if(static_pointer_cast<ThreadInfoEx>(t)->IsSuspended())
			++suspended;
	}
	//
	// heuristic that should be good enough for immersive processes
	//
	return suspended == GetThreads().size();
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
		static SYSTEM_INFO si {};
		if (si.dwNumberOfProcessors == 0)
			::GetNativeSystemInfo(&si);
		if (m_Process) {
			if (m_Process.IsWow64Process())
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
	if (m_Process)
		return m_Process.GetMemoryPriority();
	return -1;
}

WinLL::IoPriority ProcessInfoEx::GetIoPriority() const {
	if (m_Process)
		return m_Process.GetIoPriority();
	return IoPriority::Unknown;
}

VirtualizationState ProcessInfoEx::GetVirtualizationState() const {
	Token token;
	if (!token.Open(TokenAccessMask::Query, Id)) {
		return VirtualizationState::Unknown;
	}

	return token.GetVirtualizationState();
}

ProcessSymbols const& ProcessInfoEx::GetSymbols() const {
	return m_Symbols;
}

std::string ProcessInfoEx::GetAddressSymbol(uint64_t address) const {
	if (!m_Addresses.contains(address)) {
		m_Addresses.insert({ address, format("0x{:X}", address) });

		UI::SubmitWorkWithResult([=]() -> string* {
			auto& symbols = GetSymbols();
			uint64_t disp, modBase;
			const SYMBOL_INFO* sym;
			{
				lock_guard locker(s_Lock);
				sym = symbols.GetSymbolFromAddress(address, &disp, &modBase);
			}
			if (sym) {
				auto result = new string();
				if (disp == 0)
					*result = sym->Name;
				else
					*result = std::format("{}+0x{:X}", sym->Name, disp);
				*result = GetModuleName(modBase) + "!" + *result;
				return result;
			}
			return nullptr;
			}, [=](auto result) {
				if (result) {
					auto name = (string*)result;
					m_Addresses[address] = move(*name);
					delete name;
				}
				else {
					m_Addresses.erase(address);
				}
				});
	}
	return m_Addresses.at(address);
}

std::string ProcessInfoEx::GetModuleName(uint64_t baseAddress) const {
	auto& symbols = GetSymbols();
	auto name = symbols.GetModuleName(baseAddress);
	char text[256];
	sprintf_s(text, "%ws", name.c_str());
	return text;
}

std::wstring const& ProcessInfoEx::GetCommandLine() const {
	if (m_CommandLine.empty()) {
		m_CommandLine = m_Process.GetCommandLine();
		if (m_CommandLine.empty())
			m_CommandLine = L"<N/A>";
	}
	return m_CommandLine;
}

PriorityClass ProcessInfoEx::GetPriorityClass() const {
	return m_Process.GetPriorityClass();
}

std::wstring ProcessInfoEx::GetCurrentDirectory() const {
	if (!m_Process)
		return L"";

	auto dir = m_Process.GetCurrentDirectory();
	if (::GetLastError() == ERROR_ACCESS_DENIED) {
		Process p;
		if (!p.Open(Id, ProcessAccessMask::VmRead | ProcessAccessMask::QueryLimitedInformation))
			return L"";

		dir = p.GetCurrentDirectory();
		m_Process.Attach(p.Detach());
	}

	return dir;
}

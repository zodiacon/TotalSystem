#pragma once

#include <ProcessInfo.h>
#include <D3D11Image.h>
#include "TransientObject.h"
#include <WinLowLevel.h>
#include "ProcessSymbols.h"
#include <mutex>

enum class ProcessAttributes {
	NotComputed = -1,
	None = 0,
	Protected = 1,
	InJob = 2,
	Service = 4,
	Managed = 8,
	Secure = 0x10,
	Immersive = 0x20,
};
DEFINE_ENUM_FLAG_OPERATORS(ProcessAttributes);

#include "ViewBase.h"

class ProcessInfoEx : public WinLL::ProcessInfo, public TransientObject {
public:
	explicit ProcessInfoEx(uint32_t pid);
	std::pair<const ImVec4, const ImVec4> Colors(DefaultProcessManager& pm) const;
	ProcessAttributes Attributes(DefaultProcessManager const& pm) const;
	bool SuspendResume();

	WinLL::Process& GetProcess();
	WinLL::Process const& GetProcess() const;
	[[nodiscard]] bool IsSuspended() const;
	[[nodiscard]] const std::wstring& GetExecutablePath() const;
	[[nodiscard]] ID3D11ShaderResourceView* Icon() const;
	[[nodiscard]] WinLL::IntegrityLevel GetIntegrityLevel() const;
	[[nodiscard]] PVOID GetPeb() const;
	[[nodiscard]] WinLL::ProcessProtection GetProtection() const;
	[[nodiscard]] const std::wstring& GetDescription() const;
	[[nodiscard]] const std::wstring& GetCompanyName() const;
	[[nodiscard]] WinLL::DpiAwareness GetDpiAwareness() const;
	[[nodiscard]] int GetBitness() const;
	[[nodiscard]] int GetMemoryPriority() const;
	[[nodiscard]] WinLL::IoPriority GetIoPriority() const;
	[[nodiscard]] WinLL::VirtualizationState GetVirtualizationState() const;
	[[nodiscard]] ProcessSymbols const& GetSymbols() const;
	[[nodiscard]] std::string GetAddressSymbol(uint64_t address) const;
	[[nodiscard]] std::string GetModuleName(uint64_t baseAddress) const;
	[[nodiscard]] std::wstring const& GetCommandLine() const;
	[[nodiscard]] WinLL::PriorityClass GetPriorityClass() const;
	[[nodiscard]] std::wstring GetCurrentDirectory() const;
	[[nodiscard]] std::string GetSidAsString() const;

private:
	[[nodiscard]] bool AreAllThreadsSuspended() const;
	std::wstring GetVersionObject(const std::wstring& name) const;

private:
	mutable D3D11Image m_Icon;
	mutable wil::com_ptr<ID3D11ShaderResourceView> m_spIcon;
	mutable std::wstring m_ExecutablePath;
	mutable ProcessAttributes m_Attributes = ProcessAttributes::NotComputed;
	mutable std::wstring m_UserName, m_Description, m_Company;
	mutable PVOID m_Peb{ nullptr };
	mutable WinLL::Process m_Process;
	mutable int m_Bitness{ 0 };
	mutable	bool m_Suspended : 1 { false };
	mutable bool m_CompanyChecked{ false }, m_DescChecked{ false };
	ProcessSymbols m_Symbols;
	mutable std::unordered_map<uint64_t, std::string> m_Addresses;
	mutable std::mutex s_Lock;
	mutable std::wstring m_CommandLine;
};


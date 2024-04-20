#pragma once

#include <ProcessInfo.h>
#include <D3D11Image.h>
#include "TransientObject.h"
#include <WinLowLevel.h>

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

class ProcessInfoEx : public WinLL::ProcessInfo, public TransientObject {
public:
	std::pair<const ImVec4, const ImVec4> Colors() const;
	ProcessAttributes Attributes() const;
	const std::wstring& UserName() const;

	bool IsSuspended() const;
	bool SuspendResume();
	const std::wstring& GetExecutablePath() const;
	ID3D11ShaderResourceView* Icon() const;
	WinLL::IntegrityLevel GetIntegrityLevel() const;
	PVOID GetPeb() const;

	bool Filtered{ false };

private:
	mutable D3D11Image m_Icon;
	mutable wil::com_ptr<ID3D11ShaderResourceView> m_spIcon;
	mutable std::wstring m_ExecutablePath;
	mutable ProcessAttributes m_Attributes = ProcessAttributes::NotComputed;
	mutable std::wstring m_UserName;
	mutable PVOID m_Peb{ nullptr };
	bool m_Suspended : 1 { false};
};


#pragma once

#include <ProcessInfo.h>
#include <D3D11Image.h>

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

class ProcessInfoEx : public WinLL::ProcessInfo{
public:
	bool IsNew() const {
		return m_IsNew;
	}

	bool IsTerminated() const {
		return m_IsTerminated;
	}

	std::pair<const ImVec4&, const ImVec4&> Colors() const;
	ProcessAttributes Attributes() const;
	const std::wstring& UserName() const;

	bool Update();
	void New(uint32_t ms);
	void Term(uint32_t ms);
	const std::wstring& GetExecutablePath() const;
	ID3D11ShaderResourceView* Icon() const;

	bool Filtered{ false };

private:
	mutable D3D11Image m_Icon;
	mutable wil::com_ptr<ID3D11ShaderResourceView> m_spIcon;
	DWORD64 m_ExpiryTime;
	mutable std::wstring m_ExecutablePath;
	mutable ProcessAttributes m_Attributes = ProcessAttributes::NotComputed;
	mutable std::wstring m_UserName;
	bool m_IsNew : 1 = false, m_IsTerminated : 1 = false;
};


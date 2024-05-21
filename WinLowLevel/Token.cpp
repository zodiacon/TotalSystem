#include "pch.h"
#include "WinLowLevel.h"

namespace WinLL {
	using namespace std;

	Sid::Sid(PSID sid, bool copy) : m_Owner(copy) {
		if (copy)
			::CopySid(sizeof(m_Sid), &m_Sid, sid);
		else
			m_pSid = sid;
	}

	Sid::operator bool() const {
		if (m_pSid == nullptr && !m_Owner)
			return false;

		return ::IsValidSid(Get());
	}

	const PSID Sid::Get() const {
		return m_Owner ? (PSID)&m_Sid.Sid : m_pSid;
	}

	string Sid::AsString() const {
		PSTR ssid;
		string result;
		if (::ConvertSidToStringSidA(Get(), &ssid)) {
			result = ssid;
			::LocalFree(ssid);
		}
		return result;
	}

	bool Token::Open(TokenAccessMask access, uint32_t pid) {
		wil::unique_handle hProcess;
		if (pid) {
			CLIENT_ID cid = { UlongToHandle(pid) };
			auto status = ::NtOpenProcess(hProcess.addressof(), ProcessAccessMask::QueryInformation, ObjectAttributes(nullptr), &cid);
			if (!NT_SUCCESS(status))
				::SetLastError(RtlNtStatusToDosError(status));
		}
		else {
			hProcess.reset(NtCurrentProcess());
		}
		if (!hProcess)
			return false;
		return NT_SUCCESS(::NtOpenProcessToken(hProcess.get(), access, m_hObject.addressof()));
	}

	wstring Token::GetUserName(bool includeDomain) const {
		BYTE buffer[512];
		ULONG len;
		if (NT_SUCCESS(::NtQueryInformationToken(Handle(), TokenUser, buffer, sizeof(buffer), &len))) {
			auto pti = reinterpret_cast<TOKEN_USER*>(buffer);
			SID_NAME_USE use;
			WCHAR name[257], domain[64];
			DWORD lname = _countof(name), ldomain = _countof(domain);
			::LookupAccountSid(nullptr, pti->User.Sid, name, &lname, domain, &ldomain, &use);
			return includeDomain ? wstring(domain) + L"\\" + name : name;
		}
		return L"";
	}

	Sid Token::GetUserSid() const {
		BYTE buffer[512];
		ULONG len;
		if (NT_SUCCESS(::NtQueryInformationToken(Handle(), TokenUser, buffer, sizeof(buffer), &len))) {
			auto pti = reinterpret_cast<TOKEN_USER*>(buffer);
			return Sid(pti->User.Sid);
		}
		return Sid();
	}

	VirtualizationState Token::GetVirtualizationState() const {
		ULONG virt = 0;
		DWORD len;
		if (!::GetTokenInformation(Handle(), TokenVirtualizationAllowed, &virt, sizeof(virt), &len))
			return VirtualizationState::Unknown;

		if (!virt)
			return VirtualizationState::NotAllowed;

		if (::GetTokenInformation(Handle(), TokenVirtualizationEnabled, &virt, sizeof(virt), &len))
			return virt ? VirtualizationState::Enabled : VirtualizationState::Disabled;

		return VirtualizationState::Unknown;
	}

}

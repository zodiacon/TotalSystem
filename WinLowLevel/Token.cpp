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

	Sid::operator PSID() const {
		return Get();
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

	wstring Sid::AsWString() const {
		PWSTR ssid;
		wstring result;
		if (::ConvertSidToStringSidW(Get(), &ssid)) {
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
		SE_TOKEN_USER user;
		ULONG len;
		if (NT_SUCCESS(::NtQueryInformationToken(Handle(), TokenUser, &user, sizeof(user), &len))) {
			return Sid(&user.Sid);
		}
		return Sid();
	}

	uint32_t Token::GetSessionId() const {
		uint32_t session = -1;
		ULONG len;
		::NtQueryInformationToken(Handle(), ::TokenSessionId, &session, sizeof(session), &len);
		return session;
	}

	int64_t Token::GetLogonSessionId() const {
		return GetStatistics().AuthenticationId;
	}

	TokenType Token::GetType() const {
		auto type{ TokenType::Invalid };
		ULONG len;
		::NtQueryInformationToken(Handle(), ::TokenType, &type, sizeof(type), &len);
		return type;
	}

	TokenStatistics Token::GetStatistics() const {
		TokenStatistics stats{};
		ULONG len;
		::NtQueryInformationToken(Handle(), ::TokenStatistics, &stats, sizeof(stats), &len);
		return stats;
	}

	VirtualizationState Token::GetVirtualizationState() const {
		ULONG virt = 0;
		DWORD len;
		if (!NT_SUCCESS(::NtQueryInformationToken(Handle(), TokenVirtualizationAllowed, &virt, sizeof(virt), &len)))
			return VirtualizationState::Error;

		if (!virt)
			return VirtualizationState::NotAllowed;

		if (NT_SUCCESS(::NtQueryInformationToken(Handle(), TokenVirtualizationEnabled, &virt, sizeof(virt), &len)))
			return virt ? VirtualizationState::Enabled : VirtualizationState::Disabled;

		return VirtualizationState::Error;
	}

}

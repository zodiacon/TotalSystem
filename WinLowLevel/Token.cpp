#include "pch.h"
#include "WinLowLevel.h"

namespace WinLL {
	using namespace std;

	bool Token::Open(TokenAccessMask access, uint32_t pid) {
		wil::unique_handle hProcess;
		if (pid) {
			CLIENT_ID cid = { UlongToHandle(pid) };
			::NtOpenProcess(hProcess.addressof(), ProcessAccessMask::QueryInformation, ObjectAttributes(nullptr), &cid);
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
}

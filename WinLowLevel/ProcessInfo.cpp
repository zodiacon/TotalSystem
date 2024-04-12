#include "pch.h"
#include "ProcessInfo.h"
#include "WinLowLevel.h"

namespace WinLL {
	const std::wstring& ProcessInfo::GetImageName() const {
		return m_ProcessName;
	}

	const std::wstring& ProcessInfo::GetPackageFullName() const {
		return m_PackageFullName;
	}

	const std::wstring& ProcessInfo::GetNativeImagePath() const {
		return m_NativeImagePath;
	}

	const std::vector<std::shared_ptr<ThreadInfo>>& ProcessInfo::GetThreads() const {
		return m_Threads;
	}

	const std::wstring& ProcessInfo::GetUserName(bool includeDomain) const {
		if (m_UserName.empty()) {
			if (Id <= 4) {
				m_FullUserName = L"NT AUTHORITY\\SYSTEM";
				m_UserName = L"SYSTEM";
			}
			else {
				if (UserSid[0]) {
					WCHAR name[64], domain[64];
					DWORD lname = _countof(name), ldomain = _countof(domain);
					SID_NAME_USE use;
					if (::LookupAccountSid(nullptr, (PSID)UserSid, name, &lname, domain, &ldomain, &use)) {
						m_FullUserName = domain + std::wstring(L"\\") + name;
						m_UserName = name;
					}
				}
				else {
					Token token;
					if (token.Open(TokenAccessMask::Query, Id)) {
						m_FullUserName = token.GetUserName(true);
						m_UserName = m_FullUserName.substr(m_FullUserName.rfind(L'\\') + 1);
					}
				}
			}
		}
		return includeDomain ? m_FullUserName : m_UserName;
	}

	void ProcessInfo::AddThread(std::shared_ptr<ThreadInfo> thread) {
		m_Threads.push_back(thread);
	}

	void ProcessInfo::ClearThreads() {
		m_Threads.clear();
		m_Threads.reserve(16);
	}
}

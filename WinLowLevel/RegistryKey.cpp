#include "pch.h"
#include "WinLowLevel.h"
#include <assert.h>

using namespace WinLL;

void RegistryKey::Close() {
	if (Handle() && ((DWORD_PTR)(HKEY)Handle() & 0xf000000000000000) == 0) {
		::RegCloseKey((HKEY)m_hObject.get());
		m_hObject.reset();
	}
}

LSTATUS RegistryKey::Open(HKEY parent, PCWSTR path, DWORD access) {
	assert(Handle() == nullptr);
	auto error = ::RegOpenKeyEx(parent, path, 0, access, (PHKEY)m_hObject.addressof());
	return error;
}

LSTATUS RegistryKey::Create(HKEY parent, PCWSTR path, DWORD access) {
	assert(Handle() == nullptr);
	auto error = ::RegCreateKeyEx(parent, path, 0, nullptr, 0, access, nullptr, (PHKEY)m_hObject.addressof(), nullptr);
	return error;
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetStringValue(LPCTSTR pszValueName, LPCTSTR pszValue, DWORD dwType) noexcept {
	return ::RegSetValueEx((HKEY)Handle(), pszValueName, 0, dwType, reinterpret_cast<const BYTE*>(pszValue), (static_cast<DWORD>(wcslen(pszValue)) + 1) * sizeof(TCHAR));
}

_Use_decl_annotations_
LSTATUS RegistryKey::SetMultiStringValue(LPCTSTR pszValueName, LPCTSTR pszValue) noexcept {
	LPCTSTR pszTemp;
	ULONG nBytes;
	ULONG nLength;

	assert(Handle());

	nBytes = 0;
	pszTemp = pszValue;
	do {
		nLength = static_cast<ULONG>(wcslen(pszTemp)) + 1;
		pszTemp += nLength;
		nBytes += nLength * sizeof(TCHAR);
	} while (nLength != 1);

	return ::RegSetValueEx((HKEY)Handle(), pszValueName, 0, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(pszValue), nBytes);
}

LSTATUS RegistryKey::SetValue(_In_opt_z_ LPCTSTR pszValueName, _In_ DWORD dwType, _In_opt_ const void* pValue, _In_ ULONG nBytes) noexcept {
	assert(Handle());
	return ::RegSetValueEx((HKEY)Handle(), pszValueName, 0, dwType, static_cast<const BYTE*>(pValue), nBytes);
}

LSTATUS RegistryKey::SetBinaryValue(_In_opt_z_ LPCTSTR pszValueName, _In_opt_ const void* pData, _In_ ULONG nBytes) noexcept {
	assert(Handle());
	return ::RegSetValueEx((HKEY)Handle(), pszValueName, 0, REG_BINARY, reinterpret_cast<const BYTE*>(pData), nBytes);
}

LSTATUS RegistryKey::SetDWORDValue(_In_opt_z_ LPCTSTR pszValueName, _In_ DWORD dwValue) noexcept {
	assert(Handle());
	return ::RegSetValueEx((HKEY)Handle(), pszValueName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&dwValue), sizeof(DWORD));
}

LSTATUS RegistryKey::SetQWORDValue(_In_opt_z_ LPCTSTR pszValueName, _In_ ULONGLONG qwValue) noexcept {
	assert(Handle());
	return ::RegSetValueEx((HKEY)Handle(), pszValueName, 0, REG_QWORD, reinterpret_cast<const BYTE*>(&qwValue), sizeof(ULONGLONG));
}

LSTATUS RegistryKey::SetValue(_In_ DWORD dwValue, _In_opt_z_ LPCTSTR pszValueName) noexcept {
	assert(Handle());
	return SetDWORDValue(pszValueName, dwValue);
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) noexcept {
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	assert(Handle());
	nBytes = (*pnChars) * sizeof(TCHAR);
	*pnChars = 0;
	lRes = ::RegQueryValueEx((HKEY)Handle(), pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(pszValue), &nBytes);

	if (lRes != ERROR_SUCCESS) {
		return lRes;
	}

	if (dwType != REG_SZ && dwType != REG_EXPAND_SZ) {
		return ERROR_INVALID_DATA;
	}

	if (pszValue != NULL) {
		if (nBytes != 0) {
			if ((nBytes % sizeof(TCHAR) != 0) || (pszValue[nBytes / sizeof(TCHAR) - 1] != 0)) {
				return ERROR_INVALID_DATA;
			}
		}
		else {
			pszValue[0] = L'\0';
		}
	}

	*pnChars = nBytes / sizeof(WCHAR);
	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryMultiStringValue(LPCTSTR pszValueName, LPTSTR pszValue, ULONG* pnChars) noexcept {
	LONG lRes;
	DWORD dwType;
	ULONG nBytes;

	assert(Handle());
	assert(pnChars);

	if (pszValue != nullptr && *pnChars < 2)
		return ERROR_INSUFFICIENT_BUFFER;

	nBytes = (*pnChars) * sizeof(WCHAR);
	*pnChars = 0;

	lRes = ::RegQueryValueEx((HKEY)Handle(), pszValueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(pszValue), &nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_MULTI_SZ)
		return ERROR_INVALID_DATA;
	if (pszValue != nullptr && (nBytes % sizeof(WCHAR) != 0 ||
		nBytes / sizeof(WCHAR) < 1 || pszValue[nBytes / sizeof(WCHAR) - 1] != 0 ||
		((nBytes / sizeof(WCHAR)) > 1 && pszValue[nBytes / sizeof(TCHAR) - 2] != 0)))
		return ERROR_INVALID_DATA;

	*pnChars = nBytes / sizeof(WCHAR);

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LONG RegistryKey::QueryBinaryValue(LPCTSTR pszValueName, void* pValue, ULONG* pnBytes) noexcept {
	LONG lRes;
	DWORD dwType;

	assert(pnBytes);
	assert(Handle());

	lRes = ::RegQueryValueEx((HKEY)Handle(), pszValueName, nullptr, &dwType, static_cast<LPBYTE>(pValue), pnBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_BINARY && dwType != REG_RESOURCE_LIST && dwType != REG_RESOURCE_REQUIREMENTS_LIST && dwType != REG_FULL_RESOURCE_DESCRIPTOR)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryValue(LPCTSTR pszValueName, DWORD* pdwType, void* pData, ULONG* pnBytes) noexcept {
	assert((HKEY)Handle() != NULL);

	return(::RegQueryValueEx((HKEY)Handle(), pszValueName, NULL, pdwType, static_cast<LPBYTE>(pData), pnBytes));
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryDWORDValue(LPCTSTR pszValueName, DWORD& dwValue) noexcept {
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	assert((HKEY)Handle() != NULL);

	nBytes = sizeof(DWORD);
	lRes = ::RegQueryValueEx((HKEY)Handle(), pszValueName, nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue),
		&nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_DWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::QueryQWORDValue(LPCTSTR pszValueName, ULONGLONG& qwValue) noexcept {
	LONG lRes;
	ULONG nBytes;
	DWORD dwType;

	assert(Handle());

	nBytes = sizeof(ULONGLONG);
	lRes = ::RegQueryValueEx((HKEY)Handle(), pszValueName, NULL, &dwType, reinterpret_cast<LPBYTE>(&qwValue), &nBytes);
	if (lRes != ERROR_SUCCESS)
		return lRes;
	if (dwType != REG_QWORD)
		return ERROR_INVALID_DATA;

	return ERROR_SUCCESS;
}

_Use_decl_annotations_
LSTATUS RegistryKey::DeleteValue(LPCTSTR lpszValue) noexcept {
	assert(Handle());
	return ::RegDeleteValue((HKEY)Handle(), (LPTSTR)lpszValue);
}



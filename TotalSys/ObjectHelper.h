#pragma once

#include <ProcessManager.h>

struct HandleInfoEx;

struct ObjectHelper abstract final {
	static std::string GetObjectDetails(HandleInfoEx* hi, std::wstring const& type, WinLL::ProcessManager<>* pm = nullptr);
	static bool CloseHandle(HandleInfoEx* hi);
	static wil::unique_handle DupHandle(HANDLE h, DWORD pid, ACCESS_MASK access = GENERIC_READ, DWORD flags = 0);
	static std::wstring NativePathToDosPath(std::wstring const& path);

private:
	static std::wstring GetProcessImageName(uint32_t pid, WinLL::ProcessManager<>* pm);
};


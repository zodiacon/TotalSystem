#pragma once

#include <ProcessManager.h>

struct HandleInfoEx;

struct ObjectHelper abstract final {
	static std::string GetObjectDetails(HANDLE h, HandleInfoEx* hi, std::wstring const& type, WinLL::ProcessManager<>* pm = nullptr);
	static bool CloseHandle(HandleInfoEx* hi);
};


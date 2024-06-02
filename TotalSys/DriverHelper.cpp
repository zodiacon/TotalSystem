#include "pch.h"
#include "DriverHelper.h"
#include <WinLowLevel.h>
#include <SecurityHelper.h>
#include "resource.h"

#include "..\\KTotalSys\\KTotalSysPublic.h"

using namespace WinLL;

HANDLE DriverHelper::OpenHandle(void* pObject, ACCESS_MASK access) {
	if (!OpenDevice())
		return nullptr;

	OpenObjectData data;
	data.Access = access;
	data.Address = pObject;

	DWORD bytes;
	HANDLE hObject;
	return ::DeviceIoControl(s_hDevice, IOCTL_KTOTALSYS_OPEN_OBJECT, &data, sizeof(data),
		&hObject, sizeof(hObject), &bytes, nullptr)
		? hObject : nullptr;
}

HANDLE DriverHelper::DupHandle(HANDLE hObject, ULONG pid, ACCESS_MASK access, DWORD flags) {
	HANDLE hTarget{ nullptr };
	if (OpenDevice()) {
		DupHandleData data;
		data.AccessMask = access;
		data.Handle = HandleToUlong(hObject);
		data.SourcePid = pid;
		data.Flags = flags;

		DWORD bytes;
		::DeviceIoControl(s_hDevice, IOCTL_KTOTALSYS_DUP_HANDLE, &data, sizeof(data),
			&hTarget, sizeof(hTarget), &bytes, nullptr);
	}
	return hTarget;
}

HANDLE DriverHelper::OpenProcess(DWORD pid, ACCESS_MASK access) {
	HANDLE hProcess{ nullptr };
	if (OpenDevice()) {
		OpenProcessThreadData data;
		data.AccessMask = access;
		data.Id = pid;
		DWORD bytes;

		::DeviceIoControl(s_hDevice, IOCTL_KTOTALSYS_OPEN_PROCESS, &data, sizeof(data),
			&hProcess, sizeof(hProcess), &bytes, nullptr);
	}
	if(!hProcess)
		hProcess = ::OpenProcess(access, FALSE, pid);
	return hProcess;
}

HANDLE DriverHelper::OpenThread(DWORD tid, ACCESS_MASK access) {
	HANDLE hThread{ nullptr };
	if (OpenDevice()) {
		OpenProcessThreadData data;
		data.AccessMask = access;
		data.Id = tid;
		DWORD bytes;

		::DeviceIoControl(s_hDevice, IOCTL_KTOTALSYS_OPEN_THREAD, &data, sizeof(data),
			&hThread, sizeof(hThread), &bytes, nullptr);
	}
	if(!hThread)
		::OpenThread(access, FALSE, tid);
	return hThread;
}

HANDLE DriverHelper::OpenToken(DWORD pid, ACCESS_MASK access) {
	HANDLE hToken{ nullptr };
	if (OpenDevice()) {
		OpenProcessThreadData data;
		data.AccessMask = access;
		data.Id = pid;
		DWORD bytes;

		::DeviceIoControl(s_hDevice, IOCTL_KTOTALSYS_OPEN_PROCESS_TOKEN, &data, sizeof(data),
			&hToken, sizeof(hToken), &bytes, nullptr);
	}
	if (!hToken) {
		auto hProcess = OpenProcess(pid, PROCESS_QUERY_INFORMATION);
		if (hProcess) {
			::OpenProcessToken(hProcess, access, &hToken);
			::CloseHandle(hProcess);
		}
	}
	return hToken;
}

PVOID DriverHelper::GetObjectAddress(HANDLE hObject) {
	if (!OpenDevice())
		return nullptr;

	PVOID address = nullptr;
	DWORD bytes;
	::DeviceIoControl(s_hDevice, IOCTL_KTOTALSYS_GET_OBJECT_ADDRESS, &hObject, sizeof(hObject), &address, sizeof(address), &bytes, nullptr);
	return address;
}

USHORT DriverHelper::GetVersion() {
	USHORT version = 0;
	if (!OpenDevice())
		return 0;

	DWORD bytes;
	::DeviceIoControl(s_hDevice, IOCTL_KTOTALSYS_GET_VERSION, nullptr, 0, &version, sizeof(version), &bytes, nullptr);
	return version;
}

USHORT DriverHelper::GetCurrentVersion() {
	return DRIVER_CURRENT_VERSION;
}

bool DriverHelper::CloseDevice() {
	if (s_hDevice) {
		::CloseHandle(s_hDevice);
		s_hDevice = nullptr;
	}
	return true;
}

bool DriverHelper::IsDriverLoaded() {
	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::OpenService(hScm.get(), L"KObjExp", SERVICE_QUERY_STATUS));
	if (!hService)
		return false;

	SERVICE_STATUS status;
	if (!::QueryServiceStatus(hService.get(), &status))
		return false;

	return status.dwCurrentState == SERVICE_RUNNING;
}

bool DriverHelper::OpenDevice() {
	if (s_hDevice == INVALID_HANDLE_VALUE)
		return false;

	if (!s_hDevice) {
		s_hDevice = ::CreateFile(L"\\\\.\\KTotalSys", GENERIC_WRITE | GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, 0, nullptr);
		if (s_hDevice == INVALID_HANDLE_VALUE) {
			if (::GetLastError() == ERROR_FILE_NOT_FOUND) {
				if (!LoadDriver())
					return false;
				s_hDevice = nullptr;
				return OpenDevice();
			}		
			return false;
		}
	}
	return true;
}

bool DriverHelper::LoadDriver(bool load) {
	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::OpenService(hScm.get(), L"KTotalSys", SERVICE_ALL_ACCESS));
	if (!hService)
		return false;

	SERVICE_STATUS status;
	bool success = true;
	DWORD targetState;
	::QueryServiceStatus(hService.get(), &status);
	if (load && status.dwCurrentState != (targetState = SERVICE_RUNNING))
		success = ::StartService(hService.get(), 0, nullptr);
	else if (!load && status.dwCurrentState != (targetState = SERVICE_STOPPED))
		success = ::ControlService(hService.get(), SERVICE_CONTROL_STOP, &status);
	else
		return true;

	if (!success)
		return false;

	for (int i = 0; i < 20; i++) {
		::QueryServiceStatus(hService.get(), &status);
		if (status.dwCurrentState == targetState)
			return true;
		::Sleep(200);
	}
	return false;
}

bool DriverHelper::InstallDriver(bool justCopy) {
	if (!SecurityHelper::IsRunningElevated())
		return false;

	auto hRes = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_DRIVER), L"BIN");
	if (!hRes)
		return false;

	auto hGlobal = ::LoadResource(nullptr, hRes);
	if (!hGlobal)
		return false;

	auto size = ::SizeofResource(nullptr, hRes);
	void* pBuffer = ::LockResource(hGlobal);

	auto path = SystemInformation::GetSystemDirectory();
	path += L"\\Drivers\\KTotalSys.sys";
	wil::unique_hfile hFile(::CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_SYSTEM, nullptr));
	if (!hFile)
		return false;

	DWORD bytes = 0;
	::WriteFile(hFile.get(), pBuffer, size, &bytes, nullptr);
	if (bytes != size)
		return false;

	if (justCopy)
		return true;

	wil::unique_schandle hScm(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!hScm)
		return false;

	wil::unique_schandle hService(::CreateService(hScm.get(), L"KTotalSys", nullptr, SERVICE_ALL_ACCESS, SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL, path.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr));
	bool success = false;
	if (hService)
		success = ::StartService(hService.get(), 0, nullptr);
	return success;
}

bool DriverHelper::UpdateDriver() {
	if (!LoadDriver(false))
		return false;
	if (!InstallDriver(true))
		return false;
	if (!LoadDriver())
		return false;

	return true;
}

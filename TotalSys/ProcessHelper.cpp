#include "pch.h"
#include "ProcessHelper.h"
#include <ranges>
#include "ProcessInfoEx.h"
#include "DriverHelper.h"

using namespace std;
using namespace std::views;
using namespace std::ranges;
using namespace WinLL;

vector<uint32_t> ProcessHelper::GetProcessIdsByName(std::vector<std::shared_ptr<ProcessInfoEx>> const& processes, wstring_view name) {
    auto pids = processes | filter([&](auto& p) { return _wcsicmp(p->GetImageName().c_str(), name.data()) == 0; }) | views::transform([](auto& p) { return p->Id; }) | to<vector>();
    return pids;

}

ImColor ProcessHelper::GetColorByCPU(float cpu) {
    return ImColor::HSV(1.0f, cpu / 60 + .4f, .3f);
}

bool ProcessHelper::InjectDll(uint32_t pid, std::wstring_view dllPath) {
    wil::unique_handle hProcess(DriverHelper::OpenProcess(pid, ProcessAccessMask::VmOperation | ProcessAccessMask::VmWrite | ProcessAccessMask::CreateThread));
    if (!hProcess)
        return false;

    auto buffer = ::VirtualAllocEx(hProcess.get(), nullptr, 1 << 12, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!buffer)
        return false;

    if (!::WriteProcessMemory(hProcess.get(), buffer, dllPath.data(), dllPath.size() * sizeof(WCHAR), nullptr))
        return false;

    wil::unique_handle hThread(::CreateRemoteThread(hProcess.get(), nullptr, 0,
        (LPTHREAD_START_ROUTINE)::GetProcAddress(::GetModuleHandleW(L"kernel32"), "LoadLibraryW"), buffer, 0, nullptr));
    if (!hThread)
        return false;

    auto success = ::WaitForSingleObject(hThread.get(), 5000) == WAIT_OBJECT_0;
    if (success)
        ::VirtualFreeEx(hProcess.get(), buffer, 0, MEM_RELEASE);

    return success;
}

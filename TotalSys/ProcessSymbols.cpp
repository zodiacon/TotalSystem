#include "pch.h"
#include "ProcessSymbols.h"
#include <DbgHelp.h>
#include <ProcessModuleTracker.h>
#include "FormatHelper.h"
#include <KnownFolders.h>

bool ProcessSymbols::Load(HANDLE hProcess) {
    assert(hProcess);
    if (::SymInitialize(hProcess, nullptr, FALSE)) {
        m_hProcess = hProcess;
        return true;
    }
    m_hProcess = nullptr;
    return false;
}

void ProcessSymbols::Cleanup() {
    if (m_hProcess) {
        ::SymCleanup(m_hProcess);
        m_hProcess = nullptr;
    }
}

ProcessSymbols::operator bool() const {
    return m_hProcess != nullptr;
}

bool ProcessSymbols::Init() {
    if (s_hDbgHelp) {
        ::FreeLibrary(s_hDbgHelp);
        s_SymLoadModulesEx = nullptr;
    }

    //
    // look for debugging tools for windows
    //
    auto path = FormatHelper::GetFolderPath(FOLDERID_ProgramFilesX86) + L"\\Windows Kits\\10\\Debuggers\\x64\\DbgHelp.dll";
    s_hDbgHelp = ::LoadLibrary(path.c_str());
    if (s_hDbgHelp) {
        s_SymLoadModulesEx = (decltype(s_SymLoadModulesEx))::GetProcAddress(s_hDbgHelp, "SymLoadModuleExW");
    }
    return s_SymLoadModulesEx != nullptr;
}

ProcessSymbols::~ProcessSymbols() {
    Cleanup();
}

SYMBOL_INFO const* ProcessSymbols::GetSymbolFromAddress(uint64_t address, uint64_t* disp, uint64_t* modBase) const {
    if (!m_ModulesEnumerated) {
        m_ModulesEnumerated = true;
        WinLLX::ProcessModuleTracker<> tracker;
        if (!tracker.TrackProcess(::GetProcessId(m_hProcess)))
            return nullptr;

        tracker.Update();
        for (auto& m : tracker.GetModules()) {
            if (!m->Path.empty() && m->Type == WinLLX::MapType::Image) {
                if (s_SymLoadModulesEx(m_hProcess, nullptr, m->Path.c_str(), m->Name.c_str(), (DWORD64)m->Base, m->ModuleSize, nullptr, 0)) {
                    m_Modules.insert({ (uint64_t)m->Base, m });
                }
            }
        }
    }
    thread_local static SYMBOL_INFO_PACKAGE symInfo{ sizeof(SYMBOL_INFO) };
    symInfo.si.MaxNameLen = MAX_SYM_NAME;
    if (modBase) {
        for (auto& [base, m] : m_Modules)
            if (address >= base && address < base + m->ModuleSize) {
                *modBase = base;
                break;
            }
    }
    return ::SymFromAddr(m_hProcess, address, disp, &symInfo.si) ? &symInfo.si : nullptr;
}

std::wstring const& ProcessSymbols::GetModuleName(uint64_t baseAddress) const {
    if (auto it = m_Modules.find(baseAddress); it != m_Modules.end())
        return it->second->Name;

    static std::wstring empty;
    return empty;
}

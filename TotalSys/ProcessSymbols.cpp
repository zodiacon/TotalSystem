#include "pch.h"
#include "ProcessSymbols.h"
#include <DbgHelp.h>
#include <ProcessModuleTracker.h>
#include "FormatHelper.h"
#include <KnownFolders.h>
#include <assert.h>
#include <utility>
#include "DriverHelper.h"

using namespace WinLL;
using namespace std;

bool ProcessSymbols::Load(HANDLE hProcess) {
	assert(hProcess);
	if (s_SymInitialize(hProcess, nullptr, FALSE)) {
		m_hProcess = hProcess;
		return true;
	}
	m_hProcess = nullptr;
	return false;
}

void ProcessSymbols::Cleanup() {
	if (m_hProcess) {
		s_SymCleanup(m_hProcess);
		m_hProcess = nullptr;
	}
}

ProcessSymbols::operator bool() const {
	return m_hProcess != nullptr;
}

bool ProcessSymbols::Init() {
	if (s_hDbgHelp) {
		::FreeLibrary(s_hDbgHelp);
		s_SymLoadModuleEx = nullptr;
	}

	//
	// look for debugging tools for windows
	//
	auto path = FormatHelper::GetFolderPath(FOLDERID_ProgramFilesX86) + L"\\Windows Kits\\10\\Debuggers\\x64\\DbgHelp.dll";
	s_hDbgHelp = ::LoadLibrary(path.c_str());

	//
	// if not found, load from system32
	// symbols will not be available because symsrv.dll is not part of Windows
	//
	if (!s_hDbgHelp)
		s_hDbgHelp = ::LoadLibraryW(L"Dbghelp.Dll");
	if (s_hDbgHelp) {
		s_SymLoadModuleEx = (decltype(s_SymLoadModuleEx))::GetProcAddress(s_hDbgHelp, "SymLoadModuleExW");
		s_StackWalk = (decltype(s_StackWalk))::GetProcAddress(s_hDbgHelp, "StackWalk64");
		assert(s_StackWalk);
		s_SymInitialize = (decltype(s_SymInitialize))::GetProcAddress(s_hDbgHelp, "SymInitialize");
		s_SymCleanup = (decltype(s_SymCleanup))::GetProcAddress(s_hDbgHelp, "SymCleanup");
		s_SymFromAddr = (decltype(s_SymFromAddr))::GetProcAddress(s_hDbgHelp, "SymFromAddr");
		s_SymFunctionTableAccess64 = (decltype(s_SymFunctionTableAccess64))::GetProcAddress(s_hDbgHelp, "SymFunctionTableAccess64");
		s_SymGetModuleBase64 = (decltype(s_SymGetModuleBase64))::GetProcAddress(s_hDbgHelp, "SymGetModuleBase64");
		s_SymSetOptions = (decltype(s_SymSetOptions))::GetProcAddress(s_hDbgHelp, "SymSetOptions");
		s_SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
	}
	return s_SymLoadModuleEx != nullptr;
}

ProcessSymbols::~ProcessSymbols() {
	Cleanup();
}

SYMBOL_INFO const* ProcessSymbols::GetSymbolFromAddress(uint64_t address, uint64_t* disp, uint64_t* modBase) const {
	assert(m_hProcess);
	if (!LoadModules())
		return nullptr;

	thread_local static SYMBOL_INFO_PACKAGE symInfo{ sizeof(SYMBOL_INFO) };
	symInfo.si.MaxNameLen = MAX_SYM_NAME;
	assert(!m_Modules.empty());
	for (auto& [base, m] : m_Modules) {
		if (address >= base && address < base + m->ModuleSize) {
			if (modBase)
				*modBase = base;
			break;
		}
	}
	return s_SymFromAddr(m_hProcess, address, disp, &symInfo.si) ? &symInfo.si : nullptr;
}

std::wstring const& ProcessSymbols::GetModuleName(uint64_t baseAddress) const {
	if (auto it = m_Modules.find(baseAddress); it != m_Modules.end())
		return it->second->Name;

	static std::wstring empty;
	return empty;
}

std::vector<STACKFRAME64> ProcessSymbols::EnumThreadStack(uint32_t pid, uint32_t tid) const {
	Thread t;
	auto access = ThreadAccessMask::GetContext | ThreadAccessMask::QueryInformation | ThreadAccessMask::SuspendResume;
	if (!t.Open(tid, access)) {
		t.Attach(DriverHelper::OpenThread(tid, access));
	}
	if(!t)
		return {};

	Process p;
	if (!p.Open(pid, ProcessAccessMask::VmRead | ProcessAccessMask::QueryInformation))
		p.Attach(DriverHelper::OpenProcess(pid, ProcessAccessMask::VmRead | ProcessAccessMask::QueryInformation));
	if(!p)
		return {};

	assert(t.GetProcessId() == pid);
	LoadModules();
	vector<STACKFRAME64> frames;
	frames.reserve(16);
	CONTEXT ctx;
	ctx.ContextFlags = CONTEXT_FULL;

	t.Suspend();
	if (!::GetThreadContext(t.Handle(), &ctx)) {
		t.Resume();
		return {};
	}

	STACKFRAME64 frame{};
	frame.AddrPC.Offset = (DWORD64)ctx.Rip;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Offset = (DWORD64)ctx.Rsp;
	frame.AddrStack.Mode = AddrModeFlat;
	frame.AddrFrame.Offset = (DWORD64)ctx.Rbp;
	frame.AddrFrame.Mode = AddrModeFlat;

	auto read = [](auto hProcess, auto address, auto buffer, auto size, auto bytes) {
		return ::ReadProcessMemory(hProcess, (void*)address, buffer, size, (SIZE_T*)bytes);
		};

	{
		lock_guard locker(s_Lock);
		while (s_StackWalk(IMAGE_FILE_MACHINE_AMD64, p.Handle(), t.Handle(), &frame, &ctx, nullptr,
			s_SymFunctionTableAccess64, s_SymGetModuleBase64, nullptr)) {
			if (frame.AddrPC.Offset == 0)
				break;
			frames.push_back(frame);
		}
	}
	t.Resume();
	return frames;
}

bool ProcessSymbols::LoadModules() const {
	if (!m_ModulesEnumerated) {
		lock_guard locker(s_Lock);
		if (!m_ModulesEnumerated) {
			LoadKernelModules();
			WinLLX::ProcessModuleTracker<> tracker;
			if (!tracker.TrackProcess(::GetProcessId(m_hProcess)))
				return true;

			m_ModulesEnumerated = true;
			tracker.Update();
			for (auto& m : tracker.GetModules()) {
				if (!m->Path.empty() && m->Type == WinLLX::MapType::Image) {
					s_SymLoadModuleEx(m_hProcess, nullptr, m->Path.c_str(), m->Name.c_str(), (DWORD64)m->Base, m->ModuleSize, nullptr, 0);
					m_Modules.insert({ (uint64_t)m->Base, m });
				}
			}
		}
	}
	return true;
}

bool ProcessSymbols::LoadKernelModules() const {
	auto size = 1 << 20;
	auto buffer = std::make_unique<BYTE[]>(size);
	if (0 != ::NtQuerySystemInformation(SystemModuleInformation, buffer.get(), size, nullptr))
		return false;

	auto p = (RTL_PROCESS_MODULES*)buffer.get();
	WCHAR name[32];
	auto dir = SystemInformation::GetSystemDir();
	auto count = p->NumberOfModules;
	DWORD64 address = 0;
	for (ULONG i = 0; i < min(count, ULONG(3)); i++) {
		auto& module = p->Modules[i];
		auto m = make_shared<WinLLX::ModuleInfo>();
		m->Base = module.ImageBase;
		m->ModuleSize = module.ImageSize;
		::GetDeviceDriverBaseName(m->Base, name, _countof(name));
		m->Path = dir + L"\\" + name;
		if(address = s_SymLoadModuleEx(m_hProcess, nullptr, m->Path.c_str(), m->Name.c_str(), (DWORD64)m->Base, m->ModuleSize, nullptr, 0)) {
			assert(address == (DWORD64)m->Base);
			m_Modules.insert({ address, move(m) });
		}
	}
	return false;
}

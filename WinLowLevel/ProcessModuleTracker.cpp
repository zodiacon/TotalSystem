#include "pch.h"
#include "ProcessModuleTracker.h"
#include <Psapi.h>
#include <ImageHlp.h>
#include "FileHelper.h"

using namespace WinLL;

ProcessModuleTracker::ProcessModuleTracker(DWORD pid) {
	m_Handle.reset(::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid));
	if (m_Handle)
		::IsWow64Process(m_Handle.get(), &m_IsWow64);
}

ProcessModuleTracker::ProcessModuleTracker(HANDLE hProcess) : m_Handle(hProcess) {
	::IsWow64Process(m_Handle.get(), &m_IsWow64);
}

std::shared_ptr<ModuleInfo> ProcessModuleTracker::FillModule(const MODULEENTRY32& me) {
	auto mi = std::make_shared<ModuleInfo>();
	mi->Name = me.szModule;
	mi->Path = me.szExePath;
	mi->Base = (void*)me.hModule;
	mi->ModuleSize = me.modBaseSize;
	return mi;
}

std::shared_ptr<ModuleInfo> ProcessModuleTracker::FillModule(const MEMORY_BASIC_INFORMATION& mbi) {
	auto mi = std::make_shared<ModuleInfo>();
	mi->ModuleSize = 0;
	WCHAR name[MAX_PATH];
	mi->ImageBase = 0;
	if (::GetMappedFileName(m_Handle.get(), mbi.AllocationBase, name, _countof(name))) {
		mi->Path = FileHelper::GetDosNameFromNtName(name);
		mi->Name = ::wcsrchr(name, L'\\') + 1;
		BYTE buffer[1 << 12];
		if (::ReadProcessMemory(m_Handle.get(), mbi.BaseAddress, buffer, sizeof(buffer), nullptr)) {
			auto nt = ::ImageNtHeader(buffer);
			if (nt) {
				auto machine = nt->FileHeader.Machine;
				if (machine == IMAGE_FILE_MACHINE_ARM || machine == IMAGE_FILE_MACHINE_I386) {
					auto oh = (IMAGE_OPTIONAL_HEADER32*)&nt->OptionalHeader;
					mi->ImageBase = UlongToPtr(oh->ImageBase);
					mi->Characteristics = (DllCharacteristics)oh->DllCharacteristics;
				}
				else {
					mi->ImageBase = (PVOID)nt->OptionalHeader.ImageBase;
					mi->Characteristics = (DllCharacteristics)nt->OptionalHeader.DllCharacteristics;
				}
			}
		}
	}
	mi->Base = mbi.AllocationBase;
	mi->Type = mbi.Type == MEM_MAPPED ? MapType::Data : MapType::Image;
	return mi;
}

ProcessModuleTracker::operator bool() const {
	return m_Handle.is_valid();
}

uint32_t ProcessModuleTracker::EnumModules() {
	return m_Handle ? EnumModulesWithVirtualQuery() : EnumModulesWithToolHelp();
}

uint32_t ProcessModuleTracker::EnumModulesWithVirtualQuery() {
	bool first = m_Modules.empty();
	if (first) {
		m_Modules.reserve(128);
		m_ModuleMap.reserve(128);
		m_NewModules.reserve(8);
		m_UnloadedModules.reserve(8);
	}
	else {
		m_NewModules.clear();
		m_UnloadedModules.clear();
	}

	auto existing = m_ModuleMap;

	MEMORY_BASIC_INFORMATION mbi;
	const BYTE* address = nullptr;
	ModuleInfo* pmi = nullptr;
	while (::VirtualQueryEx(m_Handle.get(), address, &mbi, sizeof(mbi)) > 0) {
		if (mbi.State == MEM_COMMIT && mbi.Type != MEM_PRIVATE) {
			if (mbi.AllocationBase == mbi.BaseAddress) {
				auto mi = FillModule(mbi);
				pmi = mi.get();
				auto key = mi->Name + L"#" + std::to_wstring((size_t)mi->Base);
				if (first) {
					m_ModuleMap.insert({ key, mi });
					m_Modules.push_back(std::move(mi));
				}
				else {
					auto it = m_ModuleMap.find(key);
					if (it == m_ModuleMap.end()) {
						// new module
						m_Modules.push_back(mi);
						m_NewModules.push_back(mi);
						m_ModuleMap.insert({ key, mi });
					}
					else {
						existing.erase(key);
					}
				}
			}
			pmi->ModuleSize += (ULONG)mbi.RegionSize;
		}
		address += mbi.RegionSize;
	}

	for (auto& [key, mi] : existing) {
		m_ModuleMap.erase(key);
		m_UnloadedModules.push_back(mi);
	}

	return static_cast<uint32_t>(m_Modules.size());
}

uint32_t ProcessModuleTracker::EnumModulesWithToolHelp() {
	wil::unique_handle hSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, m_Pid));
	if (!hSnapshot)
		return 0;

	bool first = m_Modules.empty();
	if (first) {
		m_Modules.reserve(128);
		m_NewModules.reserve(8);
		m_UnloadedModules.reserve(8);
	}
	else {
		m_NewModules.clear();
		m_UnloadedModules.clear();
	}

	MODULEENTRY32 me;
	me.dwSize = sizeof(me);
	::Module32First(hSnapshot.get(), &me);

	auto existing = m_ModuleMap;
	do {
		if (first) {
			auto mi = FillModule(me);
			m_ModuleMap.insert({ mi->Path, mi });
			m_Modules.push_back(std::move(mi));
		}
		else {
			auto it = m_ModuleMap.find(me.szExePath);
			if (it == m_ModuleMap.end()) {
				// new module
				auto mi = FillModule(me);
				m_Modules.push_back(mi);
				m_NewModules.push_back(mi);
			}
			else {
				existing.erase(me.szExePath);
			}
		}
	} while (::Module32Next(hSnapshot.get(), &me));

	for (auto& [key, mi] : existing)
		m_UnloadedModules.push_back(mi);

	return static_cast<uint32_t>(m_Modules.size());
}


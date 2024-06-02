#pragma once

#include <wil\resource.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <DbgHelp.h>
#include "FileHelper.h"

namespace WinLLX {
	enum class MapType {
		Image,
		Data
	};

	enum class DllCharacteristics : uint16_t {
		None = 0,
		HighEntropyVA = 0x20,
		DynamicBase = 0x40,
		ForceIntegrity = 0x80,
		NxCompat = 0x100,
		NoIsolation = 0x200,
		NoSEH = 0x400,
		NoBind = 0x800,
		AppContainer = 0x1000,
		WDMDriver = 0x2000,
		ControlFlowGuard = 0x4000,
		TerminalServerAware = 0x8000
	};
	DEFINE_ENUM_FLAG_OPERATORS(WinLLX::DllCharacteristics);

	struct ModuleInfo {
		std::wstring Name;
		std::wstring Path;
		void* ImageBase;
		void* Base;
		uint32_t ModuleSize;
		DllCharacteristics Characteristics;
		MapType Type;
	};

	template<typename TModule> requires std::is_base_of_v<ModuleInfo, TModule>
	struct ModuleTrackerBase abstract {
		virtual bool Update() = 0;
		virtual std::vector<std::shared_ptr<TModule>> const& GetModules() const = 0;
		virtual std::vector<std::shared_ptr<TModule>> const& GetNewModules() const = 0;
		virtual std::vector<std::shared_ptr<TModule>> const& GetUnloadedModules() const = 0;
	};

	template<typename TModule = ModuleInfo> 
		requires std::is_base_of_v<ModuleInfo, TModule>
	class ProcessModuleTracker final : public ModuleTrackerBase<TModule> {
	public:
		bool TrackProcess(HANDLE hProcess) {
			m_ModuleMap.clear();
			m_Modules.clear();
			if (!hProcess)
				return false;
			m_Handle.reset(hProcess);
			m_Pid = ::GetProcessId(hProcess);
			BOOL isWow = FALSE;
			::IsWow64Process(m_Handle.get(), &isWow);
			m_IsWow64 = isWow;
			return true;
		}

		bool TrackProcess(uint32_t pid) {
			m_Pid = pid;
			auto hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, FALSE, pid);
			if (hProcess) {
				return TrackProcess(hProcess);
			}
			return false;
		}

		operator bool() const {
			return m_Handle.is_valid();
		}
		bool Update() override {
			return m_Handle ? EnumModulesWithVirtualQuery() : EnumModulesWithToolHelp();
		}

		const std::vector<std::shared_ptr<TModule>>& GetModules() const override {
			return m_Modules;
		}

		const std::vector<std::shared_ptr<TModule>>& GetNewModules() const override {
			return m_NewModules;
		}

		const std::vector<std::shared_ptr<TModule>>& GetUnloadedModules() const override {
			return m_UnloadedModules;
		}

		uint32_t GetPid() const {
			return m_Pid;
		}

		ModuleInfo* GetModuleFromAddress(void* address) {
			for (auto& mod : m_Modules)
				if (mod->Base <= address && address < (PBYTE)mod->Base + mod->ModuleSize)
					return mod.get();
			return nullptr;
		}

	private:
		bool EnumModulesWithVirtualQuery() {
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
			TModule* pmi = nullptr;
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

			return true;
		}

		bool EnumModulesWithToolHelp() {
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

		std::shared_ptr<TModule> FillModule(const MODULEENTRY32& me) {
			auto mi = std::make_shared<TModule>();
			mi->Name = me.szModule;
			mi->Path = me.szExePath;
			mi->Base = (void*)me.hModule;
			mi->ModuleSize = me.modBaseSize;
			return mi;
		}
		
		std::shared_ptr<TModule> FillModule(const MEMORY_BASIC_INFORMATION& mbi) {
			auto mi = std::make_shared<TModule>();
			mi->ModuleSize = 0;
			WCHAR name[MAX_PATH];
			mi->ImageBase = 0;
			if (::GetMappedFileName(m_Handle.get(), mbi.AllocationBase, name, _countof(name))) {
				mi->Path = WinLL::FileHelper::GetDosNameFromNtName(name);
				mi->Name = ::wcsrchr(name, L'\\') + 1;
				BYTE buffer[1 << 12];
				if (::ReadProcessMemory(m_Handle.get(), mbi.BaseAddress, buffer, sizeof(buffer), nullptr)) {
					static const auto pImageNtHeader = (decltype(::ImageNtHeader)*)::GetProcAddress(::GetModuleHandle(L"Dbghelp"), "ImageNtHeader");

					auto nt = pImageNtHeader(buffer);
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


		std::vector<std::shared_ptr<TModule>> m_Modules, m_NewModules, m_UnloadedModules;
		std::unordered_map<std::wstring, std::shared_ptr<TModule>> m_ModuleMap;
		uint32_t m_Pid;
		wil::unique_handle m_Handle;
		bool m_IsWow64;

	};

}

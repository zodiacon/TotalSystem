#pragma once

#include <wil\resource.h>
#include "ProcessModuleTracker.h"

namespace WinLLX {
	struct KernelModuleInfo : ModuleInfo {
		std::string KName;
		std::string KPath;
		HANDLE hSection;
		uint32_t Flags;
		uint16_t LoadOrderIndex;
		uint16_t InitOrderIndex;
		uint16_t LoadCount;
		void* DefaultBase;
		uint32_t ImageChecksum;
		uint32_t TimeDateStamp;
	};

	template<typename T = KernelModuleInfo> requires std::is_base_of_v<KernelModuleInfo, T>
	class KernelModuleTracker final : public ModuleTrackerBase<T> {
	public:
		bool Update() override {
			DWORD size = 1 << 17;
			wil::unique_virtualalloc_ptr<> buffer(::VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

			if (!NT_SUCCESS(::NtQuerySystemInformation(SystemModuleInformationEx, buffer.get(), size, nullptr)))
				return 0;

			auto empty = m_Modules.empty();
			if (empty) {
				m_Modules.reserve(256);
				m_UnloadedModules.reserve(4);
				m_NewModules.reserve(4);
			}
			else {
				m_UnloadedModules.clear();
				m_NewModules.clear();
			}

			auto p = (RTL_PROCESS_MODULE_INFORMATION_EX*)buffer.get();
			CHAR winDir[MAX_PATH];
			::GetWindowsDirectoryA(winDir, _countof(winDir));
			static const std::string root("\\SystemRoot\\");

			auto modules = m_ModuleMap;
			for (;;) {
				if (p->BaseInfo.ImageBase == 0)
					break;

				std::shared_ptr<T> m;
				bool isNew = false;
				if (empty) {
					m = std::make_shared<T>();
					isNew = true;
				}
				else {
					auto it = modules.find(p->BaseInfo.ImageBase);
					if (it != modules.end()) {
						m = it->second;
						modules.erase(p->BaseInfo.ImageBase);
					}
					else {
						m = std::make_shared<T>();
						isNew = true;
					}
				}
				if (isNew) {
					m->Flags = p->BaseInfo.Flags;
					m->KPath = (const char*)p->BaseInfo.FullPathName;
					if (m->KPath.find(root) == 0)
						m->KPath = winDir + m->KPath.substr(root.size() - 1);
					m->Path.assign(m->KPath.begin(), m->KPath.end());
					m->Base = p->BaseInfo.MappedBase;
					m->ImageBase = p->BaseInfo.ImageBase;
					m->ModuleSize = p->BaseInfo.ImageSize;
					m->InitOrderIndex = p->BaseInfo.InitOrderIndex;
					m->LoadOrderIndex = p->BaseInfo.LoadOrderIndex;
					m->LoadCount = p->BaseInfo.LoadCount;
					m->hSection = p->BaseInfo.Section;
					m->DefaultBase = p->DefaultBase;
					m->ImageChecksum = p->ImageChecksum;
					m->TimeDateStamp = p->TimeDateStamp;
					m->KName = std::string((PCSTR)(p->BaseInfo.FullPathName + p->BaseInfo.OffsetToFileName));
					m->Name.assign(m->KName.begin(), m->KName.end());
					m_Modules.push_back(m);
					if (!empty)
						m_NewModules.push_back(m);
					m_ModuleMap.insert({ m->ImageBase, move(m) });
				}

				if (p->NextOffset == 0)
					break;
				p = (RTL_PROCESS_MODULE_INFORMATION_EX*)((BYTE*)p + p->NextOffset);
			}

			for (auto const& [p, m] : modules) {
				m_UnloadedModules.push_back(move(m));
				m_ModuleMap.erase(p);
			}

			return true;
		}

		const std::vector<std::shared_ptr<T>>& GetModules() const {
			return m_Modules;
		}

		const std::vector<std::shared_ptr<T>>& GetNewModules() const {
			return m_NewModules;
		}

		const std::vector<std::shared_ptr<T>>& GetUnloadedModules() const {
			return m_UnloadedModules;
		}

	private:
		std::vector<std::shared_ptr<T>> m_Modules, m_NewModules, m_UnloadedModules;
		std::unordered_map<PVOID, std::shared_ptr<T>> m_ModuleMap;
	};
}

#pragma once

#include <wil\resource.h>
#include <TlHelp32.h>

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

	class ProcessModuleTracker final {
	public:
		bool TrackProcess(uint32_t pid);
		explicit ProcessModuleTracker(HANDLE hProcess = nullptr);

		operator bool() const;
		uint32_t Update();
		const std::vector<std::shared_ptr<ModuleInfo>>& GetModules() const;
		const std::vector<std::shared_ptr<ModuleInfo>>& GetNewModules() const;
		const std::vector<std::shared_ptr<ModuleInfo>>& GetUnloadedModules() const;
		uint32_t GetPid() const;

	private:
		uint32_t EnumModulesWithVirtualQuery();
		uint32_t EnumModulesWithToolHelp();
		std::shared_ptr<ModuleInfo> FillModule(const MODULEENTRY32& me);
		std::shared_ptr<ModuleInfo> FillModule(const MEMORY_BASIC_INFORMATION& mbi);

		std::vector<std::shared_ptr<ModuleInfo>> m_Modules, m_NewModules, m_UnloadedModules;
		std::unordered_map<std::wstring, std::shared_ptr<ModuleInfo>> m_ModuleMap;
		uint32_t m_Pid;
		wil::unique_handle m_Handle;
		bool m_IsWow64;

	};

}

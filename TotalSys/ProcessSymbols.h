#pragma once

#include <ProcessModuleTracker.h>

typedef struct _SYMBOL_INFO SYMBOL_INFO, *PSYMBOL_INFO;

class ProcessSymbols {
public:
	static bool Init();

	ProcessSymbols() = default;
	ProcessSymbols(ProcessSymbols const&) = delete;
	ProcessSymbols& operator=(ProcessSymbols const&) = delete;
	~ProcessSymbols();

	bool Load(HANDLE hProcess);
	void Cleanup();
	operator bool() const;

	SYMBOL_INFO const* GetSymbolFromAddress(uint64_t address, uint64_t* disp = nullptr, uint64_t* modBase = nullptr) const;
	std::wstring const& GetModuleName(uint64_t baseAddress) const;

private:
	HANDLE m_hProcess;
	mutable bool m_ModulesEnumerated{ false };
	mutable std::unordered_map<uint64_t, std::shared_ptr<WinLLX::ModuleInfo>> m_Modules;
	inline static HMODULE s_hDbgHelp;
	inline static decltype(::SymLoadModuleExW)* s_SymLoadModulesEx;
};


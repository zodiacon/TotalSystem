#pragma once

#include <ProcessModuleTracker.h>
#include <atomic>
#include <mutex>

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

	[[nodiscard]] SYMBOL_INFO const* GetSymbolFromAddress(uint64_t address, uint64_t* disp = nullptr, uint64_t* modBase = nullptr) const;
	[[nodiscard]] std::wstring const& GetModuleName(uint64_t baseAddress) const;
	[[nodiscard]] std::vector<STACKFRAME64> EnumThreadStack(uint32_t pid, uint32_t tid) const;

private:
	bool LoadModules() const;
	bool LoadKernelModules() const;

	HANDLE m_hProcess;
	mutable std::atomic<bool> m_ModulesEnumerated{ false };
	mutable std::unordered_map<uint64_t, std::shared_ptr<WinLLX::ModuleInfo>> m_Modules;
	inline static HMODULE s_hDbgHelp;
	inline static std::mutex s_Lock;
	inline static decltype(::SymLoadModuleExW)* s_SymLoadModuleEx;
	inline static decltype(::StackWalk64)* s_StackWalk;
	inline static decltype(::SymInitialize)* s_SymInitialize;
	inline static decltype(::SymCleanup)* s_SymCleanup;
	inline static decltype(::SymFromAddr)* s_SymFromAddr;
	inline static decltype(::SymFunctionTableAccess64)* s_SymFunctionTableAccess64;
	inline static decltype(::SymGetModuleBase64)* s_SymGetModuleBase64;
	inline static decltype(::SymSetOptions)* s_SymSetOptions;
};


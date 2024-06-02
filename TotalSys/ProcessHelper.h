#pragma once

class ProcessInfoEx;

struct ProcessHelper abstract final {
	static std::vector<uint32_t> GetProcessIdsByName(std::vector<std::shared_ptr<ProcessInfoEx>> const& processes, std::wstring_view name);
	static ImColor GetColorByCPU(float cpu);
	static bool InjectDll(uint32_t pid, std::wstring_view dllPath);
};


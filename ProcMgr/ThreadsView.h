#pragma once

#include "ProcessInfoEx.h"
#include <functional>
#include <ThreadInfo.h>
#include <d3d11Image.h>

class ThreadInfoEx;

class ThreadsView {
public:
	void BuildThreadMenu();
	void BuildTable(std::shared_ptr<ProcessInfoEx>& p);

	static PCSTR StateToString(WinLL::ThreadState state);
	static PCSTR WaitReasonToString(WinLL::WaitReason reason);
	static D3D11Image& GetStateImage(WinLL::ThreadState state);

	static bool Init();

private:
	enum class Column {
		State, Id, WaitReason, CPU, BasePriority, CurrentPriority, CPUTime, CreateTime, KernelTime, UserTime, StartAddress,
		SuspendCount, Service, ContextSwitches, Cycles, CyclesDelta, MemoryPriority, IOPriority,
	};

	struct ColumnInfo {
		PCSTR Header;
		std::function<void(ThreadInfoEx& ti)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
	};

	inline static D3D11Image s_StateIcons[10];
};


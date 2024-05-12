#pragma once

#include "ProcessInfoEx.h"
#include <functional>
#include "ThreadInfoEx.h"
#include <d3d11Image.h>
#include <ProcessManager.h>
#include "ViewBase.h"

class ThreadInfoEx;

class ThreadsView : public ViewBase {
public:
	explicit ThreadsView(DefaultProcessManager* extarnal = nullptr);
	~ThreadsView();

	void BuildThreadMenu();
	void Build() override;
	void BuildTable(std::shared_ptr<ProcessInfoEx> p);
	void BuildToolBar();
	void Clear();
	bool RefreshAll(bool now = false);
	bool RefreshProcess(std::shared_ptr<ProcessInfoEx>& p, bool now = false);

	static PCSTR StateToString(WinLL::ThreadState state);
	static PCSTR WaitReasonToString(WinLL::WaitReason reason);
	static D3D11Image& GetStateImage(WinLL::ThreadState state);

	static bool Init();

private:
	enum class Column {
		State, Id, ProcessId, ProcessName, WaitReason, CPU, BasePriority, Priority, CPUTime, 
		CreateTime, KernelTime, UserTime, StartAddress, Win32StartAddress, Teb, StackBase, StackLimit,
		SuspendCount, Service, ContextSwitches, MemoryPriority, IOPriority, WaitTime,
	};

	struct ColumnInfo {
		PCSTR Header;
		std::function<void(std::shared_ptr<ThreadInfoEx>& ti)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
	};

	void DoSort(int column, bool asc);
	void CommonRefresh();

	inline static D3D11Image s_StateIcons[10];
	std::vector<std::shared_ptr<ThreadInfoEx>> m_Threads;
	std::shared_ptr<WinLL::ThreadInfo> m_SelectedThread;
	std::shared_ptr<ProcessInfoEx> m_Process;
	DefaultProcessManager m_ProcMgr;
	DefaultProcessManager* m_ActualProcMgr;
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	bool m_AllThreads{ false };
};


#pragma once

#include "ProcessInfoEx.h"
#include <functional>
#include <SimpleMessageBox.h>
#include "ThreadsView.h"
#include "ViewBase.h"
#include "SortedFilteredVector.h"
#include "ModulesView.h"

//#include "ProcessProperties.h"

struct ImGuiTableSortSpecsColumn;

class ProcessesView : public ViewBase {
	enum class Column {
		ProcessName, Pid, UserName, Session, CPU, ParentPid, CreateTime, Commit, BasePriority, Threads,
		Handles, WorkingSet, ExePath, CPUTime, PeakThreads, VirtualSize, PeakWS, Attributes,
		PagedPool, NonPagedPool, KernelTime, UserTime,
		PeakPagedPool, PeakNonPagedPool, Integrity, PEB, Protection, 
		Platform, Description, Company, JobId, MemoryPriority, IoPriority, Virtualization,
		ReadOperationsCount, WriteOperationsCount, OtherOperationsCount,
		ReadOperationsBytes, WriteOperationsBytes, OtherOperationsBytes,
		GdiObjects, PeakGdiObjects, UserObjects, PeakUserObjects,
	};

	struct ColumnInfo {
		PCSTR Header;
		std::function<void(std::shared_ptr<ProcessInfoEx>& pi)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
	};

public:
	ProcessesView();
	void BuildWindow();
	void ShowLowerPane(bool show);
	bool Refresh(bool now = false);

private:
	void DoSort(int col, bool asc);
	bool KillProcess(uint32_t id);
	void TryKillProcess(WinLL::ProcessInfo& pi);

	void BuildTable();
	void BuildViewMenu();
	void BuildProcessMenu(ProcessInfoEx& pi);
	void BuildToolBar();
	void BuildLowerPane();

	bool BuildPriorityClassMenu(WinLL::ProcessInfo& pi);
	bool GotoFileLocation(WinLL::ProcessInfo const& pi);

	//std::shared_ptr<ProcessProperties> GetProcessProperties(WinSys::ProcessInfo* pi);
	//std::shared_ptr<ProcessProperties> GetOrAddProcessProperties(const std::shared_ptr<WinSys::ProcessInfo>& pi);

	static std::string ProcessAttributesToString(ProcessAttributes attributes);

private:
	WinLL::ProcessManager<ProcessInfoEx, WinLL::ThreadInfo> m_ProcMgr;
	std::vector<uint32_t> m_PidsToKill;
	ThreadsView m_ThreadsView;
	ModulesView m_ModulesView;
	char m_FilterText[24]{};
	SortedFilteredVector<std::shared_ptr<ProcessInfoEx>> m_Processes;
	std::shared_ptr<ProcessInfoEx> m_SelectedProcess;
	SimpleMessageBox m_KillDlg;
	int m_SelectedIndex{ -1 };
	bool m_ShowLowerPane : 1{ false }, m_WasRunning : 1 { false };
};

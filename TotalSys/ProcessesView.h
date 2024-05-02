#pragma once

#include "ProcessInfoEx.h"
#include <SimpleMessageBox.h>
#include "ThreadsView.h"
#include "ViewBase.h"
#include "SortedFilteredVector.h"
#include "ModulesView.h"
#include "HandlesView.h"

struct ImGuiTableSortSpecsColumn;

class ProcessesView : public ViewBase {
public:
	ProcessesView();
	void BuildWindow();
	void ShowLowerPane(bool show);
	bool ToggleLowerPane();
	bool Refresh(bool now = false);

private:
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

	static std::string ProcessAttributesToString(ProcessAttributes attributes);

private:
	DefaultProcessManager m_ProcMgr;
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	std::vector<uint32_t> m_PidsToKill;
	ThreadsView m_ThreadsView;
	ModulesView m_ModulesView;
	HandlesView m_HandlesView;
	char m_FilterText[24]{};
	SortedFilteredVector<std::shared_ptr<ProcessInfoEx>> m_Processes;
	std::shared_ptr<ProcessInfoEx> m_SelectedProcess;
	SimpleMessageBox m_KillDlg;
	int m_SelectedIndex{ -1 };
	bool m_ShowLowerPane : 1{ false }, m_WasRunning : 1 { false }, m_FilterChanged : 1 { false };
	bool m_DoSize: 1{ false };
	bool m_ThreadsActive : 1 { false }, m_ModulesActive : 1 { false }, m_HandlesActive : 1 { false };
};

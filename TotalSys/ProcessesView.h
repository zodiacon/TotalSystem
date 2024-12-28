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
	void Build() noexcept override;
	void ShowLowerPane(bool show) noexcept;
	bool ToggleLowerPane() noexcept;
	bool Refresh(bool now = false) noexcept;
	uint32_t GetProcessesCount() const;

private:
	enum class Column {
		ProcessName, Pid, UserName, Session, CPU, ParentPid, CreateTime, Commit, BasePriority, PriorityClass, Threads,
		Handles, WorkingSet, ExePath, CPUTime, PeakThreads, VirtualSize, PeakWS, Attributes,
		PagedPool, NonPagedPool, KernelTime, UserTime,
		PeakPagedPool, PeakNonPagedPool, Integrity, PEB, Protection,
		Platform, Description, Company, JobId, MemoryPriority, IoPriority, Virtualization,
		ReadOperationsCount, WriteOperationsCount, OtherOperationsCount,
		ReadOperationsBytes, WriteOperationsBytes, OtherOperationsBytes, CommandLine, CycleCount,
		GdiObjects, PeakGdiObjects, UserObjects, PeakUserObjects, 
		ColumnCount
	};

	struct ColumnInfo {
		Column Type;
		PCSTR Header;
		std::function<void(std::shared_ptr<ProcessInfoEx>& pi)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
		ImGuiTableColumnFlags StateFlags{ 0 };
	};

	std::string GetColumnText(Column col, ProcessInfoEx* p) const;

	void DoSort(int col, bool asc) noexcept;
	bool KillProcess(uint32_t id) noexcept;
	void TryKillProcess(WinLL::ProcessInfo& pi) noexcept;

	void BuildTable() noexcept;
	void BuildViewMenu() noexcept;
	void BuildProcessMenu(ProcessInfoEx& pi) noexcept;
	void BuildToolBar() noexcept;
	void BuildLowerPane() noexcept;

	bool BuildPriorityClassMenu(WinLL::ProcessInfo& pi);
	bool GotoFileLocation(ProcessInfoEx const& pi);

	static std::string ProcessAttributesToString(ProcessAttributes attributes);

private:
	void InitColumns();
	void BuildPerfGraphs(ProcessInfoEx const* pi);

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
	int m_CurrentIndex{ -1 };
	std::vector<ColumnInfo> m_Columns;
	SimpleMessageBox m_MsgBox;
	bool m_WasRunning : 1 { false }, m_FilterChanged : 1 { false };
	bool m_DoSize : 1{ false }, m_UpdateNow : 1 { false }, m_FilterFocused : 1 { false };
	bool m_ThreadsActive : 1 { false }, m_ModulesActive : 1 { false }, m_HandlesActive : 1 { false };
};

#pragma once

#include "ProcessInfoEx.h"
#include <WinLowLevel.h>

//#include "ProcessProperties.h"

struct ImGuiTableSortSpecsColumn;
class TabManager;

class ProcessesView {
public:
	ProcessesView();
	void BuildWindow();

private:
	void DoSort(int col, bool asc);
	void DoUpdate();
	bool KillProcess(uint32_t id);
	bool TryKillProcess(WinLL::ProcessInfo& pi, bool& success);

	void BuildTable();
	void BuildViewMenu();
	void BuildProcessMenu();
	void BuildToolBar();

	bool BuildPriorityClassMenu(WinLL::ProcessInfo& pi);
	bool GotoFileLocation(WinLL::ProcessInfo const& pi);
	void TogglePause();
	//void BuildPropertiesWindow(ProcessProperties* props);

	//std::shared_ptr<ProcessProperties> GetProcessProperties(WinSys::ProcessInfo* pi);
	//std::shared_ptr<ProcessProperties> GetOrAddProcessProperties(const std::shared_ptr<WinSys::ProcessInfo>& pi);

	static std::string ProcessAttributesToString(ProcessAttributes attributes);

private:
	wil::com_ptr<ID3D11ShaderResourceView> m_spImage;
	DWORD64 m_Tick = 0;
	char m_FilterText[24]{};
	std::vector<std::shared_ptr<ProcessInfoEx>> m_Processes;
	const ImGuiTableColumnSortSpecs* m_Specs = nullptr;
	std::shared_ptr<ProcessInfoEx> m_SelectedProcess;
	int m_UpdateInterval{ 1000 }, m_OldInterval{ 0 };
	bool m_ModalOpen : 1 = false, m_KillFailed : 1 = false;
};

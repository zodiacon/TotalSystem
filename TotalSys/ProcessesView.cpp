#include "pch.h"
#include "ProcessesView.h"
#include "SortHelper.h"
#include "resource.h"
#include "Globals.h"
#include <ProcessManager.h>
#include <WinLowLevel.h>
#include <StandardColors.h>
#include <ImGuiExt.h>
#include "FormatHelper.h"
#include <ShellApi.h>
#include "TotalSysSettings.h"
#include "ProcessHelper.h"
#include "MainWindow.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace ImGui;
using namespace std;
using namespace WinLL;

ProcessesView::ProcessesView() : m_ThreadsView(&m_ProcMgr) {
	InitColumns();
	Open(true);
}

void ProcessesView::InitColumns() {
	static const ColumnInfo columns[]{
	{ "Name", [this](auto& p) {
		Image(p->Icon(), ImVec2(16, 16)); SameLine();
		PushFont(Globals::VarFont());
		if (Selectable(format("{}##{} {}", GetColumnText(Column::ProcessName, p.get()), p->Id, p->ParentId).c_str(),
			m_SelectedProcess == p, ImGuiSelectableFlags_SpanAllColumns)) {
			m_SelectedProcess = p;
			m_UpdateNow = true;
		}
		PopFont();

		auto item = format("{} {}", p->Id, p->ParentId);
		if (BeginPopupContextItem(item.c_str())) {
			if (IsRunning()) {
				TogglePause();
				m_ThreadsView.TogglePause();
				m_WasRunning = true;
			}
			m_SelectedProcess = p;
			BuildProcessMenu(*p);
			EndPopup();
		}

		if (m_SelectedProcess) {
			auto item = format("{} {}", m_SelectedProcess->Id, m_SelectedProcess->ParentId);
			if (!IsPopupOpen(item.c_str())) {
				if (m_WasRunning) {
					TogglePause();
					m_ThreadsView.TogglePause();
					m_WasRunning = false;
				}
			}
		}
		}, ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder },
	{ "PID", [&](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%9s", GetColumnText(Column::Pid, p.get()).c_str());
		PopFont();
		}, 0, 120.0f },
	{ "User Name", [&](auto p) {
		PushFont(Globals::VarFont());
		auto username = GetColumnText(Column::UserName, p.get());
		if (username.empty())
			TextColored(StandardColors::LightGray, "<access denied>");
		else
			TextUnformatted(username.c_str());
		PopFont();
		}, 0, 110.0f },
	{ "Session", [](auto p) {
		PushFont(Globals::MonoFont());
		Text("%4u", p->SessionId);
		PopFont();
		}, ImGuiTableColumnFlags_NoResize, 50.0f },
	{ "CPU %", [&](auto p) {
		if (p->CPU > 0 && !p->IsTerminated()) {
			auto orgBackColor = GetStyle().Colors[ImGuiCol_TableRowBg];

			PushFont(Globals::MonoFont());
			auto value = p->CPU / 10000.0f;
			auto str = format("{:7.2f}  ", value);
			ImVec4 color;
			auto customColors = p->Id && value > 1.0f;
			if (customColors) {
				color = ProcessHelper::GetColorByCPU(value).Value;
			}
			else {
				color = orgBackColor;
			}
			if (customColors) {
				TableSetBgColor(ImGuiTableBgTarget_CellBg, ColorConvertFloat4ToU32(color));
				TextColored(ImVec4(1, 1, 1, 1), str.c_str());
			}
			else {
				TextUnformatted(str.c_str());
			}
			PopFont();
		}
	}, 0, 70 },
	{ "Parent", [&](auto p) {
		if (p->ParentId > 0) {
			PushFont(Globals::MonoFont());
			Text("%7s", (Globals::Settings().HexIds() ? format("0x{:X}", p->ParentId) : format("{}", p->ParentId)).c_str());
			PopFont();
			auto parent = m_ProcMgr.GetProcessById(p->ParentId);
			if (parent && parent->CreateTime < p->CreateTime) {
				SameLine();
				PushFont(Globals::VarFont());
				Text("(%ws)", parent->GetImageName().c_str());
				PopFont();
			}
		}
	}, 0, 140.0f },
	{ "Create Time", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text(FormatHelper::FormatDateTime(p->CreateTime).c_str());
		PopFont();
		},ImGuiTableColumnFlags_NoResize },
	{ "Private Bytes", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%12ws K", FormatHelper::FormatNumber(p->PrivatePageCount >> 10).c_str());
		PopFont();
		}, },
	{ "Pri", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%4d", p->BasePriority);
		PopFont();
		}, },
	{ "Pri Class", [](auto& p) {
		PushFont(Globals::VarFont());
		TextUnformatted(FormatHelper::PriorityClassToString(p->GetPriorityClass()));
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Threads", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%6ws", FormatHelper::FormatNumber(p->ThreadCount).c_str());
		PopFont();
		}, },
	{ "Handles", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%8ws", FormatHelper::FormatNumber(p->HandleCount).c_str());
		PopFont();
		}, },
	{ "Working Set", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%12ws K", FormatHelper::FormatNumber(p->WorkingSetSize >> 10).c_str());
		PopFont();
		}, },
	{ "Executable Path", [](auto& p) {
		PushFont(Globals::VarFont());
		Text("%ws", p->GetExecutablePath().c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 150.0f },
	{ "CPU Time", [](auto& p) {
		PushFont(Globals::MonoFont());
		TextUnformatted(FormatHelper::FormatTimeSpan(p->UserTime + p->KernelTime).c_str());
		PopFont();
		}, },
	{ "Peak Thr", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%7ws", FormatHelper::FormatNumber(p->PeakThreads).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Virtual Size", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%14ws K", FormatHelper::FormatNumber(p->VirtualSize >> 10).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Peak WS", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%12ws K", FormatHelper::FormatNumber(p->PeakWorkingSetSize >> 10).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Attributes", [this](auto& p) {
		PushFont(Globals::MonoFont());
		TextUnformatted(ProcessAttributesToString(p->Attributes(m_ProcMgr)).c_str());
		PopFont();
		}, },
	{ "Paged Pool", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%9ws K", FormatHelper::FormatNumber(p->PagedPoolUsage >> 10).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "NP Pool", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%9ws K", FormatHelper::FormatNumber(p->NonPagedPoolUsage >> 10).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "kernel Time", [](auto& p) {
		PushFont(Globals::MonoFont());
		TextUnformatted(FormatHelper::FormatTimeSpan(p->KernelTime).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "User Time", [](auto& p) {
		PushFont(Globals::MonoFont());
		TextUnformatted(FormatHelper::FormatTimeSpan(p->UserTime).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Peak Paged", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%9ws K", FormatHelper::FormatNumber(p->PeakPagedPoolUsage >> 10).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Peak Non-Paged", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%9ws K", FormatHelper::FormatNumber(p->PeakNonPagedPoolUsage >> 10).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Integrity", [](auto& p) {
		PushFont(Globals::MonoFont());
		TextUnformatted(FormatHelper::IntegrityToString(p->GetIntegrityLevel()));
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "PEB", [](auto& p) {
		auto peb = p->GetPeb();
		if (peb) {
			PushFont(Globals::MonoFont());
			Text("0x%p", peb);
			PopFont();
		}
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Protection", [](auto& p) {
		auto protection = p->GetProtection();
		PushFont(Globals::VarFont());
		TextUnformatted(FormatHelper::ProtectionToString(protection).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },
	{ "Platform", [](auto& p) {
		PushFont(Globals::VarFont());
		Text("%d-Bit", p->GetBitness());
		PopFont();
		}, 0 },
	{ "Description", [](auto& p) {
		PushFont(Globals::VarFont());
		Text("%ws", p->GetDescription().c_str());
		PopFont();
		}, 0, 150, },
	{ "Company Name", [](auto& p) {
		PushFont(Globals::VarFont());
		Text("%ws", p->GetCompanyName().c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 150, },
	{ "Job ID", [](auto& p) {
		if (p->JobObjectId) {
			PushFont(Globals::MonoFont());
			Text("%4u", p->JobObjectId);
			PopFont();
		}
		}, ImGuiTableColumnFlags_DefaultHide, 60, },
	{ "Memory Pri", [](auto& p) {
		auto priority = p->GetMemoryPriority();
		if (priority >= 0) {
			PushFont(Globals::MonoFont());
			Text("%4u", priority);
			PopFont();
		}
		}, ImGuiTableColumnFlags_DefaultHide, 60, },
	{ "I/O Pri", [](auto& p) {
		PushFont(Globals::VarFont());
		TextUnformatted(FormatHelper::IoPriorityToString(p->GetIoPriority()));
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 70, },
	{ "Virtualization", [](auto& p) {
		PushFont(Globals::VarFont());
		TextUnformatted(FormatHelper::VirtualizationStateToString(p->GetVirtualizationState()));
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 70, },
	{ "Read Op Count", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%12ws", FormatHelper::FormatNumber(p->ReadOperationCount).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 100, },
	{ "Write Op Count", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%12ws", FormatHelper::FormatNumber(p->WriteOperationCount).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 100, },
	{ "Other Op Count", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%12ws", FormatHelper::FormatNumber(p->OtherOperationCount).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 100, },
	{ "Read Bytes", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%17ws", FormatHelper::FormatNumber(p->ReadTransferCount).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 130, },
	{ "Write Bytes", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%17ws", FormatHelper::FormatNumber(p->WriteTransferCount).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 130, },
	{ "Other Bytes", [](auto& p) {
		PushFont(Globals::MonoFont());
		Text("%17ws", FormatHelper::FormatNumber(p->OtherTransferCount).c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 130, },
	{ "Command line", [](auto& p) {
		PushFont(Globals::VarFont());
		Text("%ws", p->GetCommandLine().c_str());
		PopFont();
		}, ImGuiTableColumnFlags_DefaultHide, 150, },

	};

	m_Columns.insert(m_Columns.end(), begin(columns), end(columns));
}

void ProcessesView::Build() noexcept {
	if (IsOpen()) {
		PushFont(Globals::VarFont());
		auto view = GetMainViewport();
		SetNextWindowSize(view->WorkSize, ImGuiCond_FirstUseEver);
		if (Begin("Processes", GetOpenAddress(), ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar)) {
			if (Globals::RootWindow().SaveSelected()) {
				// handle Save...
			}
			if (BeginMenuBar()) {
				if (m_SelectedProcess && BeginMenu("Process")) {
					BuildProcessMenu(*m_SelectedProcess);
					ImGui::EndMenu();
				}
				BuildViewMenu();
				EndMenuBar();
			}
			BuildToolBar();
			BuildTable();
		}
		ImGui::End();
		PopFont();
	}
}


void ProcessesView::ShowLowerPane(bool show) noexcept {
	Globals::Settings().ShowLowerPane(show);
	m_DoSize = true;
}

bool ProcessesView::ToggleLowerPane() noexcept {
	Globals::Settings().ShowLowerPane(!Globals::Settings().ShowLowerPane());
	m_DoSize = true;
	return Globals::Settings().ShowLowerPane();
}

bool ProcessesView::Refresh(bool now) noexcept {
	if (NeedUpdate() || now || m_UpdateNow) {
		m_UpdateNow = false;
		auto& pm = m_ProcMgr;
		auto empty = m_Processes.empty();
		pm.Update(true);
		if (Globals::Settings().ShowLowerPane() && m_SelectedProcess) {
			if (m_ThreadsActive)
				m_ThreadsView.RefreshProcess(m_SelectedProcess, true);
			else if (m_ModulesActive)
				m_ModulesView.Refresh(m_SelectedProcess->Id, true);
			else if (m_HandlesActive)
				m_HandlesView.Refresh(m_SelectedProcess->Id, true);
		}

		std::string filter = m_FilterText;

		auto count = static_cast<int>(m_Processes.size());
		for (int i = 0; i < count; i++) {
			auto& p = m_Processes[i];
			if (p->Update()) {
				// process terminated
				if (p == m_SelectedProcess)
					m_SelectedProcess.reset();
				m_Processes.Remove(i);
				i--;
				count--;
				continue;
			}
		}

		if (empty) {
			m_Processes = pm.GetProcesses();
			if (m_Specs)
				m_Specs->SpecsDirty = true;
		}
		else {
			for (auto& pi : pm.GetNewProcesses()) {
				m_Processes.push_back(pi);
				pi->New(Globals::Settings().NewObjectsTime() * 1000);
			}

			for (auto& pi : pm.GetTerminatedProcesses()) {
				pi->Term(Globals::Settings().OldObjectsTime() * 1000);
			}
		}

		if (m_FilterChanged) {
			if (!filter.empty()) {
				_strlwr_s(filter.data(), filter.length() + 1);
				m_Processes.Filter([&](auto& p, auto) {
					std::wstring name(p->GetImageName());
					if (!name.empty()) {
						_wcslwr_s(name.data(), name.length() + 1);
						wstring wfilter(filter.begin(), filter.end());
						if (name.find(wfilter) != wstring::npos) {
							return true;
						}
					}
					if (to_string(p->Id).find(filter) != string::npos) {
						return true;
					}
					if (format("{:x}", p->Id).find(filter) != string::npos) {
						return true;
					}
					return false;
					});
			}
			else {
				m_Processes.Filter(nullptr);
			}
			m_FilterChanged = false;
		}

		if (m_Specs) {
			m_Specs->SpecsDirty = true;
		}

		UpdateTick();

		return true;
	}
	return false;
}

std::string ProcessesView::GetColumnText(Column col, ProcessInfoEx* p) const {
	static char output[4096];
	switch (col) {
		case Column::ProcessName: sprintf_s(output, "%ws", p->GetImageName().c_str()); return output;
		case Column::Pid: return Globals::Settings().HexIds() ? format("0x{:X}", p->Id) : format("{}", p->Id);
		case Column::UserName: sprintf_s(output, "%ws", p->GetUserName(true).c_str()); return output;
		case Column::Session: return format("{}", p->SessionId);
		case Column::CPU: return format("{:7.2f}  ", p->CPU / 10000.0f);
		case Column::ParentPid: 
			auto text = Globals::Settings().HexIds() ? format("0x{:X}", p->ParentId) : format("{}", p->ParentId);
			if (p->ParentId) {
				auto parent = m_ProcMgr.GetProcessById(p->ParentId);
				if (parent && parent->CreateTime < p->CreateTime) {
					sprintf_s(output, "%s (%ws)", text.c_str(), parent->GetImageName().c_str());
					return output;
				}
			}
			return text;
	}
	return "";
}

void ProcessesView::DoSort(int col, bool asc) noexcept {
	m_Processes.Sort([&](const auto& p1, const auto& p2) {
		switch (static_cast<Column>(col)) {
			case Column::ProcessName: return SortHelper::Sort(p1->GetImageName(), p2->GetImageName(), asc);
			case Column::Pid: return SortHelper::Sort(p1->Id, p2->Id, asc);
			case Column::UserName: return SortHelper::Sort(p1->GetUserName(true), p2->GetUserName(true), asc);
			case Column::Session: return SortHelper::Sort(p1->SessionId, p2->SessionId, asc);
			case Column::CPU: return SortHelper::Sort(p1->CPU, p2->CPU, asc);
			case Column::ParentPid: return SortHelper::Sort(p1->ParentId, p2->ParentId, asc);
			case Column::CreateTime: return SortHelper::Sort(p1->CreateTime, p2->CreateTime, asc);
			case Column::Commit: return SortHelper::Sort(p1->PrivatePageCount, p2->PrivatePageCount, asc);
			case Column::BasePriority: return SortHelper::Sort(p1->BasePriority, p2->BasePriority, asc);
			case Column::Threads: return SortHelper::Sort(p1->ThreadCount, p2->ThreadCount, asc);
			case Column::Handles: return SortHelper::Sort(p1->HandleCount, p2->HandleCount, asc);
			case Column::WorkingSet: return SortHelper::Sort(p1->WorkingSetSize, p2->WorkingSetSize, asc);
			case Column::ExePath: return SortHelper::Sort(p1->GetExecutablePath(), p2->GetExecutablePath(), asc);
			case Column::CPUTime: return SortHelper::Sort(p1->KernelTime + p1->UserTime, p2->KernelTime + p2->UserTime, asc);
			case Column::PeakThreads: return SortHelper::Sort(p1->PeakThreads, p2->PeakThreads, asc);
			case Column::VirtualSize: return SortHelper::Sort(p1->VirtualSize, p2->VirtualSize, asc);
			case Column::PeakWS: return SortHelper::Sort(p1->PeakWorkingSetSize, p2->PeakWorkingSetSize, asc);
			case Column::Attributes: return SortHelper::Sort(p1->Attributes(m_ProcMgr), p2->Attributes(m_ProcMgr), asc);
			case Column::PagedPool: return SortHelper::Sort(p1->PagedPoolUsage, p2->PagedPoolUsage, asc);
			case Column::NonPagedPool: return SortHelper::Sort(p1->NonPagedPoolUsage, p2->NonPagedPoolUsage, asc);
			case Column::KernelTime: return SortHelper::Sort(p1->KernelTime, p2->KernelTime, asc);
			case Column::UserTime: return SortHelper::Sort(p1->UserTime, p2->UserTime, asc);
			case Column::PeakPagedPool: return SortHelper::Sort(p1->PeakPagedPoolUsage, p2->PeakPagedPoolUsage, asc);
			case Column::PeakNonPagedPool: return SortHelper::Sort(p1->PeakNonPagedPoolUsage, p2->PeakNonPagedPoolUsage, asc);
			case Column::Integrity: return SortHelper::Sort(p1->GetIntegrityLevel(), p2->GetIntegrityLevel(), asc);
			case Column::PEB: return SortHelper::Sort(p1->GetPeb(), p2->GetPeb(), asc);
			case Column::Protection: return SortHelper::Sort(p1->GetProtection().Level, p2->GetProtection().Level, asc);
			case Column::Company: return SortHelper::Sort(p1->GetCompanyName(), p2->GetCompanyName(), asc);
			case Column::Description: return SortHelper::Sort(p1->GetDescription(), p2->GetDescription(), asc);
			case Column::Platform: return SortHelper::Sort(p1->GetBitness(), p2->GetBitness(), asc);
			case Column::JobId: return SortHelper::Sort(p1->JobObjectId, p2->JobObjectId, asc);
			case Column::MemoryPriority: return SortHelper::Sort(p1->GetMemoryPriority(), p2->GetMemoryPriority(), asc);
			case Column::IoPriority: return SortHelper::Sort(p1->GetIoPriority(), p2->GetIoPriority(), asc);
			case Column::Virtualization: return SortHelper::Sort(p1->GetVirtualizationState(), p2->GetVirtualizationState(), asc);
			case Column::ReadOperationsBytes: return SortHelper::Sort(p1->ReadTransferCount, p2->ReadTransferCount, asc);
			case Column::WriteOperationsBytes: return SortHelper::Sort(p1->WriteTransferCount, p2->WriteTransferCount, asc);
			case Column::OtherOperationsBytes: return SortHelper::Sort(p1->OtherTransferCount, p2->OtherTransferCount, asc);
			case Column::ReadOperationsCount: return SortHelper::Sort(p1->ReadOperationCount, p2->ReadOperationCount, asc);
			case Column::WriteOperationsCount: return SortHelper::Sort(p1->WriteOperationCount, p2->WriteOperationCount, asc);
			case Column::OtherOperationsCount: return SortHelper::Sort(p1->OtherOperationCount, p2->OtherOperationCount, asc);
		}
		return false;
		});
}


bool ProcessesView::KillProcess(uint32_t id) noexcept {
	Process process;
	if (!process.Open(id, ProcessAccessMask::Terminate))
		return false;

	return process.Terminate();
}

void ProcessesView::TryKillProcess(ProcessInfo& pi) noexcept {
	if (m_KillDlg.IsEmpty()) {
		m_PidsToKill.clear();
		char text[128];
		sprintf_s(text, "Kill process %u (%ws)?", pi.Id, pi.GetImageName().c_str());

		m_KillDlg.Init("Kill Process", text, MessageBoxButtons::OkCancel);
	}
}

void ProcessesView::BuildTable() noexcept {
	static char output[8192];

	auto size = GetWindowSize();
	if (!Globals::Settings().ShowLowerPane())
		m_DoSize = false;

	if (BeginChild("upper", ImVec2(), Globals::Settings().ShowLowerPane() ? ImGuiChildFlags_ResizeY : 0, ImGuiWindowFlags_NoScrollbar)) {
		if (m_DoSize) {
			SetWindowSize(ImVec2(0, size.y / 2), ImGuiCond_Always);
			m_DoSize = false;
		}
		if (IsKeyPressed(ImGuiKey_Space)) {
			TogglePause();
			m_ThreadsView.TogglePause();
		}
		if (BeginTable("ProcessesTable", (int)m_Columns.size(),
			ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable |
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterV)) {
			TableSetupScrollFreeze(2, 1);

			int c = 0;
			PushFont(Globals::VarFont());
			for (auto& ci : m_Columns)
				TableSetupColumn(ci.Header, ci.Flags, ci.Width, c++);
			TableHeadersRow();

			auto update = Refresh();
			auto specs = m_Specs = TableGetSortSpecs();
			if (specs->SpecsDirty) {
				specs->SpecsDirty = false;
				DoSort(specs->Specs->ColumnIndex, specs->Specs->SortDirection == ImGuiSortDirection_Ascending);
			}

			PopFont();

			auto result = m_KillDlg.ShowModal();
			if (result == MessageBoxResult::OK) {
				if (m_PidsToKill.empty()) {
					auto success = KillProcess(m_SelectedProcess->Id);
					if (success)
						m_SelectedProcess.reset();
				}
				else {
					for (auto& pid : m_PidsToKill) {
						Process p;
						if (p.Open(pid, ProcessAccessMask::Terminate))
							p.Terminate();
					}

				}
			}

			auto count = static_cast<int>(m_Processes.size());
			ImGuiListClipper clipper;
			clipper.Begin(count);
			clipper.IncludeItemByIndex(m_SelectedIndex);
			bool pressed = false;
			while (clipper.Step()) {
				for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
					auto& p = m_Processes[j];
					TableNextRow();

					auto popCount = 0;
					auto colors = p->Colors(m_ProcMgr);
					if (colors.first.x >= 0) {
						TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(colors.first));
						PushStyleColor(ImGuiCol_Text, colors.second);
						popCount = 1;
					}

					if (!pressed && j > m_SelectedIndex && IsKeyChordPressed((ImGuiKey)(ImGuiKey_A + toupper(p->GetImageName()[0]) - 'A'))) {
						m_SelectedIndex = j;
						m_SelectedProcess = p;
						m_UpdateNow = true;
						SetKeyboardFocusHere();
						pressed = true;
						OutputDebugString(format(L"Selected index: {}, process: {}\n", m_SelectedIndex, m_SelectedProcess->GetImageName()).c_str());
					}

					int count = (int)m_Columns.size();
					for (c = 0; c < count; c++) {
						if (TableSetColumnIndex(c)) {
							m_Columns[c].StateFlags = TableGetColumnFlags(c);
							m_Columns[c].Callback(p);
							if (!pressed && c == 0 && IsItemFocused()) {
								m_SelectedProcess = p;
								m_SelectedIndex = j;
							}
						}
					}

					PopStyleColor(popCount);

				}
			}

			//for (size_t i = m_SelectedIndex + 1; i < m_Processes.size(); i++) {
			//	auto& p = m_Processes[i];
			//	if (IsKeyChordPressed((ImGuiKey)(ImGuiKey_A + toupper(p->GetImageName()[0]) - 'A'))) {
			//		m_SelectedProcess = p;
			//		m_SelectedIndex = (int)i;
			//		break;
			//	}
			//}

			EndTable();


			if (m_SelectedProcess && IsKeyChordPressed(ImGuiKey_Delete)) {
				TryKillProcess(*m_SelectedProcess);
			}

		}

	}
	EndChild();

	BuildLowerPane();
}

void ProcessesView::BuildViewMenu() noexcept {
	if (IsKeyChordPressed(ImGuiKey_L | ImGuiMod_Ctrl)) {
		ToggleLowerPane();
	}

	PushFont(Globals::VarFont());
	if (BeginMenu("View")) {
		if (MenuItem("Show Lower Pane", "Ctrl+L", Globals::Settings().ShowLowerPane())) {
			ToggleLowerPane();
		}
		BuildUpdateIntervalMenu();
		Separator();
		if (MenuItem("Refresh", "F5")) {
		}
		ImGui::EndMenu();
	}
	PopFont();
}

void ProcessesView::BuildProcessMenu(ProcessInfoEx& pi) noexcept {
	PushFont(Globals::VarFont());
	if (pi.Id > 4) {
		if (MenuItem("Kill", "Delete")) {
			TryKillProcess(pi);
		}
		auto len = pi.GetImageName().size() + 32;
		auto name = make_unique<char[]>(len);
		sprintf_s(name.get(), len, "Kill all %ws processes...", pi.GetImageName().c_str());
		if (MenuItem(name.get())) {
			if (m_KillDlg.IsEmpty()) {
				auto processes = ProcessHelper::GetProcessIdsByName(m_Processes.GetAllItems(), pi.GetImageName());
				m_KillDlg.Init("Kill Processes", FormatHelper::Format("Kill %u processes named %ws?",
					(uint32_t)processes.size(), pi.GetImageName().c_str()), MessageBoxButtons::OkCancel);
				m_PidsToKill = move(processes);
			}
		}
		Separator();
		if (BuildPriorityClassMenu(pi))
			Separator();

		if (MenuItem(m_SelectedProcess->IsSuspended() ? "Resume" : "Suspend")) {
			pi.SuspendResume();
		}
		Separator();
	}
	if (MenuItem("Open file location")) {
		GotoFileLocation(pi);
	}
	//Separator();
	//if (MenuItem("Properties...")) {
	//	GetOrAddProcessProperties(p);
	//}
	PopFont();
}

void ProcessesView::BuildToolBar() noexcept {
	PushFont(Globals::VarFont());
	if (ImageButton("LowerPane", Globals::ImageManager().GetImage(Globals::Settings().ShowLowerPane() ? IDI_WINDOW : IDI_SPLIT), ImVec2(16, 16))) {
		ToggleLowerPane();
	}
	if (IsItemHovered())
		SetTooltip(((Globals::Settings().ShowLowerPane() ? "Hide" : "Show") + string(" Lower Pane")).c_str());
	SameLine();
	if (ImageButton("Pause", Globals::ImageManager().GetImage(IsRunning() ? IDI_PAUSE : IDI_RUNNING), ImVec2(16, 16))) {
		TogglePause();
		m_ThreadsView.TogglePause();
	}
	if (IsItemHovered())
		SetTooltip(IsRunning() ? "Pause" : "Resume");

	SameLine();
	SetNextItemWidth(120);
	if (IsKeyChordPressed(ImGuiKey_F | ImGuiMod_Ctrl))
		SetKeyboardFocusHere();

	if (InputTextWithHint("##Filter", "Filter (Ctrl+F)", m_FilterText, _countof(m_FilterText),
		ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EscapeClearsAll)) {
		m_FilterChanged = true;
	}
	if (IsItemHovered())
		SetTooltip("Filter by name/ID");

	SameLine(0, 20);
	if (ButtonEnabled("Kill", m_SelectedProcess != nullptr, ImVec2(40, 0))) {
		TryKillProcess(*m_SelectedProcess);
	}
	if (IsItemHovered())
		SetTooltip("Terminate Process");
	SameLine();

	if (BuildUpdateIntervalToolBar())
		m_ThreadsView.SetUpdateInterval(GetUpdateInterval());

	SameLine();
	if (ButtonEnabled("Copy", m_SelectedProcess != nullptr)) {
		// copy entire row

	}
	SameLine();

	bool open = Button("Colors", ImVec2(60, 0));
	if (open)
		OpenPopup("colors");

	if (BeginPopup("colors", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
		auto& colors = Globals::Settings().GetProcessColors();

		for (auto& c : colors) {
			Checkbox(c.Name.c_str(), &c.Enabled);
			SameLine(150);
			ColorEdit4(format("Background##{}", c.Name).c_str(), (float*)&c.Color, ImGuiColorEditFlags_NoInputs);
			SameLine();
			if (Button(format("Reset##{}", c.Name).c_str()))
				c.Color = c.DefaultColor;

			SameLine();
			ColorEdit4(format("Text##{}", c.Name).c_str(), (float*)&c.TextColor, ImGuiColorEditFlags_NoInputs);
			SameLine();
			if (Button(format("Reset##Text{}", c.Name).c_str()))
				c.TextColor = c.DefaultTextColor;
		}

		EndPopup();
	}
	PopFont();
}

void ProcessesView::BuildLowerPane() noexcept {
	if (Globals::Settings().ShowLowerPane()) {
		if (BeginChild("lowerpane", ImVec2(), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar)) {
			if (m_SelectedProcess) {
				PushFont(Globals::VarFont());
				if (BeginTabBar("lowertabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_Reorderable)) {
					SameLine(500);
					Text("PID: %u (%ws)", m_SelectedProcess->Id, m_SelectedProcess->GetImageName().c_str()); SameLine();
				}
				if (m_ThreadsActive = BeginTabItem("Threads", nullptr, ImGuiTabItemFlags_None)) {
					m_ThreadsView.BuildToolBar();
					m_ThreadsView.BuildTable(m_SelectedProcess);
					EndTabItem();
				}
				if (m_ModulesActive = BeginTabItem("Modules", nullptr, ImGuiTabItemFlags_None)) {
					m_ModulesView.Track(m_SelectedProcess->Id);
					m_ModulesView.BuildTable();
					EndTabItem();
				}
				if (m_HandlesActive = BeginTabItem("Handles", nullptr, ImGuiTabItemFlags_None)) {
					m_HandlesView.Track(m_SelectedProcess->Id);
					m_HandlesView.BuildToolBar();
					m_HandlesView.BuildTable();
					EndTabItem();
				}
				if ((m_SelectedProcess->Attributes(m_ProcMgr) & ProcessAttributes::InJob) == ProcessAttributes::InJob) {
					if (BeginTabItem("Job", nullptr, ImGuiTabItemFlags_None)) {
						EndTabItem();
					}
				}
				if (BeginTabItem("Token")) {
					EndTabItem();
				}
				if (BeginTabItem("Memory", nullptr, ImGuiTabItemFlags_None)) {
					EndTabItem();
				}
				if (BeginTabItem("Environment", nullptr, ImGuiTabItemFlags_None)) {
					EndTabItem();
				}
				EndTabBar();
				PopFont();
			}
		}
		EndChild();
	}
}

bool ProcessesView::BuildPriorityClassMenu(ProcessInfo& pi) {
	auto pc = PriorityClass::Normal;
	Process process;
	auto enabled = process.Open(pi.Id, ProcessAccessMask::QueryLimitedInformation | ProcessAccessMask::SetInformation);
	if (!enabled)
		enabled = process.Open(pi.Id, ProcessAccessMask::QueryLimitedInformation);

	if (enabled) {
		pc = process.GetPriorityClass();

		if (BeginMenu("Priority")) {
			if (MenuItem("Idle (4)", nullptr, pc == PriorityClass::Idle, pc != PriorityClass::Idle))
				process.SetPriorityClass(WinLL::PriorityClass::Idle);
			if (MenuItem("Below Normal (6)", nullptr, pc == PriorityClass::BelowNormal, pc != PriorityClass::BelowNormal))
				process.SetPriorityClass(WinLL::PriorityClass::BelowNormal);
			if (MenuItem("Normal (8)", nullptr, pc == PriorityClass::Normal, pc != PriorityClass::Normal))
				process.SetPriorityClass(WinLL::PriorityClass::BelowNormal);
			if (MenuItem("Above Normal (10)", nullptr, pc == PriorityClass::AboveNormal, pc != PriorityClass::AboveNormal))
				process.SetPriorityClass(PriorityClass::AboveNormal);
			if (MenuItem("High (13)", nullptr, pc == PriorityClass::High, pc != PriorityClass::High))
				process.SetPriorityClass(PriorityClass::High);
			if (MenuItem("Real-time (24)", nullptr, pc == PriorityClass::Realtime, pc != PriorityClass::Realtime))
				process.SetPriorityClass(PriorityClass::Realtime);
			Separator();
			if (MenuItem("Background Mode")) {
				process.SetBackgroundMode(true);
			}
			if (MenuItem("Foreground Mode")) {
				process.SetBackgroundMode(false);
			}
			ImGui::EndMenu();
		}
	}
	return enabled;
}

bool ProcessesView::GotoFileLocation(ProcessInfo const& pi) {
	wstring path;
	if (pi.Id <= 4) {
		path = SystemInformation::GetSystemDir() + L"\\ntoskrnl.exe";
	}
	if (path.empty()) {
		Process process;
		if (process.Open(pi.Id, ProcessAccessMask::QueryLimitedInformation)) {
			auto path = process.GetFullImageName();
			auto bs = path.rfind(L'\\');
			if (bs == std::wstring::npos)
				return false;

			auto folder = path.substr(0, bs);
		}
	}
	if (path.empty())
		return false;

	return (INT_PTR)::ShellExecute(nullptr, L"open", L"explorer", (L"/select,\"" + path + L"\"").c_str(),
		nullptr, SW_SHOWDEFAULT) > 31;
}


string ProcessesView::ProcessAttributesToString(ProcessAttributes attributes) {
	string text;

	static const struct {
		ProcessAttributes Attribute;
		const char* Text;
	} attribs[] = {
		{ ProcessAttributes::Managed, "Managed" },
		{ ProcessAttributes::Immersive, "Immersive" },
		{ ProcessAttributes::Protected, "Protected" },
		{ ProcessAttributes::Secure, "Secure" },
		{ ProcessAttributes::Service, "Service" },
		{ ProcessAttributes::InJob, "Job" },
	};

	for (auto& item : attribs)
		if ((item.Attribute & attributes) == item.Attribute)
			text += std::string(item.Text) + ", ";
	if (!text.empty())
		text = text.substr(0, text.length() - 2);
	return text;
}



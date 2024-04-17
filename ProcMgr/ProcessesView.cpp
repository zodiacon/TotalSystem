#include "pch.h"
#include "ProcessesView.h"
#include "SortHelper.h"
#include "resource.h"
#include "Globals.h"
#include <ProcessManager.h>
#include <WinLowLevel.h>
#include <StandardColors.h>
#include <ImGuiExt.h>
#include <Shlwapi.h>
#include "FormatHelper.h"
#include "Resource.h"
#include <ShellApi.h>
#include "ProcMgrSettings.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace ImGui;
using namespace std;
using namespace WinLL;

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;

ProcessesView::ProcessesView() {
	UINT icons[]{ IDI_PAUSE };

	for (auto& icon : icons) {
		auto hIcon = (HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(icon), IMAGE_ICON, 16, 16, LR_COPYFROMRESOURCE | LR_CREATEDIBSECTION);
		m_Icons.insert({ icon, D3D11Image::FromIcon(hIcon) });
	}
}

void ProcessesView::BuildWindow() {
	if (m_Open) {
		SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
		SetNextWindowPos(ImVec2(10, 20), ImGuiCond_FirstUseEver);
		if (Begin("Processes", &m_Open, ImGuiWindowFlags_MenuBar)) {
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
	}


	//std::vector<WinSys::ProcessOrThreadKey> keys;
	//for (const auto& [key, p] : _processProperties) {
	//	if (p->WindowOpen)
	//		BuildPropertiesWindow(p.get());
	//	else
	//		keys.push_back(p->GetProcess()->Key);
	//}

	//for (auto& key : keys)
	//	_processProperties.erase(key);
}

bool ProcessesView::IsOpen() const {
	return m_Open;
}

void ProcessesView::Open(bool open) {
	m_Open = true;
}

void ProcessesView::ShowLowerPane(bool show) {
	m_ShowLowerPane = show;
}

void ProcessesView::BuildThreadTable(std::shared_ptr<ProcessInfoEx>& p) {

}

void ProcessesView::DoSort(int col, bool asc) {
	ranges::sort(m_Processes, [&](const auto& p1, const auto& p2) {
		switch (static_cast<Column>(col)) {
			case Column::ProcessName: return SortHelper::Sort(p1->GetImageName(), p2->GetImageName(), asc);
			case Column::Pid: return SortHelper::Sort(p1->Id, p2->Id, asc);
			case Column::UserName: return SortHelper::Sort(p1->UserName(), p2->UserName(), asc);
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
			case Column::Attributes: return SortHelper::Sort(p1->Attributes(), p2->Attributes(), asc);
			case Column::PagedPool: return SortHelper::Sort(p1->PagedPoolUsage, p2->PagedPoolUsage, asc);
			case Column::NonPagedPool: return SortHelper::Sort(p1->NonPagedPoolUsage, p2->NonPagedPoolUsage, asc);
			case Column::KernelTime: return SortHelper::Sort(p1->KernelTime, p2->KernelTime, asc);
			case Column::UserTime: return SortHelper::Sort(p1->UserTime, p2->UserTime, asc);
			case Column::PeakPagedPool: return SortHelper::Sort(p1->PeakPagedPoolUsage, p2->PeakPagedPoolUsage, asc);
			case Column::PeakNonPagedPool: return SortHelper::Sort(p1->PeakNonPagedPoolUsage, p2->PeakNonPagedPoolUsage, asc);
		}
		return false;
		});
}

void ProcessesView::DoUpdate() {
	auto& pm = Globals::ProcessManager();
	for (auto& pi : pm.GetNewProcesses()) {
		m_Processes.push_back(pi);
		pi->New(2000);
	}

	for (auto& pi : pm.GetTerminatedProcesses()) {
		pi->Term(2000);
	}
}

bool ProcessesView::KillProcess(uint32_t id) {
	Process process;
	if (!process.Open(id, ProcessAccessMask::Terminate))
		return false;

	return process.Terminate();
}

void ProcessesView::TryKillProcess(ProcessInfo& pi) {
	if (m_KillDlg.IsEmpty()) {
		const std::string name(pi.GetImageName().begin(), pi.GetImageName().end());
		auto text = format("Kill process {} ({})?", pi.Id, name);

		m_KillDlg.Init("Kill Process?", move(text), MessageBoxButtons::OkCancel);
	}
}

void ProcessesView::BuildTable() {
	auto orgBackColor = GetStyle().Colors[ImGuiCol_TableRowBg];
	auto& pm = Globals::ProcessManager();

	static const ColumnInfo columns[] = {
		{ "Name", [this](auto& p) {
			Image(p->Icon(), ImVec2(16, 16)); SameLine();
			const std::string name(p->GetImageName().begin(), p->GetImageName().end());
			auto str = format("{}##{} {}", name, p->Id, p->ParentId);
			Selectable(str.c_str(), m_SelectedProcess.get() == p.get(), ImGuiSelectableFlags_SpanAllColumns);

			if (IsItemClicked()) {
				m_SelectedProcess = p;
			}

			auto item = format("{} {}", p->Id, p->ParentId);
			if (BeginPopupContextItem(item.c_str())) {
				if (m_UpdateInterval) {
					TogglePause();
					m_Paused = true;
				}
				m_SelectedProcess = p;
				BuildProcessMenu(*p);
				EndPopup();
			}
			if (m_SelectedProcess) {
				item = format("{} {}", m_SelectedProcess->Id, m_SelectedProcess->ParentId);
				if (!IsPopupOpen(item.c_str())) {
					if (m_Paused) {
						TogglePause();
						m_Paused = false;
					}
				}
			}

			}, ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder },
		{ "PID", [](auto& p) {
			Text("%7u (0x%05X)", p->Id, p->Id);
			}, 0, 130.0f },
		{ "User Name", [](auto p) {
			auto& username = p->GetUserName(true);
			if (username.empty())
				TextColored(StandardColors::LightGray, "<access denied>");
			else
				Text("%ws", username.c_str());
			}, 0, 110.0f },
		{ "Session", [](auto p) {
			Text("%4u", p->SessionId);
			}, ImGuiTableColumnFlags_NoResize, 50.0f },
		{ "CPU %", [&](auto p) {
			if (p->CPU > 0 && !p->IsTerminated()) {
				auto value = p->CPU / 10000.0f;
				auto str = format("{:7.2f}  ", value);
				ImVec4 color;
				auto customColors = p->Id && value > 1.0f;
				if (customColors) {
					color = ImColor::HSV(.6f, value / 100 + .3f, .3f).Value;
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
			}
		} },
		{ "Parent", [&](auto p) {
			if (p->ParentId > 0) {
				Text("%6d", p->ParentId);
				auto parent = pm.GetProcessById(p->ParentId);
				if (parent && parent->CreateTime < p->CreateTime) {
					SameLine();
					Text("(%ws)", parent->GetImageName().c_str());
				}
			}
		}, 0, 140.0f },
		{ "Created", [](auto& p) {
			Text(FormatHelper::FormatDateTime(p->CreateTime).c_str());
			},ImGuiTableColumnFlags_NoResize },
		{ "Private Bytes", [](auto& p) {
			Text("%12ws K", FormatHelper::FormatNumber(p->PrivatePageCount >> 10).c_str());
			}, },
		{ "Pri", [](auto& p) {
			Text("%5d", p->BasePriority);
			}, },
		{ "Threads", [](auto& p) {
			Text("%6ws", FormatHelper::FormatNumber(p->ThreadCount).c_str());
			}, },
		{ "Handles", [](auto& p) {
			Text("%6ws", FormatHelper::FormatNumber(p->HandleCount).c_str());
			}, },
		{ "Working Set", [](auto& p) {
			Text("%12ws K", FormatHelper::FormatNumber(p->WorkingSetSize >> 10).c_str());
			}, },
		{ "Executable Path", [](auto& p) {
			Text("%ws", p->GetExecutablePath().c_str());
			}, 0, 150.0f },
		{ "CPU Time", [](auto& p) {
			TextUnformatted(FormatHelper::FormatTimeSpan(p->UserTime + p->KernelTime).c_str());
			}, },
		{ "Peak Thr", [](auto& p) {
			Text("%7ws", FormatHelper::FormatNumber(p->PeakThreads).c_str());
			}, },
		{ "Virtual Size", [](auto& p) {
			Text("%14ws K", FormatHelper::FormatNumber(p->VirtualSize >> 10).c_str());
			}, },
		{ "Peak WS", [](auto& p) {
			Text("%12ws K", FormatHelper::FormatNumber(p->PeakWorkingSetSize >> 10).c_str());
			}, },
		{ "Attributes", [](auto& p) {
			TextUnformatted(ProcessAttributesToString(p->Attributes()).c_str());
			}, },
		{ "Paged Pool", [](auto& p) {
			Text("%9ws K", FormatHelper::FormatNumber(p->PagedPoolUsage >> 10).c_str());
			}, },
		{ "NP Pool", [](auto& p) {
			Text("%9ws K", FormatHelper::FormatNumber(p->NonPagedPoolUsage >> 10).c_str());
			}, },
		{ "kernel Time", [](auto& p) {
			TextUnformatted(FormatHelper::FormatTimeSpan(p->KernelTime).c_str());
			}, },
		{ "User Time", [](auto& p) {
			TextUnformatted(FormatHelper::FormatTimeSpan(p->UserTime).c_str());
			}, },
		{ "Peak Paged", [](auto& p) {
			Text("%9ws K", FormatHelper::FormatNumber(p->PeakPagedPoolUsage >> 10).c_str());
			}, },
		{ "Peak Non-Paged", [](auto& p) {
			Text("%9ws K", FormatHelper::FormatNumber(p->PeakNonPagedPoolUsage >> 10).c_str());
			}, },
	};

	float y = 0;
	if (m_ShowLowerPane) {
		y = GetWindowSize().y / 2;
		BeginChild("upper", ImVec2(0, y), ImGuiChildFlags_ResizeY);
	}
	if (BeginTable("processes", _countof(columns), ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | 0 * ImGuiTableFlags_NoSavedSettings |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterV)) {
		TableSetupScrollFreeze(2, 1);

		int i = 0;
		for (auto& ci : columns)
			TableSetupColumn(ci.Header, ci.Flags, ci.Width, i++);

		TableHeadersRow();

		if (IsKeyPressed(ImGuiKey_Space)) {
			TogglePause();
		}

		auto result = m_KillDlg.ShowModal();
		if (result == MessageBoxResult::OK) {
			auto success = KillProcess(m_SelectedProcess->Id);
			if (success)
				m_SelectedProcess.reset();
			else {
				//SimpleMessageBox::ShowModal("Terminate Process", format("Process {} termination failed.", m_SelectedProcess->Id).c_str());
			}
		}

		if (m_UpdateInterval > 0 && ::GetTickCount64() - m_Tick >= m_UpdateInterval) {
			auto empty = m_Processes.empty();
			if (empty) {
				m_Processes.reserve(1024);
			}
			if (m_SelectedProcess && m_ShowLowerPane)
				pm.UpdateWithThreads(0);
			else
				pm.Update();
			if (empty) {
				m_Processes = pm.GetProcesses();
			}
			else {
				DoUpdate();
			}
			if (m_Specs)
				DoSort(m_Specs->ColumnIndex, m_Specs->SortDirection == ImGuiSortDirection_Ascending);
			m_Tick = ::GetTickCount64();
		}


		std::string filter;
		if (m_FilterText[0]) {
			filter = m_FilterText;
			_strlwr_s(filter.data(), filter.length() + 1);
		}
		std::vector<int> indices;
		indices.reserve(m_Processes.size());

		auto count = static_cast<int>(m_Processes.size());
		for (int i = 0; i < count; i++) {
			auto& p = m_Processes[i];
			if (p->Update()) {
				// process terminated
				if (p == m_SelectedProcess)
					m_SelectedProcess.reset();
				m_Processes.erase(m_Processes.begin() + i);
				i--;
				count--;
				continue;
			}
			p->Filtered = false;
			if (filter[0]) {
				p->Filtered = true;
				std::wstring name(p->GetImageName());
				if (!name.empty()) {
					_wcslwr_s(name.data(), name.length() + 1);
					wstring wfilter(filter.begin(), filter.end());
					if (name.find(wfilter) != wstring::npos) {
						p->Filtered = false;
					}
				}
				if (p->Filtered && to_string(p->Id).find(filter) != string::npos) {
					p->Filtered = false;
				}
				if (p->Filtered && format("{:x}", p->Id).find(filter) != string::npos) {
					p->Filtered = false;
				}
			}
			if (!p->Filtered)
				indices.push_back(i);
		}

		auto specs = TableGetSortSpecs();
		if (specs && specs->SpecsDirty) {
			m_Specs = specs->Specs;
			DoSort(m_Specs->ColumnIndex, m_Specs->SortDirection == ImGuiSortDirection_Ascending);
			specs->SpecsDirty = false;
		}
		ImGuiListClipper clipper;

		count = static_cast<int>(indices.size());
		clipper.Begin(count);

		while (clipper.Step()) {
			for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
				int i = indices[j];
				auto& p = m_Processes[i];
				if (p->Filtered) {
					clipper.ItemsCount--;
					continue;
				}
				TableNextRow();

				auto popCount = 0;
				auto colors = p->Colors();
				if (colors.first.x >= 0) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(colors.first));
					PushStyleColor(ImGuiCol_Text, colors.second);
					popCount = 1;
				}

				for (int i = 0; i < _countof(columns); i++) {
					if (TableSetColumnIndex(i)) {
						columns[i].Callback(p);
						if (i == 0 && IsItemFocused())
							m_SelectedProcess = p;
					}
				}

				PopStyleColor(popCount);

			}
		}
		EndTable();

		if (m_SelectedProcess && IsKeyPressed(ImGuiKey_Delete)) {
			TryKillProcess(*m_SelectedProcess);
		}

	}
	if (m_ShowLowerPane) {
		EndChild();
		BuildLowerPane();
	}
}

void ProcessesView::BuildViewMenu() {
	if (IsKeyPressed(ImGuiKey_L) && GetIO().KeyCtrl) {
		m_ShowLowerPane = !m_ShowLowerPane;
	}
	if (BeginMenu("View")) {
		if (MenuItem("Show Lower Pane", "Ctrl+L", m_ShowLowerPane)) {
			m_ShowLowerPane = !m_ShowLowerPane;
		}
		if (BeginMenu("Update Interval")) {
			if (MenuItem("500 ms", nullptr, m_UpdateInterval == 500))
				m_UpdateInterval = 500;
			if (MenuItem("1 second", nullptr, m_UpdateInterval == 1000))
				m_UpdateInterval = 1000;
			if (MenuItem("2 seconds", nullptr, m_UpdateInterval == 2000))
				m_UpdateInterval = 2000;
			if (MenuItem("5 seconds", nullptr, m_UpdateInterval == 5000))
				m_UpdateInterval = 5000;
			Separator();
			if (MenuItem("Paused", "SPACE", m_UpdateInterval == 0)) {
				TogglePause();
			}
			ImGui::EndMenu();
		}
		Separator();
		if (MenuItem("Refresh", "F5")) {
		}
		ImGui::EndMenu();
	}
}

void ProcessesView::BuildProcessMenu(ProcessInfoEx& pi) {
	if (MenuItem("Kill", "Delete")) {
		TryKillProcess(pi);
	}
	Separator();
	if (BuildPriorityClassMenu(pi))
		Separator();

	if (MenuItem(m_SelectedProcess->IsSuspended() ? "Resume" : "Suspend")) {
		pi.SuspendResume();
	}
	Separator();

	if (MenuItem("Go to file location...")) {
		GotoFileLocation(pi);
	}
	Separator();
	if (MenuItem("Properties...")) {
	//	GetOrAddProcessProperties(p);
	}
}

void ProcessesView::BuildToolBar() {
	if (ImageButton("Pause", m_Icons[IDI_PAUSE].Get(), ImVec2(16, 16))) {
		TogglePause();
	}
	SameLine();
	SetNextItemWidth(120);
	if (GetIO().KeyCtrl && IsKeyPressed(ImGuiKey_F))
		SetKeyboardFocusHere();

	InputTextWithHint("##Filter", "Filter (Ctrl+F)", m_FilterText, _countof(m_FilterText), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EscapeClearsAll);

	SameLine(0, 20);
	PushStyleColor(ImGuiCol_Button, StandardColors::DarkRed);

	if (ButtonEnabled("Kill", m_SelectedProcess != nullptr, ImVec2(40, 0))) {
		TryKillProcess(*m_SelectedProcess);
	}
	PopStyleColor(1);
	SameLine();
	static const struct {
		const char* Text;
		int Interval;
	} intervals[] = {
		{ "500 msec", 500 },
		{ "1 Second", 1000 },
		{ "2 Seconds", 2000 },
		{ "5 Seconds", 5000 },
		{ "Paused", 0 },
	};
	int current;
	for (int i = 0; i < _countof(intervals); i++) {
		if (intervals[i].Interval == m_UpdateInterval) {
			current = i;
			break;
		}
	}
	SetNextItemWidth(100);
	if (BeginCombo("Update", intervals[current].Text, ImGuiComboFlags_None)) {
		for (auto& item : intervals) {
			if (item.Interval == 0)
				break;
			if (MenuItem(item.Text, nullptr, m_UpdateInterval == item.Interval)) {
				m_UpdateInterval = item.Interval;
			}
		}
		Separator();
		if (MenuItem("Paused", "SPACE", m_UpdateInterval == 0)) {
			TogglePause();
		}
		EndCombo();
	}

	SameLine();
	bool open = Button("Colors", ImVec2(60, 0));
	if (open)
		OpenPopup("colors");

	if (BeginPopup("colors", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
		auto& colors = Globals::Settings().ProcessColors;

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
}

void ProcessesView::BuildLowerPane() {
	if (m_ShowLowerPane) {
		if (BeginChild("lowerpane", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_NoTitleBar)) {
			if (BeginTabBar("lowertabs", ImGuiTabBarFlags_NoCloseWithMiddleMouseButton | ImGuiTabBarFlags_Reorderable)) {
				if (m_SelectedProcess) {
					SameLine(200);
					Text("PID: %u (%ws)", m_SelectedProcess->Id, m_SelectedProcess->GetImageName().c_str()); SameLine();
				}
				if (BeginTabItem("Threads", nullptr, ImGuiTabItemFlags_None)) {
					if (m_SelectedProcess) {
						m_ThreadsView.BuildTable(m_SelectedProcess);
					}
					EndTabItem();
				}
				if (BeginTabItem("DLLs", nullptr, ImGuiTabItemFlags_None)) {
					EndTabItem();
				}
				EndTabBar();
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

			ImGui::EndMenu();
		}
	}
	return enabled;
}

bool ProcessesView::GotoFileLocation(ProcessInfo const& pi) {
	Process process;
	if (process.Open(pi.Id, ProcessAccessMask::QueryLimitedInformation)) {
		auto path = process.GetFullImageName();
		auto bs = path.rfind(L'\\');
		if (bs == std::wstring::npos)
			return false;

		auto folder = path.substr(0, bs);
		return (INT_PTR)::ShellExecute(nullptr, L"open", L"explorer", (L"/select,\"" + path + L"\"").c_str(),
			nullptr, SW_SHOWDEFAULT) > 31;
	}
	return false;
}

void ProcessesView::TogglePause() {
	if (m_UpdateInterval == 0) {
		m_UpdateInterval = m_OldInterval;
	}
	else {
		m_OldInterval = m_UpdateInterval;
		m_UpdateInterval = 0;
	}
}

//void ProcessesView::BuildPropertiesWindow(ProcessProperties* props) {
//	SetNextWindowSizeConstraints(ImVec2(300, 200), GetIO().DisplaySize);
//	SetNextWindowSize(ImVec2(GetIO().DisplaySize.x / 2, 300), ImGuiCond_Once);
//	if (Begin(props->GetName().c_str(), &props->WindowOpen, ImGuiWindowFlags_None)) {
//	}
//	End();
//}

//shared_ptr<ProcessProperties> ProcessesView::GetProcessProperties(WinSys::ProcessInfo* pi) {
//	auto it = _processProperties.find(pi->Key);
//	return it == _processProperties.end() ? nullptr : it->second;
//}

//shared_ptr<ProcessProperties> ProcessesView::GetOrAddProcessProperties(const std::shared_ptr<WinSys::ProcessInfo>& pi) {
//	auto props = GetProcessProperties(pi.get());
//	if (props == nullptr) {
//		CStringA name;
//		name.Format("%ws (%u) Properties##%lld", pi->GetImageName().c_str(), pi->Id, pi->CreateTime);
//		props = std::make_shared<ProcessProperties>(std::string(name), pi);
//		_processProperties.insert({ pi->Key, props });
//		_tm.AddWindow(props);
//	}
//	return props;
//}

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


#include "pch.h"
#include "ThreadsView.h"
#include "ProcessInfoEx.h"
#include "ThreadInfoEx.h"
#include "resource.h"
#include "Globals.h"
#include "TotalSysSettings.h"
#include "SortHelper.h"
#include "FormatHelper.h"
#include <ImGuiExt.h>
#include "ProcessHelper.h"
#include "IconHelper.h"
#include "ThreadStackWindow.h"
#include "MainWindow.h"
#include "UI.h"
#include "DriverHelper.h"

using namespace ImGui;
using namespace std;
using namespace WinLL;

ThreadsView::ThreadsView(DefaultProcessManager* external) : m_ActualProcMgr(external) {
	if (external == nullptr) {
		m_AllThreads = true;
		m_ActualProcMgr = &m_ProcMgr;
		Open(Globals::Settings().ThreadsWindowOpen());
	}
}

ThreadsView::~ThreadsView() {
	if (m_AllThreads)
		Globals::Settings().ThreadsWindowOpen(IsOpen());
}

void ThreadsView::Build() {
	if (!IsOpen())
		return;

	PushFont(Globals::VarFont());
	SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	if (Begin("All Threads", GetOpenAddress(), ImGuiWindowFlags_MenuBar)) {
		if (BeginMenuBar()) {
			if (BeginMenu("View")) {
				BuildUpdateIntervalMenu();
				ImGui::EndMenu();
			}
			if (m_SelectedThread != nullptr && BeginMenu("Thread")) {
				if (MenuItem("Stack")) {
					ShowThreadStack();
				}
				if (MenuItem("Suspend")) {
					m_SelectedThread->Suspend();
				}
				if (MenuItem("Resume")) {
					m_SelectedThread->Resume();
				}
				if (MenuItem("Terminate")) {
					m_SelectedThread->Terminate();
				}
				ImGui::EndMenu();
			}
			EndMenuBar();
		}
		BuildToolBar();
		RefreshAll();
		BuildTable(nullptr);
	}
	PopFont();
	ImGui::End();
}

void ThreadsView::BuildTable(std::shared_ptr<ProcessInfoEx> p) {
	auto pid = p ? p->Id : -1;
	auto orgBackColor = GetStyle().Colors[ImGuiCol_TableRowBg];

	static const ColumnInfo columns[]{
		{ "State", [&](auto& t) {
			Image(GetStateImage(t->State).Get(), ImVec2(16, 16)); SameLine();
			auto str = format("{}##{}" ,StateToString(t->State), t->Id);
			PushFont(Globals::VarFont());
			Selectable(str.c_str(), m_SelectedThread == t, ImGuiSelectableFlags_SpanAllColumns);
			PopFont();
			if (IsItemClicked()) {
				m_SelectedThread = t;
				SetItemDefaultFocus();
			}
			}, 0, 90 },
		{ "TID", [&](auto& t) {
			PushFont(Globals::MonoFont());
			if (m_Process && m_Process->Id == 0)
				Text(" CPU %4u", t->Id);
			else
				Text(Globals::Settings().HexIds() ? " 0x%08X" : " %8u", t->Id);
			PopFont();
			}, 0, 100 },
		{ "PID", [&](auto& t) {
			PushFont(Globals::MonoFont());
			Text(Globals::Settings().HexIds() ? " 0x%08X" : " %8u", t->ProcessId);
			PopFont();
			}, pid == -1 ? 0 : ImGuiTableColumnFlags_DefaultHide },
		{ "Process Name", [&](auto& t) {
			PushFont(Globals::VarFont());
			Text("%ws", t->GetProcessImageName().c_str());
			PopFont();
			}, pid == -1 ? 0 : ImGuiTableColumnFlags_DefaultHide },
		{ "Wait Reason", [](auto& t) {
			if (t->State == ThreadState::Waiting) {
				PushFont(Globals::VarFont());
				TextUnformatted(WaitReasonToString(t->WaitReason));
				PopFont();
			}
			}, 0, 120 },
		{ "CPU %", [&](auto& t) {
			if (t->CPU > 0 && t->State != ThreadState::Terminated) {
				auto value = t->CPU / 10000.0f;
				auto str = format("{:7.2f}  ", value);
				ImVec4 color;
				auto customColors = pid && value > 1.0f;
				if (customColors) {
					color = ProcessHelper::GetColorByCPU(value).Value;
				}
				else {
					color = orgBackColor;
				}
				PushFont(Globals::MonoFont());
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
		{ "Base Pri", [&](auto& t) {
			PushFont(Globals::MonoFont());
			Text("%4u", t->BasePriority);
			PopFont();
			}, ImGuiTableColumnFlags_NoResize, 65, },
		{ "Dyn Pri", [&](auto& t) {
			PushFont(Globals::MonoFont());
			Text("%4u", t->Priority);
			PopFont();
			}, ImGuiTableColumnFlags_NoResize, 65 },
		{ "CPU Time", [](auto& t) {
			PushFont(Globals::MonoFont());
			TextUnformatted(FormatHelper::FormatTimeSpan(t->UserTime + t->KernelTime).c_str());
			PopFont();
			}, ImGuiTableColumnFlags_NoResize },
		{ "Create Time", [](auto& t) {
			PushFont(Globals::MonoFont());
			Text(FormatHelper::FormatDateTime(t->CreateTime).c_str());
			PopFont();
			}, ImGuiTableColumnFlags_NoResize },
		{ "Kernel Time", [](auto& t) {
			PushFont(Globals::MonoFont());
			TextUnformatted(FormatHelper::FormatTimeSpan(t->KernelTime).c_str());
			PopFont();
			}, ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoResize },
		{ "User Time", [](auto& t) {
			PushFont(Globals::MonoFont());
			TextUnformatted(FormatHelper::FormatTimeSpan(t->UserTime).c_str());
			PopFont();
			}, ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoResize },
		{ "Start Address", [&](auto& t) {
			if (t->StartAddress) {
				string name;
				if (Globals::Settings().ResolveSymbols())
					name = m_Process->GetAddressSymbol((uint64_t)t->StartAddress);
				if (name.empty() || name[0] == '0') {
					PushFont(Globals::MonoFont());
					Text("0x%p", t->StartAddress);
				}
				else {
					PushFont(Globals::VarFont());
					TextUnformatted(name.c_str());
				}
				PopFont();
			}
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "Win32 Start Address", [&](auto& t) {
			if (t->Win32StartAddress) {
				string name;
				if (Globals::Settings().ResolveSymbols())
					name = m_Process->GetAddressSymbol((uint64_t)t->Win32StartAddress);
				if (name.empty() || name[0] == '0') {
					PushFont(Globals::MonoFont());
					Text("0x%p", t->Win32StartAddress);
				}
				else {
					PushFont(Globals::VarFont());
					TextUnformatted(name.c_str());
				}
				PopFont();
			}
			}, 0 },
		{ "TEB", [](auto& t) {
			if (t->TebBase) {
				PushFont(Globals::MonoFont());
				Text(" 0x%p ", t->TebBase);
				PopFont();
			}
			}, ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoResize },
		{ "Stack Base", [](auto& t) {
			if (t->StackBase) {
				PushFont(Globals::MonoFont());
				Text(" 0x%p ", t->StackBase);
				PopFont();
			}
			}, ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoResize },
		{ "Stack Limit", [](auto& t) {
			if (t->StackLimit) {
				PushFont(Globals::MonoFont());
				Text(" 0x%p ", t->StackLimit);
				PopFont();
			}
			}, ImGuiTableColumnFlags_DefaultHide | ImGuiTableColumnFlags_NoResize, 120 },
		{ "Suspend Cnt", [](auto& t) {
			auto count = t->GetSuspendCount();
			if (count) {
				PushFont(Globals::MonoFont());
				Text("%4d ", count);
				PopFont();
			}
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "Service", [](auto& t) {
			PushFont(Globals::VarFont());
			Text(" %ws ", t->GetServiceName().c_str());
			PopFont();
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "Ctx Switch", [](auto& t) {
			PushFont(Globals::MonoFont());
			Text("%12ws ", FormatHelper::FormatNumber(t->ContextSwitches).c_str());
			PopFont();
			}, ImGuiTableColumnFlags_DefaultHide, 100 },
		{ "Mem Pri", [](auto& t) {
			auto priority = t->GetMemoryPriority();
			if (priority >= 0) {
				PushFont(Globals::MonoFont());
				Text("%d", priority);
				PopFont();
			}
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "I/O Pri", [](auto& t) {
			PushFont(Globals::VarFont());
			Text(" %s ", FormatHelper::IoPriorityToString(t->GetIoPriority()));
			PopFont();
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "Wait Time", [](auto& t) {
			PushFont(Globals::MonoFont());
			TextUnformatted(FormatHelper::FormatTimeSpan(t->WaitTime).c_str());
			PopFont();
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "Description", [](auto& t) {
			auto desc = t->GetDescription();
			if (!desc.empty()) {
				PushFont(Globals::VarFont());
				Text("%ws", desc.c_str());
				PopFont();
			}
			} },
	};

	PushFont(Globals::VarFont());
	if (BeginTable("Threads", _countof(columns), ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit)) {
		TableSetupScrollFreeze(2, 1);

		int c = 0;
		auto header = pid == 0 ? "Processor" : "TID";
		for (auto& ci : columns) {
			TableSetupColumn(c == 1 ? header : ci.Header, ci.Flags, ci.Width, c);
			c++;
		}

		TableHeadersRow();
		auto specs = m_Specs = TableGetSortSpecs();
		if (specs->SpecsDirty) {
			specs->SpecsDirty = false;
			DoSort(specs->Specs->ColumnIndex, specs->Specs->SortDirection == ImGuiSortDirection_Ascending);
		}

		ImGuiListClipper clipper;
		clipper.Begin((int)m_Threads.size());

		while (clipper.Step()) {
			for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
				auto t = static_pointer_cast<ThreadInfoEx>(m_Threads[j]);
				TableNextRow();

				if (t->IsNew()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(Globals::Settings().GetProcessColors()[TotalSysSettings::NewObjects].Color));
				}
				else if (t->IsTerminated()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(Globals::Settings().GetProcessColors()[TotalSysSettings::DeletedObjects].Color));
				}

				for (int c = 0; c < _countof(columns); c++) {
					if (TableSetColumnIndex(c)) {
						columns[c].Callback(t);
						if (c == 0 && IsItemFocused())
							m_SelectedThread = t;
					}
				}
			}
		}

		EndTable();
	}
	PopFont();
	if (m_AllThreads && IsKeyReleased(ImGuiKey_Space)) {
		TogglePause();
	}

}

void ThreadsView::BuildToolBar() {
	auto selected = m_SelectedThread != nullptr;
	if (ButtonEnabled("Stack", selected)) {
		ShowThreadStack();
	}
	SameLine();
	if (ButtonEnabled("Suspend", selected)) {
		m_SelectedThread->Suspend();
	}
	SameLine();
	if (ButtonEnabled("Resume", selected)) {
		m_SelectedThread->Resume();
	}
	SameLine();
	if (ButtonEnabled("Kill", selected)) {
		m_SelectedThread->Terminate();
	}
	if (m_AllThreads) {
		SameLine();
		BuildUpdateIntervalToolBar();
	}
}

void ThreadsView::Clear() {
	m_Process = nullptr;
	m_Threads.clear();
	if (m_AllThreads)
		m_Threads.reserve(4096);
}

bool ThreadsView::RefreshAll(bool now) {
	bool empty = m_Threads.empty();

	auto& pm = m_ProcMgr;
	bool updated = now;
	if (now || (updated = NeedUpdate())) {
		pm.Update(true);
		UpdateTick();
	}
	if (empty) {
		m_Threads = pm.GetThreads();
	}
	else if (updated) {
		CommonRefresh();
	}
	return updated;
}

bool ThreadsView::RefreshProcess(std::shared_ptr<ProcessInfoEx>& p, bool now) {
	assert(p);
	auto pid = p->Id;
	bool newProcess = m_Threads.empty() || (m_Process && m_Process->Id != pid);

	assert(m_ActualProcMgr);
	auto& pm = *m_ActualProcMgr;

	if (newProcess) {
		m_Process = pm.GetProcessById(pid);
		m_Threads.clear();
		for (auto& t : m_Process->GetThreads())
			m_Threads.push_back(static_pointer_cast<ThreadInfoEx>(t));

		if (m_Specs)
			m_Specs->SpecsDirty = true;

		m_SelectedThread = nullptr;
	}
	else {
		CommonRefresh();
	}
	return !newProcess;
}

void ThreadsView::CommonRefresh() {
	auto count = m_Threads.size();
	for (size_t i = 0; i < count; i++) {
		auto& t = m_Threads[i];
		if (t->Update()) {
			m_Threads.erase(m_Threads.begin() + i);
			i--;
			count--;
			continue;
		}
	}

	auto& pm = *m_ActualProcMgr;

	for (auto t : pm.GetNewThreads()) {
		t->New(Globals::Settings().NewObjectsTime() * 1000);
		if (m_AllThreads || t->ProcessId == m_Process->Id)
			m_Threads.push_back(t);
	}
	for (auto t : pm.GetTerminatedThreads()) {
		t->Term(Globals::Settings().OldObjectsTime() * 1000);
	}
	if (m_Specs)
		m_Specs->SpecsDirty = true;

}

void ThreadsView::ShowThreadStack() {
	assert(m_SelectedThread);
	if (m_Process == nullptr)
		m_Process = m_ProcMgr.GetProcessById(m_SelectedThread->ProcessId);
	assert(m_Process);
	UI::SubmitWorkWithResult([=]() -> void* {
		auto frames = m_Process->GetSymbols().EnumThreadStack(m_Process->Id, m_SelectedThread->Id);
		if (!frames.empty()) {
			auto win = new ThreadStackWindow(m_Process, m_SelectedThread, frames);
			win->Open();
			return win;
		}
		return nullptr;
		},
		[=](auto result) {
			if (result)
				Globals::RootWindow().AddWindow(unique_ptr<ThreadStackWindow>((ThreadStackWindow*)result));
		});
}

PCSTR ThreadsView::StateToString(ThreadState state) {
	switch (state) {
		case ThreadState::Initialized: return "Init";
		case ThreadState::Ready: return "Ready";
		case ThreadState::Running: return "Running";
		case ThreadState::Waiting: return "Waiting";
		case ThreadState::DeferredReady: return "Deferred Ready";
		case ThreadState::Terminated: return "Terminated";
		case ThreadState::Standby: return "Standby";
		case ThreadState::Transition: return "Transition";
		case ThreadState::WaitingForProcessInSwap: return "Wait Inswap";
		case ThreadState::GateWaitObsolete: return "Gate Wait";
	}
	return "<unknown>";
}

PCSTR ThreadsView::WaitReasonToString(WinLL::WaitReason reason) {
	static PCSTR reasons[] = {
		"Executive",
		"FreePage",
		"PageIn",
		"PoolAllocation",
		"DelayExecution",
		"Suspended",
		"UserRequest",
		"WrExecutive",
		"WrFreePage",
		"WrPageIn",
		"WrPoolAllocation",
		"WrDelayExecution",
		"WrSuspended",
		"WrUserRequest",
		"WrEventPair",
		"WrQueue",
		"WrLpcReceive",
		"WrLpcReply",
		"WrVirtualMemory",
		"WrPageOut",
		"WrRendezvous",
		"WrKeyedEvent",
		"WrTerminated",
		"WrProcessInSwap",
		"WrCpuRateControl",
		"WrCalloutStack",
		"WrKernel",
		"WrResource",
		"WrPushLock",
		"WrMutex",
		"WrQuantumEnd",
		"WrDispatchInt",
		"WrPreempted",
		"WrYieldExecution",
		"WrFastMutex",
		"WrGuardedMutex",
		"WrRundown",
		"WrAlertByThreadId",
		"WrDeferredPreempt"
	};

	return (int)reason < _countof(reasons) ? reasons[(int)reason] : "<Unknown>";
}

D3D11Image& ThreadsView::GetStateImage(WinLL::ThreadState state) {
	return s_StateIcons[(int)state];
}

bool ThreadsView::Init() {
	UINT icons[]{
		IDI_THREAD_NEW, IDI_READY, IDI_RUNNING, IDI_STANDBY, IDI_STOP,
		IDI_PAUSE, IDI_TRANSITION, IDI_READY, 0, IDI_TRANSITION
	};

	int i = 0;
	for (auto icon : icons) {
		if (icon)
			s_StateIcons[i++] = D3D11Image::FromIcon(IconHelper::LoadIconFromResource(icon, 16));
	}
	return true;
}

void ThreadsView::DoSort(int column, bool asc) {
	std::sort(m_Threads.begin(), m_Threads.end(), [&](const auto& tx1, const auto& tx2) {
		auto t1 = static_pointer_cast<ThreadInfoEx>(tx1).get();
		auto t2 = static_pointer_cast<ThreadInfoEx>(tx2).get();

		switch (static_cast<Column>(column)) {
			case Column::State: return SortHelper::Sort(t1->State, t2->State, asc);
			case Column::Id: return SortHelper::Sort(t1->Id, t2->Id, asc);
			case Column::ProcessId: return SortHelper::Sort(t1->ProcessId, t2->ProcessId, asc);
			case Column::ProcessName: return SortHelper::Sort(t1->GetProcessImageName(), t2->GetProcessImageName(), asc);
			case Column::CPU: return SortHelper::Sort(t1->CPU, t2->CPU, asc);
			case Column::CreateTime: return SortHelper::Sort(t1->CreateTime, t2->CreateTime, asc);
			case Column::Priority: return SortHelper::Sort(t1->Priority, t2->Priority, asc);
			case Column::BasePriority: return SortHelper::Sort(t1->BasePriority, t2->BasePriority, asc);
			case Column::CPUTime: return SortHelper::Sort(t1->KernelTime + t1->UserTime, t2->KernelTime + t2->UserTime, asc);
			case Column::UserTime: return SortHelper::Sort(t1->UserTime, t2->UserTime, asc);
			case Column::KernelTime: return SortHelper::Sort(t1->KernelTime, t2->KernelTime, asc);
			case Column::Teb: return SortHelper::Sort(t1->TebBase, t2->TebBase, asc);
			case Column::WaitReason: return SortHelper::Sort(t1->WaitReason, t2->WaitReason, asc);
			case Column::StartAddress: return SortHelper::Sort(t1->StartAddress, t2->StartAddress, asc);
			case Column::Win32StartAddress: return SortHelper::Sort(t1->Win32StartAddress, t2->Win32StartAddress, asc);
			case Column::StackBase: return SortHelper::Sort(t1->StackBase, t2->StackBase, asc);
			case Column::StackLimit: return SortHelper::Sort(t1->StackLimit, t2->StackLimit, asc);
			case Column::ContextSwitches: return SortHelper::Sort(t1->ContextSwitches, t2->ContextSwitches, asc);
			case Column::WaitTime: return SortHelper::Sort(t1->WaitTime, t2->WaitTime, asc);
			case Column::MemoryPriority: return SortHelper::Sort(t1->GetMemoryPriority(), t2->GetMemoryPriority(), asc);
			case Column::IOPriority: return SortHelper::Sort(t1->GetIoPriority(), t2->GetIoPriority(), asc);
			case Column::Service: return SortHelper::Sort(t1->GetServiceName(), t2->GetServiceName(), asc);
			case Column::Desc: return SortHelper::Sort(t1->GetDescription(), t2->GetDescription(), asc);
			//case Column::ComFlags: return SortHelper::SortNumbers(GetThreadInfoEx(t1.get()).GetComFlags(), GetThreadInfoEx(t2.get()).GetComFlags(), asc);
			//case Column::ComApartment: return SortHelper::SortStrings(
			//	FormatHelper::ComApartmentToString(GetThreadInfoEx(t1.get()).GetComFlags()),
			//	FormatHelper::ComApartmentToString(GetThreadInfoEx(t2.get()).GetComFlags()), asc);
		}
		return false;
		});
}


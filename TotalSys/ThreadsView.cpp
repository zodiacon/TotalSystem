#include "pch.h"
#include "ThreadsView.h"
#include "ProcessInfoEx.h"
#include "ThreadInfoEx.h"
#include "resource.h"
#include "Globals.h"
#include "TotalSysSettings.h"
#include "SortHelper.h"
#include "FormatHelper.h"

using namespace ImGui;
using namespace std;
using namespace WinLL;

ThreadsView::ThreadsView(bool allThreads) : m_AllThreads(allThreads) {
}

void ThreadsView::BuildWindow() {
}

void ThreadsView::BuildTable(std::shared_ptr<ProcessInfoEx>& p) {
	bool newProcess = m_Process == nullptr || m_Process->Id != p->Id;

	auto& pm = m_ProcMgr;
	bool updated = false;
	if (newProcess || (updated = ::GetTickCount64() - m_LastUpdate > 1000)) {
		pm.UpdateWithThreads(p->Id);
		m_LastUpdate = ::GetTickCount64();
	}
	if (newProcess) {
		m_Process = m_ProcMgr.GetProcessById(p->Id);
		m_Threads = m_Process->GetThreads();
	}
	else if (updated) {
		for (auto t : pm.GetNewThreads()) {
			t->New(2000);
			m_Threads.push_back(t);
		}
		for (auto t : pm.GetTerminatedThreads()) {
			t->Term(2000);
		}

	}

	auto orgBackColor = GetStyle().Colors[ImGuiCol_TableRowBg];

	static const ColumnInfo columns[]{
		{ "State", [&](auto& t) {
			Image(GetStateImage(t.State).Get(), ImVec2(16, 16)); SameLine();
			auto str = format("{}##{}" ,StateToString(t.State), p->Id);
			Selectable(str.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
			}, 0, 60 },
		{ "TID", [&](auto& t) {
			if (p->Id == 0)
				Text("%7u", t.Id);
			else
				Text("%7u (0x%05X)", t.Id, t.Id);
			}, 0 },
		{ "PID", [&](auto& t) {
			Text("%7u (0x%05X)", t.ProcessId, t.ProcessId);
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "Process Name", [&](auto& t) {
			Text("%ws", t.GetProcessImageName().c_str());
			}, ImGuiTableColumnFlags_DefaultHide },
		{ "Wait Reason", [](auto& t) {
			if (t.State == ThreadState::Waiting)
				TextUnformatted(WaitReasonToString(t.WaitReason));
			}, 0, 120 },
		{ "CPU %", [&](auto& t) {
			if (t.CPU > 0 && t.State != ThreadState::Terminated) {
				auto value = t.CPU / 10000.0f;
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
		}, 0, 70 },
		{ "Base Pri", [&](auto& t) {
			Text("%4u", t.BasePriority);
			}, ImGuiTableColumnFlags_NoResize, 65, },
		{ "Dyn Pri", [&](auto& t) {
			Text("%4u", t.Priority);
			}, ImGuiTableColumnFlags_NoResize, 65 },
		{ "CPU Time", [](auto& t) {
			TextUnformatted(FormatHelper::FormatTimeSpan(t.UserTime + t.KernelTime).c_str());
			}, },
		{ "Create Time", [](auto& t) {
			Text(FormatHelper::FormatDateTime(t.CreateTime).c_str());
			},ImGuiTableColumnFlags_NoResize },
		{ "Kernel Time", [](auto& t) {
			TextUnformatted(FormatHelper::FormatTimeSpan(t.KernelTime).c_str());
			}, },
		{ "User Time", [](auto& t) {
			TextUnformatted(FormatHelper::FormatTimeSpan(t.UserTime).c_str());
			}, },
		{ "Start Address", [](auto& t) {
			if (t.StartAddress)
				TextUnformatted(format("{}", t.StartAddress).c_str());
			}, },
		{ "Win32 Start Address", [](auto& t) {
			if (t.Win32StartAddress)
				TextUnformatted(format("{}", t.Win32StartAddress).c_str());
			}, },

	};

	if (BeginTable("Threads", _countof(columns), ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit)) {

		TableSetupScrollFreeze(2, 1);

		int c = 0;
		PushFont(Globals::VarFont());
		auto header = p->Id == 0 ? "Processor" : "TID";
		for (auto& ci : columns) {
			TableSetupColumn(c == 1 ? header : ci.Header, ci.Flags, ci.Width, c);
			c++;
		}

		TableHeadersRow();
		PopFont();

		auto specs = TableGetSortSpecs();
		if (specs) {
			if (newProcess || updated)
				specs->SpecsDirty = true;

			if (specs->SpecsDirty) {
				auto sort = specs->Specs;
				DoSort(sort->ColumnIndex, sort->SortDirection == ImGuiSortDirection_Ascending);
				specs->SpecsDirty = false;
			}
		}
		auto count = (int)m_Threads.size();
		for (int i = 0; i < count; i++) {
			auto t = (ThreadInfoEx*)m_Threads[i].get();
			if (t->Update()) {
				m_Threads.erase(m_Threads.begin() + i);
				i--;
				count--;
				continue;
			}
		}

		ImGuiListClipper clipper;
		clipper.Begin((int)m_Threads.size());

		while (clipper.Step()) {
			for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
				auto& t = *(ThreadInfoEx*)m_Threads[j].get();
				TableNextRow();

				if (t.IsNew()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(Globals::Settings().ProcessColors[TotalSysSettings::NewObjects].Color));
				}
				else if (t.IsTerminated()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(Globals::Settings().ProcessColors[TotalSysSettings::DeletedObjects].Color));
				}

				for (int i = 0; i < _countof(columns); i++) {
					if (TableSetColumnIndex(i)) {
						columns[i].Callback(t);
					}
				}
			}
		}

		EndTable();
	}
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
			s_StateIcons[i++] = D3D11Image::FromIcon(
				(HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(icon), IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION | LR_COPYFROMRESOURCE));
	}
	return true;
}

void ThreadsView::DoSort(int column, bool asc) {
	std::sort(m_Threads.begin(), m_Threads.end(), [&](const auto& t1, const auto& t2) {
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
			//case Column::MemoryPriority: return SortHelper::Sort(t1->GetMemoryPriority(), t2->GetMemoryPriority(), asc);
			//case Column::IOPriority: return SortHelper::Sort(t1->GetIoPriority(), t2->GetIoPriority(), asc);
			//case Column::ComFlags: return SortHelper::SortNumbers(GetThreadInfoEx(t1.get()).GetComFlags(), GetThreadInfoEx(t2.get()).GetComFlags(), asc);
			//case Column::ComApartment: return SortHelper::SortStrings(
			//	FormatHelper::ComApartmentToString(GetThreadInfoEx(t1.get()).GetComFlags()),
			//	FormatHelper::ComApartmentToString(GetThreadInfoEx(t2.get()).GetComFlags()), asc);
		}
		return false;
		});
}

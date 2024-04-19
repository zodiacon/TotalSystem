#include "pch.h"
#include "ThreadsView.h"
#include "ProcessInfoEx.h"
#include "ThreadInfoEx.h"
#include "resource.h"
#include "Globals.h"
#include "ProcMgrSettings.h"

using namespace ImGui;
using namespace std;
using namespace WinLL;

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
		if (specs && specs->SpecsDirty) {
			auto sort = specs->Specs;
			//DoSort(m_Specs->ColumnIndex, m_Specs->SortDirection == ImGuiSortDirection_Ascending);
			specs->SpecsDirty = false;
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
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(Globals::Settings().ProcessColors[ProcMgrSettings::NewObjects].Color));
				}
				else if (t.IsTerminated()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(Globals::Settings().ProcessColors[ProcMgrSettings::DeletedObjects].Color));
				}
				//auto popCount = 0;
				//auto colors = p->Colors();
				//if (colors.first.x >= 0) {
				//	TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(colors.first));
				//	PushStyleColor(ImGuiCol_Text, colors.second);
				//	popCount = 1;
				//}

				for (int i = 0; i < _countof(columns); i++) {
					if (TableSetColumnIndex(i)) {
						columns[i].Callback(t);
					}
				}

				//PopStyleColor(popCount);

			}
		}

		EndTable();
	}
}

PCSTR ThreadsView::StateToString(ThreadState state) {
	switch (state) {
		case ThreadState::Initialized: return "Initialized";
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

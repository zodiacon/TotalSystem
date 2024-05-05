#include "pch.h"
#include "HandlesView.h"
#include "Globals.h"
#include "TotalSysSettings.h"
#include "FormatHelper.h"
#include "SortHelper.h"
#include "UI.h"
#include "AccessMaskDecoder.h"
#include "ObjectHelper.h"
#include "resource.h"

using namespace ImGui;
using namespace std;
using namespace WinLL;

void HandlesView::Init() {
	const struct {
		PCWSTR type;
		UINT icon;
	} icons[] = {
		{ L"", IDI_OBJECT },
		{ L"Process", IDI_PROCESS },
		{ L"Thread", IDI_THREAD },
		{ L"Mutant", IDI_LOCK },
		{ L"Semaphore", IDI_SEMAPHORE },
		{ L"Job", IDI_JOB },
		{ L"Section", IDI_SECTION },
		{ L"SymbolicLink", IDI_SYMLINK },
		{ L"Token", IDI_TOKEN },
		{ L"Key", IDI_KEY },
		{ L"File", IDI_FILE },
	};

	for(auto& icon : icons)
		s_Icons.insert({ icon.type, D3D11Image::FromIcon(
			(HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(icon.icon), IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION | LR_COPYFROMRESOURCE)) });


}

bool HandlesView::Track(uint32_t pid, PCWSTR type) {
	auto tracking = m_Tracker.Track(pid, type);
	if (tracking) {
		m_Handles.clear();
	}
	return tracking;
}

void HandlesView::BuildTable() {
	static const ColumnInfo columns[]{
		{ "Handle", [&](auto& h) {
			PushFont(Globals::MonoFont());
			auto text = format("0x{:08X}##%X", h->HandleValue, h->ProcessId);
			ImTextureID image;
			if (auto it = s_Icons.find(GetObjectType(h.get())); it != s_Icons.end())
				image = it->second.Get();
			else
				image = s_Icons[L""].Get();
			Image(image, ImVec2(16, 16)); SameLine();
			if (Selectable(text.c_str(), m_SelectedHandle == h, ImGuiSelectableFlags_SpanAllColumns)) {
				m_SelectedHandle = h;
			}
			PopFont();
			}, ImGuiTableColumnFlags_NoResize, },

		{ "Type", [&](auto& h) {
			PushFont(Globals::VarFont());
			Text("%ws", GetObjectType(h.get()).c_str());
			PopFont();
		}, ImGuiTableColumnFlags_NoHide, 150.0f },

		{ "Name", [&](auto& h) {
			PushFont(Globals::VarFont());
			Text("%ws", GetObjectName(h.get()).c_str());
			PopFont();
		}, 0, 400.0f },

		{ "PID", [](auto& h) {
			PushFont(Globals::MonoFont());
			Text(Globals::Settings().HexIds() ? "0x%08" : "%8u", h->ProcessId);
			PopFont();
		}, m_Tracker.GetPid() <= 0 ? 0 : ImGuiTableColumnFlags_DefaultHide, 80 },

		{ "Process Name", [&](auto& h) {
			PushFont(Globals::VarFont());
			Text("%ws", GetProcessName(h.get()).c_str());
			PopFont();
		}, m_Tracker.GetPid() <= 0 ? 0 : ImGuiTableColumnFlags_DefaultHide },

		{ "Access", [](auto& h) {
			PushFont(Globals::MonoFont());
			Text(" 0x%08X ", h->GrantedAccess);
			PopFont();
		}, ImGuiTableColumnFlags_NoResize, },

		{ "Address", [](auto& h) {
			PushFont(Globals::MonoFont());
			Text(" 0x%p ", h->Object);
			PopFont();
		}, ImGuiTableColumnFlags_NoResize },

		{ "Attributes", [](auto& h) {
			PushFont(Globals::VarFont());
			TextUnformatted(FormatHelper::HandleAttributesToString(h->HandleAttributes).c_str());
			PopFont();
		}, 0, 90},

		{ "Decoded Access", [&](auto& h) {
			PushFont(Globals::VarFont());
			TextUnformatted(AccessMaskDecoder::DecodeAccessMask(GetObjectType(h.get()), h->GrantedAccess).c_str());
			PopFont();
		}, 0, 240 },

		{ "Details", [&](auto& h) {
			auto hDup = WinLLX::ObjectManager::DupHandle(ULongToHandle(h->HandleValue), h->ProcessId);
			if (hDup) {
				PushFont(Globals::VarFont());
				TextUnformatted(ObjectHelper::GetObjectDetails(hDup, h.get(), GetObjectType(h.get()), &m_ProcMgr).c_str());
				PopFont();
				::CloseHandle(hDup);
			}
		}, 0, 250 },

	};

	if (BeginTable("Handles", _countof(columns), ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterV)) {
		TableSetupScrollFreeze(0, 1);

		auto& settings = Globals::Settings();
		int c = 0;
		PushFont(Globals::VarFont());
		for (auto& ci : columns)
			TableSetupColumn(ci.Header, ci.Flags, ci.Width, c++);
		TableHeadersRow();
		PopFont();

		auto specs = m_Specs = TableGetSortSpecs();
		if (specs->SpecsDirty) {
			specs->SpecsDirty = false;
			DoSort(specs->Specs->ColumnIndex, specs->Specs->SortDirection == ImGuiSortDirection_Ascending);
		}

		auto count = static_cast<int>(m_Handles.size());
		ImGuiListClipper clipper;
		clipper.Begin(count);

		while (clipper.Step()) {
			for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
				if (j >= m_Handles.size())
					break;
				auto& h = m_Handles[j];
				TableNextRow();

				if (h->Update()) {
					if (h == m_SelectedHandle)
						m_SelectedHandle.reset();
					m_Handles.Remove(j);
					j--;
					clipper.DisplayEnd--;
					clipper.ItemsCount--;
					continue;
				}

				if (h->IsNew()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(settings.GetProcessColors()[TotalSysSettings::NewObjects].Color));
				}
				else if (h->IsTerminated()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(settings.GetProcessColors()[TotalSysSettings::DeletedObjects].Color));
				}

				for (int c = 0; c < _countof(columns); c++) {
					if (TableSetColumnIndex(c)) {
						columns[c].Callback(h);
						if (c == 0 && IsItemFocused())
							m_SelectedHandle = h;
					}
				}
			}
		}

		EndTable();
	}
}

void HandlesView::BuildWindow() {
	if (!IsOpen())
		return;

	if (m_Tracker.GetPid() == 0)
		SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);

	PushFont(Globals::VarFont());
	if (Begin("All Handles", GetOpenAddress())) {
		Refresh(0);
		BuildTable();
	}
	PopFont();
	End();
}

bool HandlesView::Refresh(uint32_t pid, bool now) {
	if (!m_Updating && (NeedUpdate() || now)) {
		if (pid == 0)
			m_ProcMgr.Update();
		Track(pid);
		m_Updating = true;
		UI::SubmitWork(
			[&]() {
				m_Tracker.Update();
			},
			[&]() {

				auto empty = m_Handles.empty();
				if (empty) {
					m_Handles = m_Tracker.GetNewHandles();
					if (m_Specs)
						m_Specs->SpecsDirty = true;
				}
				else {
					for (auto& hi : m_Tracker.GetNewHandles()) {
						m_Handles.push_back(hi);
						hi->New(Globals::Settings().NewObjectsTime() * 1000);
					}

					for (auto& hi : m_Tracker.GetClosedHandles()) {
						hi->Term(Globals::Settings().OldObjectsTime() * 1000);
					}
					if (m_Specs)
						m_Specs->SpecsDirty = true;
				}
				m_Updating = false;
			});
		return true;
	}
	return false;
}

std::wstring const& HandlesView::GetObjectName(HandleInfoEx* hi) const {
	if (!hi->NameChecked) {
		hi->Name = WinLLX::ObjectManager::GetObjectName((HANDLE)(ULONG_PTR)hi->HandleValue, hi->ProcessId, hi->ObjectTypeIndex);
		hi->NameChecked = true;
	}
	return hi->Name;
}

std::wstring const& HandlesView::GetObjectType(HandleInfoEx* hi) const {
	if (hi->Type.empty())
		hi->Type = WinLLX::ObjectManager::GetType(hi->ObjectTypeIndex)->TypeName;
	return hi->Type;
}

std::wstring const& HandlesView::GetProcessName(HandleInfoEx* hi) const {
	if (!hi->ProcessNameChecked) {
		hi->ProcessNameChecked = true;
		auto p = m_ProcMgr.GetProcessById(hi->ProcessId);
		if (p)
			hi->ProcessName = p->GetImageName();
	}
	return hi->ProcessName;
}

void HandlesView::DoSort(int col, bool asc) {
	auto compare = [&](auto const& h1, auto const& h2) {
		switch (static_cast<Column>(col)) {
			case Column::Address: return SortHelper::Sort(h1->Object, h2->Object, asc);
			case Column::ProcessName: return SortHelper::Sort(h1->ProcessName, h2->ProcessName, asc);
			case Column::Name: return SortHelper::Sort(GetObjectName(h1.get()), GetObjectName(h2.get()), asc);
			case Column::Type: return SortHelper::Sort(GetObjectType(h1.get()), GetObjectType(h2.get()), asc);
			case Column::Handle: return SortHelper::Sort(h1->HandleValue, h2->HandleValue, asc);
			case Column::Attributes: return SortHelper::Sort(h1->HandleAttributes, h2->HandleAttributes, asc);
			case Column::PID: return SortHelper::Sort(h1->ProcessId, h2->ProcessId, asc);
			case Column::Access:
			case Column::DecodedAccess:
				return SortHelper::Sort(h1->GrantedAccess, h2->GrantedAccess, asc);
		}
		return false;
		};

	m_Handles.Sort(compare, m_Handles.size() > 100000);
}


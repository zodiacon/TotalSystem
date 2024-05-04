#include "pch.h"
#include "ModulesView.h"
#include "Globals.h"
#include "TotalSysSettings.h"
#include "FormatHelper.h"
#include "SortHelper.h"
#include "resource.h"

using namespace ImGui;
using namespace WinLLX;

static auto textSize = 512;
static auto text = std::make_unique<char[]>(textSize);

bool ModulesView::Track(uint32_t pid) {
	if (m_Tracker.GetPid() == pid)
		return true;

	auto tracking = m_Tracker.TrackProcess(pid);
	if (tracking) {
		m_Modules.clear();
		m_SelectedModule = nullptr;
	}
	return tracking;
}

void ModulesView::BuildTable() {
	static const ColumnInfo columns[]{
		{ "Name", [&](auto& m) {
			sprintf_s(text.get(), textSize, "%ws", m->Name.c_str());
			Image(s_Icons[m->Type == MapType::Data ? 1 : 0].Get(), ImVec2(16, 16)); SameLine();
			PushFont(Globals::VarFont());
			if (Selectable(text[0] ? text.get() : "<Pagefile backed>", m_SelectedModule == m, ImGuiSelectableFlags_SpanAllColumns)) {
				m_SelectedModule = m;
			}
			PopFont();
			}, 0, 200.0f },

		{ "Type", [](auto& m) {
			PushFont(Globals::VarFont());
			TextUnformatted(m->Type == MapType::Image ? "Image" : "Data");
			PopFont();
		}, ImGuiTableColumnFlags_NoResize, 70.0f },

		{ "Size", [](auto& m) {
			PushFont(Globals::MonoFont());
			Text("0x%08X", m->ModuleSize);
			PopFont();
		}, 0, 90.0f },

		{ "Base Address", [](auto& m) {
			PushFont(Globals::MonoFont());
			Text("0x%p", m->Base);
			PopFont();
		}, ImGuiTableColumnFlags_NoResize },

		{ "Image Base", [](auto& m) {
			if (m->ImageBase) {
				PushFont(Globals::MonoFont());
				Text("0x%p", m->ImageBase);
				PopFont();
			}
		}, ImGuiTableColumnFlags_NoResize },
		{ "Full Path", [&](auto& m) {
			PushFont(Globals::VarFont());
			Text("%ws", m->Path.c_str());
			PopFont();
			}, 0, 400 },
		{ "Characteristics", [](auto& m) {
			PushFont(Globals::MonoFont());
			Text("0x%04X", (uint16_t)m->Characteristics);
			PopFont();
			if (m->Characteristics != DllCharacteristics::None) {
				PushFont(Globals::VarFont());
				SameLine();
				Text("(%s)", FormatHelper::DllCharacteristicsToString((uint16_t)m->Characteristics).c_str());
				PopFont();
			}
			}, 0, 260 },
	};

	if (BeginTable("Modules", _countof(columns), ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterV)) {
		TableSetupScrollFreeze(1, 1);

		auto& settings = Globals::Settings();
		int c = 0;
		PushFont(Globals::VarFont());
		for (auto& ci : columns)
			TableSetupColumn(ci.Header, ci.Flags, ci.Width, c++);
		PopFont();

		TableHeadersRow();
		auto specs = m_Specs = TableGetSortSpecs();
		if (specs->SpecsDirty) {
			specs->SpecsDirty = false;
			DoSort(specs->Specs->ColumnIndex, specs->Specs->SortDirection == ImGuiSortDirection_Ascending);
		}

		auto count = static_cast<int>(m_Modules.size());
		ImGuiListClipper clipper;
		clipper.Begin(count);

		while (clipper.Step()) {
			for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
				auto& m = m_Modules[j];
				TableNextRow();

				if (m->IsNew()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(settings.GetProcessColors()[TotalSysSettings::NewObjects].Color));
				}
				else if (m->IsTerminated()) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(settings.GetProcessColors()[TotalSysSettings::DeletedObjects].Color));
				}
				else if (m->ImageBase && m->ImageBase != m->Base) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(settings.GetRelocatedColor()));
				}
				for (int c = 0; c < _countof(columns); c++) {
					if (TableSetColumnIndex(c)) {
						columns[c].Callback(m);
						if (c == 0 && IsItemFocused())
							m_SelectedModule = m;
					}
				}
			}
		}

		EndTable();
	}
}

bool ModulesView::Refresh(uint32_t pid, bool now) {
	if (NeedUpdate() || now) {
		Track(pid);
		auto empty = m_Modules.empty();
		m_Tracker.Update();

		auto count = static_cast<int>(m_Modules.size());
		for (int i = 0; i < count; i++) {
			auto& m = m_Modules[i];
			if (m->Update()) {
				if (m == m_SelectedModule)
					m_SelectedModule.reset();
				m_Modules.Remove(i);
				i--;
				count--;
				continue;
			}
		}

		if (empty) {
			m_Modules = m_Tracker.GetModules();
		}
		else {
			for (auto& mi : m_Tracker.GetNewModules()) {
				m_Modules.push_back(mi);
				mi->New(Globals::Settings().NewObjectsTime * 1000);
			}

			for (auto& mi : m_Tracker.GetUnloadedModules()) {
				mi->Term(Globals::Settings().OldObjectsTime * 1000);
			}
		}

		if (m_Specs)
			m_Specs->SpecsDirty = true;

		return true;
	}
	return false;
}

void ModulesView::Init() {
	UINT icons[]{
		IDI_DLL, IDI_MOD_DATA,
	};

	int i = 0;
	for (auto icon : icons) {
		s_Icons[i++] = D3D11Image::FromIcon(
			(HICON)::LoadImage(::GetModuleHandle(nullptr), MAKEINTRESOURCE(icon), IMAGE_ICON, 16, 16, LR_CREATEDIBSECTION | LR_COPYFROMRESOURCE));
	}
}

void ModulesView::DoSort(int col, bool asc) {
	m_Modules.Sort([&](auto& m1, auto& m2) {
		switch (static_cast<Column>(col)) {
			case Column::Name: return SortHelper::Sort(m1->Name, m2->Name, asc);
			case Column::Type: return SortHelper::Sort(m1->Type, m2->Type, asc);
			case Column::BaseAddress: return SortHelper::Sort(m1->Base, m2->Base, asc);
			case Column::ImageBase: return SortHelper::Sort(m1->ImageBase, m2->ImageBase, asc);
			case Column::Size: return SortHelper::Sort(m1->ModuleSize, m2->ModuleSize, asc);
			case Column::Path: return SortHelper::Sort(m1->Path, m2->Path, asc);
			case Column::Characteristics: return SortHelper::Sort(m1->Characteristics, m2->Characteristics, asc);
		}
		return false;
		});
}

#include "pch.h"
#include "ModulesView.h"
#include "Globals.h"
#include "TotalSysSettings.h"
#include "FormatHelper.h"
#include "SortHelper.h"
#include "resource.h"
#include "IconHelper.h"
#include "DriverHelper.h"
#include "PEHelper.h"

using namespace ImGui;
using namespace WinLL;
using namespace WinLLX;
using namespace std;

static auto textSize = 512;
static auto text = std::make_unique<char[]>(textSize);

void ModulesView::InitColumns() {
	const ColumnInfo columns[]{
		{ Column::Name, "Name", [&](auto& m) {
			PushFont(Globals::VarFont());
			if (IsKernel()) {
				Image(s_Icons[2], ImVec2(16, 16)); SameLine();
				if (Selectable(m->KName.c_str(), m_SelectedModule == m, ImGuiSelectableFlags_SpanAllColumns)) {
					m_SelectedModule = m;
				}
			}
			else {
				sprintf_s(text.get(), textSize, "%ws", m->Name.c_str());
				Image(s_Icons[m->Type == MapType::Data ? 1 : 0], ImVec2(16, 16)); SameLine();
				if (Selectable(format("{}##{}", text[0] ? text.get() : "<Pagefile backed>", m->Base).c_str(), m_SelectedModule == m, ImGuiSelectableFlags_SpanAllColumns)) {
					m_SelectedModule = m;
				}
			}
			PopFont();
			}, 0, 200.0f },

		{ Column::Type, "Type", [](auto& m) {
			PushFont(Globals::VarFont());
			TextUnformatted(m->Type == MapType::Image ? "Image" : "Data");
			PopFont();
		}, IsKernel() ? ImGuiTableColumnFlags_Disabled : (ImGuiTableColumnFlags_NoResize | ImGuiTableColumnFlags_DefaultHide), 70.0f },

		{ Column::Size, "Size", [](auto& m) {
			PushFont(Globals::MonoFont());
			Text("0x%08X", m->ModuleSize);
			PopFont();
		}, 0, 90.0f },

		{ Column::BaseAddress, "Base Address", [](auto& m) {
			PushFont(Globals::MonoFont());
			Text("0x%p", m->Base);
			PopFont();
		}, ImGuiTableColumnFlags_NoResize | (IsKernel() ? ImGuiTableColumnFlags_DefaultHide : 0) },

		{ Column::ImageBase, "Image Base", [](auto& m) {
			if (m->ImageBase) {
				PushFont(Globals::MonoFont());
				Text("0x%p", m->ImageBase);
				PopFont();
			}
		}, ImGuiTableColumnFlags_NoResize },
		{ Column::Path, "Path", [&](auto& m) {
			PushFont(Globals::VarFont());
			Text("%ws", m->Path.c_str());
			PopFont();
			}, 0, 400 },
		{ Column::Characteristics, "Characteristics", [](auto& m) {
			PushFont(Globals::MonoFont());
			Text("0x%04X", (uint16_t)m->Characteristics);
			PopFont();
			if (m->Characteristics != DllCharacteristics::None) {
				PushFont(Globals::VarFont());
				SameLine();
				Text("(%s)", FormatHelper::DllCharacteristicsToString((uint16_t)m->Characteristics).c_str());
				PopFont();
			}
			}, IsKernel() ? ImGuiTableColumnFlags_Disabled : ImGuiTableColumnFlags_DefaultHide, 150 },
		{ Column::Description, "Description", [](auto& m) {
			if (m->Type == MapType::Image) {
				PushFont(Globals::VarFont());
				Text("%ws", m->GetDescription().c_str());
				PopFont();
			}
		}, 0, 180 },
		{ Column::Company, "Company", [](auto& m) {
			if (m->Type == MapType::Image) {
				PushFont(Globals::VarFont());
				Text("%ws", m->GetCompanyName().c_str());
				PopFont();
			}
		}, 0, 150 },
	};
	m_Columns.clear();
	m_Columns.insert(m_Columns.end(), begin(columns), end(columns));

}

bool ModulesView::Track(uint32_t pid) {
	if (m_Pid == pid)
		return true;

	if (pid < 4)
		return false;

	auto wasKernel = m_Pid == 4;
	bool tracking = true;
	m_Modules.clear();
	m_Pid = pid;
	m_TheTracker = nullptr;
	if (pid == 4) {
		m_TheTracker = &m_KernelTracker;
	}
	else {
		tracking = m_Tracker.TrackProcess(
			DriverHelper::OpenProcess(pid, ProcessAccessMask::QueryInformation | ProcessAccessMask::VmRead | ProcessAccessMask::Syncronize));
		if(tracking)
			m_TheTracker = &m_Tracker;
	}
	if (m_Columns.empty() || wasKernel && IsKernel())
		InitColumns();
	m_SelectedModule = nullptr;
	return tracking;
}

void ModulesView::BuildTable() {
	auto columns = (int)m_Columns.size();
	if (BeginTable("Modules", columns, ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterV)) {
		TableSetupScrollFreeze(1, 1);

		auto& settings = Globals::Settings();
		int c = 0;
		PushFont(Globals::VarFont());
		for (auto& ci : m_Columns)
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
				else if (!IsKernel() && m->ImageBase && m->ImageBase != m->Base) {
					TableSetBgColor(ImGuiTableBgTarget_RowBg0, ColorConvertFloat4ToU32(settings.GetRelocatedColor()));
				}
				for (int c = 0; c < columns; c++) {
					if (TableSetColumnIndex(c)) {
						m_Columns[c].Callback(m);
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
	if (now || NeedUpdate()) {
		if (!Track(pid) || !m_TheTracker)
			return false;

		auto empty = m_Modules.empty();
		m_TheTracker->Update();

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
			m_Modules = m_TheTracker->GetModules();
		}
		else {
			for (auto& mi : m_TheTracker->GetNewModules()) {
				m_Modules.push_back(mi);
				mi->New(Globals::Settings().NewObjectsTime() * 1000);
			}

			for (auto& mi : m_TheTracker->GetUnloadedModules()) {
				mi->Term(Globals::Settings().OldObjectsTime() * 1000);
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
		IDI_DLL, IDI_MOD_DATA, IDI_SYSFILE,
	};

	int i = 0;
	for (auto icon : icons) {
		s_Icons[i++] = D3D11Image::FromIcon(IconHelper::LoadIconFromResource(icon, 16));
	}
}

void ModulesView::Build() {
}

bool ModulesView::IsKernel() const {
	return m_Pid == 4;
}

void ModulesView::DoSort(int col, bool asc) {
	m_Modules.Sort([&](auto& m1, auto& m2) {
		auto kernel = IsKernel();
		switch (static_cast<Column>(col)) {
			case Column::Name: return kernel ? SortHelper::Sort(m1->KName, m2->KName, asc) : SortHelper::Sort(m1->Name, m2->Name, asc);
			case Column::Type: return SortHelper::Sort(m1->Type, m2->Type, asc);
			case Column::BaseAddress: return SortHelper::Sort(m1->Base, m2->Base, asc);
			case Column::ImageBase: return SortHelper::Sort(m1->ImageBase, m2->ImageBase, asc);
			case Column::Size: return SortHelper::Sort(m1->ModuleSize, m2->ModuleSize, asc);
			case Column::Path: return SortHelper::Sort(m1->Path, m2->Path, asc);
			case Column::Characteristics: return SortHelper::Sort(m1->Characteristics, m2->Characteristics, asc);
			case Column::Company: return SortHelper::Sort(m1->GetCompanyName(), m2->GetCompanyName(), asc);
			case Column::Description: return SortHelper::Sort(m1->GetDescription(), m2->GetDescription(), asc);
		}
		return false;
		});
}

std::wstring const& ModuleInfoEx::GetDescription() const {
	if (m_Desc.empty())
		m_Desc = PEHelper::GetVersionObject(Path, L"FileDescription");
	return m_Desc;
}

std::wstring const& ModuleInfoEx::GetCompanyName() const {
	if (m_Company.empty())
		m_Company = PEHelper::GetVersionObject(Path, L"CompanyName");
	return m_Company;
}

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
#include <ImGuiExt.h>
#include "IconHelper.h"
#include "DriverHelper.h"

using namespace ImGui;
using namespace std;
using namespace WinLL;

void HandlesView::Init() {
	const struct {
		PCWSTR type;
		UINT icon;
	} icons[] = {
		{ L"", IDI_OBJECT },
		{ L"Event", IDI_EVENT },
		{ L"Process", IDI_PROCESS },
		{ L"Directory", IDI_DIRECTORY },
		{ L"Thread", IDI_THREAD },
		{ L"Mutant", IDI_LOCK },
		{ L"Semaphore", IDI_SEMAPHORE },
		{ L"Job", IDI_JOB },
		{ L"Timer", IDI_TIMER },
		{ L"IRTimer", IDI_TIMER },
		{ L"Section", IDI_SECTION },
		{ L"SymbolicLink", IDI_SYMLINK },
		{ L"Token", IDI_TOKEN },
		{ L"Key", IDI_KEY },
		{ L"File", IDI_FILE },
		{ L"ALPC Port", IDI_PLUG },
		{ L"Desktop", IDI_DESKTOP },
		{ L"WindowStation", IDI_WINSTATION },
		{ L"TpWorkerFactory", IDI_FACTORY },
	};

	for (auto& icon : icons)
		s_Icons.insert({ icon.type, D3D11Image::FromIcon(IconHelper::LoadIconFromResource(icon.icon, 16)) });
}

HandlesView::HandlesView() {
	InitColumns();
	Open(Globals::Settings().HandlesWindowOpen());
}

void HandlesView::InitColumns() {
	const ColumnInfo columns[]{
		{ Column::Handle, "Handle", [&](auto& h) {
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

		{ Column::Type, "Type", [&](auto& h) {
			PushFont(Globals::VarFont());
			Text("%ws", GetObjectType(h.get()).c_str());
			PopFont();
		}, ImGuiTableColumnFlags_NoHide, 150.0f },

		{ Column::Name, "Name", [&](auto& h) {
			PushFont(Globals::VarFont());
			Text("%ws", GetObjectName(h.get()).c_str());
			PopFont();
		}, 0, 400.0f },

		{ Column::PID, "PID", [](auto& h) {
			PushFont(Globals::MonoFont());
			Text(Globals::Settings().HexIds() ? "0x%08X" : "%8u", h->ProcessId);
			PopFont();
		}, m_Tracker.GetPid() <= 0 ? 0 : ImGuiTableColumnFlags_DefaultHide, 80 },

		{ Column::ProcessName, "Process Name", [&](auto& h) {
			PushFont(Globals::VarFont());
			Text("%ws", GetProcessName(h.get()).c_str());
			PopFont();
		}, m_Tracker.GetPid() <= 0 ? 0 : ImGuiTableColumnFlags_DefaultHide },

		{ Column::Access, "Access", [](auto& h) {
			PushFont(Globals::MonoFont());
			Text(" 0x%08X ", h->GrantedAccess);
			PopFont();
		}, ImGuiTableColumnFlags_NoResize, },

		{ Column::Address, "Address", [](auto& h) {
			PushFont(Globals::MonoFont());
			Text(" 0x%p ", h->Object);
			PopFont();
		}, ImGuiTableColumnFlags_NoResize },

		{ Column::Attributes, "Attributes", [](auto& h) {
			PushFont(Globals::VarFont());
			TextUnformatted(FormatHelper::HandleAttributesToString(h->HandleAttributes).c_str());
			PopFont();
		}, ImGuiTableColumnFlags_DefaultHide },

		{ Column::DecodedAccess, "Decoded Access", [&](auto& h) {
			PushFont(Globals::VarFont());
			TextUnformatted(AccessMaskDecoder::DecodeAccessMask(GetObjectType(h.get()), h->GrantedAccess).c_str());
			PopFont();
		}, 0, 240 },

		{ Column::Details, "Details", [&](auto& h) {
			PushFont(Globals::VarFont());
			TextUnformatted(ObjectHelper::GetObjectDetails(h.get(), GetObjectType(h.get()), &m_ProcMgr).c_str());
			PopFont();
		}, ImGuiTableColumnFlags_NoSort, 250 },
	};

	m_Columns.insert(m_Columns.end(), begin(columns), end(columns));
}

HandlesView::~HandlesView() {
	if (m_AllHandles)
		Globals::Settings().HandlesWindowOpen(IsOpen());
}

bool HandlesView::Track(uint32_t pid, PCWSTR type) {
	if (m_Tracker.GetPid() == pid)
		return true;

	auto tracking = m_Tracker.Track(pid, type);
	if (tracking) {
		m_Handles.clear();
		m_SelectedHandle = nullptr;
	}
	return tracking;
}

void HandlesView::BuildTable() noexcept {
	auto result = m_ModalBox.ShowModal();
	if (result == MessageBoxResult::OK) {
		ObjectHelper::CloseHandle(m_SelectedHandle.get());
	}

	int columns = (int)m_Columns.size();
	if (BeginTable(m_AllHandles ? "AllHandles" : "Handles", columns, ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersOuterV)) {
		TableSetupScrollFreeze(0, 1);

		auto& settings = Globals::Settings();
		int c = 0;
		PushFont(Globals::VarFont());
		for (auto& ci : m_Columns)
			TableSetupColumn(ci.Header, ci.Flags, ci.Width, c++);
		TableHeadersRow();
		PopFont();

		if (m_FilterChanged) {
			m_FilterChanged = false;
			_strlwr_s(m_FilterText);
			if (m_FilterText[0] || m_NamedObjects)
				m_Handles.Filter([&](auto& h, auto) { return Filter(h); });
			else
				m_Handles.Filter(nullptr);
		}

		auto specs = m_Specs = TableGetSortSpecs();
		if (specs->SpecsDirty) {
			specs->SpecsDirty = false;
			DoSort(specs->Specs->ColumnIndex, specs->Specs->SortDirection == ImGuiSortDirection_Ascending);
		}

		auto count = static_cast<int>(m_Handles.size());
		ImGuiListClipper clipper;
		clipper.Begin(count);

		bool open = false;
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

				for (int c = 0; c < columns; c++) {
					if (TableSetColumnIndex(c)) {
						m_Columns[c].Callback(h);
						if (c == 0 && IsItemFocused())
							m_SelectedHandle = h;
						if (TableGetColumnFlags(c) & ImGuiTableColumnFlags_IsHovered) {
							m_HoveredColumn = c;
						}

						if (m_HoveredColumn == c)
							if (DoContextMenu(h.get(), c))
								m_SelectedHandle = h;

					}
				}

			}
		}

		EndTable();
	}
}

void HandlesView::Build() {
	if (!IsOpen())
		return;

	if (m_Tracker.GetPid() == 0) {
		m_AllHandles = true;
		SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
	}
	PushFont(Globals::VarFont());
	if (Begin("All Handles", GetOpenAddress())) {
		Refresh(0);
		BuildToolBar();
		BuildTable();
	}
	PopFont();
	End();
}

void HandlesView::BuildToolBar() noexcept {
	bool selected = m_SelectedHandle != nullptr;
	PushFont(Globals::VarFont());
	if (Shortcut(ImGuiKey_H | ImGuiMod_Ctrl))
		SetKeyboardFocusHere();
	SetNextItemWidth(110);
	static DWORD64 lastTime = ::GetTickCount64();
	if (InputTextWithHint("##Filter", "Filter (Ctrl+H)", m_FilterText, _countof(m_FilterText),
		ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EscapeClearsAll) && ::GetTickCount64() - lastTime > 250) {
		m_FilterChanged = true;
	}

	SameLine();
	if (Checkbox("Named Objects", &m_NamedObjects)) {
		m_FilterChanged = true;
		m_UpdateNow = true;
	}
	SameLine(); Spacing(); SameLine();
	if (ButtonEnabled("Close Handle", selected)) {
		PromptCloseHandle(m_SelectedHandle.get());
	}
	SameLine(); Spacing(); SameLine();
	if (ButtonEnabled("Copy", selected)) {
		SetClipboardText(DoCopy(m_SelectedHandle.get(), -1).c_str());
	}
	PopFont();
}

bool HandlesView::Refresh(uint32_t pid, bool now) noexcept {
	if (!m_Updating && (NeedUpdate() || now || m_UpdateNow)) {
		m_UpdateNow = false;
		if (m_Updating)
			return false;

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
					m_FilterChanged = true;
				}
				else {
					for (auto& hi : m_Tracker.GetNewHandles()) {
						m_Handles.push_back(hi);
						hi->New(Globals::Settings().NewObjectsTime() * 1000);
					}

					for (auto& hi : m_Tracker.GetClosedHandles()) {
						hi->Term(Globals::Settings().OldObjectsTime() * 1000);
					}
				}
				if (m_Specs)
					m_Specs->SpecsDirty = true;
				m_Updating = false;
			});
		return true;
	}
	return false;
}

std::wstring const& HandlesView::GetObjectName(HandleInfoEx* hi) const noexcept {
	if (!hi->NameChecked) {
		auto hDup = DriverHelper::DupHandle((HANDLE)(ULONG_PTR)hi->HandleValue, hi->ProcessId, GENERIC_READ, 0);
		if (hDup) {
			hi->Name = WinLLX::ObjectManager::GetObjectName(hDup, hi->ObjectTypeIndex);
			if (hi->Name.starts_with(L"\\Device\\"))
				hi->Name = ObjectHelper::NativePathToDosPath(hi->Name);
			::CloseHandle(hDup);
		}
		hi->NameChecked = true;
	}
	return hi->Name;
}

std::wstring const& HandlesView::GetObjectType(HandleInfoEx* hi) const noexcept {
	if (hi->Type.empty())
		hi->Type = WinLLX::ObjectManager::GetType(hi->ObjectTypeIndex)->TypeName;
	return hi->Type;
}

std::wstring const& HandlesView::GetProcessName(HandleInfoEx* hi) const noexcept {
	if (!hi->ProcessNameChecked) {
		hi->ProcessNameChecked = true;
		auto p = m_ProcMgr.GetProcessById(hi->ProcessId);
		if (p)
			hi->ProcessName = p->GetImageName();
	}
	return hi->ProcessName;
}

void HandlesView::DoSort(int col, bool asc) noexcept {
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

void HandlesView::PromptCloseHandle(HandleInfoEx* hi) const {
	m_ModalBox.Init("Close Handle", format("Close Handle 0x{:X}?", hi->HandleValue), MessageBoxButtons::YesNo);
}

string HandlesView::DoCopy(HandleInfoEx* hi, int c) const noexcept {
	string text;
	if (c < 0) {
		for (c = 0; c < (int)Column::_Count; c++) {
			text += DoCopy(hi, c) + ",";
		}
		return text.substr(0, text.length() - 1);
	}
	switch (static_cast<Column>(c)) {
		case Column::Handle: text = format("0x{:X}", hi->HandleValue); break;
		case Column::Name: text = FormatHelper::UnicodeToUtf8(GetObjectName(hi).c_str()); break;
		case Column::Type: text = FormatHelper::UnicodeToUtf8(GetObjectType(hi).c_str()); break;
		case Column::Address: text = format("0x{}", hi->Object); break;
		case Column::Attributes: text = FormatHelper::HandleAttributesToString(hi->HandleAttributes); break;
		case Column::PID: text = Globals::Settings().HexIds() ? format("0x{:X}", hi->ProcessId) : format("{}", hi->ProcessId); break;
		case Column::ProcessName: text = FormatHelper::UnicodeToUtf8(GetProcessName(hi).c_str()); break;
		case Column::Access: text = format("0x{:X}", hi->GrantedAccess); break;
		case Column::DecodedAccess: text = AccessMaskDecoder::DecodeAccessMask(GetObjectType(hi), hi->GrantedAccess); break;
		case Column::Details: text = ObjectHelper::GetObjectDetails(hi, GetObjectType(hi)); break;
	}

	return text;
}

bool HandlesView::DoContextMenu(HandleInfoEx* h, int c) const noexcept {
	if (BeginPopupContextItem(format("{}{}", h->HandleValue, h->ProcessId).c_str())) {
		PushFont(Globals::VarFont());
		if (MenuItem("Close Handle")) {
			PromptCloseHandle(h);
		}
		if (MenuItem("Copy")) {
			auto text = DoCopy(h, c);
			if (!text.empty())
				SetClipboardText(text.c_str());
		}
		PopFont();
		EndPopup();
		return true;
	}
	return false;
}

bool HandlesView::Filter(std::shared_ptr<HandleInfoEx> const& h) const {
	if (m_NamedObjects && GetObjectName(h.get()).empty())
		return false;

	if (m_FilterText[0] == 0)
		return true;

	auto type = FormatHelper::UnicodeToUtf8(GetObjectType(h.get()).c_str());
	if (!type.empty()) {
		_strlwr_s(type.data(), type.length() + 1);
		if (type.find(m_FilterText) != string::npos)
			return true;
	}
	auto name = FormatHelper::UnicodeToUtf8(GetObjectName(h.get()).c_str());
	if (!name.empty()) {
		_strlwr_s(name.data(), name.length() + 1);
		if (name.find(m_FilterText) != string::npos)
			return true;
	}
	return false;
}


#include "pch.h"
#include "ProcessesView.h"
#include "SortHelper.h"
#include "resource.h"
#include "Globals.h"
#include <ProcessManager.h>
#include <MessageBox.h>
#include <chrono>
#include <Colors.h>
#include <ImGuiExt.h>
#include <Shlwapi.h>
#include "FormatHelpers.h"

#pragma comment(lib, "Shlwapi.lib")

using namespace ImGui;
using namespace std;
using namespace WinLL;

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext* g_pd3dDeviceContext;

bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);

ProcessesView::ProcessesView() {
	UINT width, height;
	wil::com_ptr<IWICImagingFactory> spFactory = wil::CoCreateInstance<IWICImagingFactory>(CLSID_WICImagingFactory);
	wil::com_ptr<IWICBitmap> spBitmap;
	spFactory->CreateBitmapFromHICON(::LoadIcon(::GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_PROCMGR)), &spBitmap);
	spBitmap->GetSize(&width, &height);

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; //DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	wil::com_ptr<ID3D11Texture2D> pTexture;
	D3D11_SUBRESOURCE_DATA subResource;
	wil::com_ptr<IWICBitmapLock> spLock;
	spBitmap->Lock(nullptr, WICBitmapLockRead, &spLock);
	UINT size;
	WICInProcPointer ptr;
	spLock->GetDataPointer(&size, &ptr);
	subResource.pSysMem = ptr;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;
	g_pd3dDevice->CreateTexture2D(&desc, &subResource, pTexture.addressof());

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;	//DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	g_pd3dDevice->CreateShaderResourceView(pTexture.get(), &srvDesc, &m_spImage);
}

void ProcessesView::BuildWindow() {
	if (Begin("Processes", nullptr, ImGuiWindowFlags_MenuBar)) {
		if (BeginMenuBar()) {
			BuildProcessMenu();
			BuildViewMenu();
			EndMenuBar();
		}
		BuildToolBar();
		BuildTable();
	}
	ImGui::End();

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

void ProcessesView::DoSort(int col, bool asc) {
	ranges::sort(m_Processes, [&](const auto& p1, const auto& p2) {
		switch (col) {
			case 0: return SortHelper::Sort(p1->GetImageName(), p2->GetImageName(), asc);
			case 1: return SortHelper::Sort(p1->Id, p2->Id, asc);
			case 2: return SortHelper::Sort(p1->UserName(), p2->UserName(), asc);
			case 3: return SortHelper::Sort(p1->SessionId, p2->SessionId, asc);
			case 4: return SortHelper::Sort(p1->CPU, p2->CPU, asc);
			case 5: return SortHelper::Sort(p1->ParentId, p2->ParentId, asc);
			case 6: return SortHelper::Sort(p1->CreateTime, p2->CreateTime, asc);
			case 7: return SortHelper::Sort(p1->PrivatePageCount, p2->PrivatePageCount, asc);
			case 8: return SortHelper::Sort(p1->BasePriority, p2->BasePriority, asc);
			case 9: return SortHelper::Sort(p1->ThreadCount, p2->ThreadCount, asc);
			case 10: return SortHelper::Sort(p1->HandleCount, p2->HandleCount, asc);
			case 11: return SortHelper::Sort(p1->WorkingSetSize, p2->WorkingSetSize, asc);
			case 12: return SortHelper::Sort(p1->GetExecutablePath(), p2->GetExecutablePath(), asc);
			case 13: return SortHelper::Sort(p1->KernelTime + p1->UserTime, p2->KernelTime + p2->UserTime, asc);
			case 14: return SortHelper::Sort(p1->PeakThreads, p2->PeakThreads, asc);
			case 15: return SortHelper::Sort(p1->VirtualSize, p2->VirtualSize, asc);
			case 16: return SortHelper::Sort(p1->PeakWorkingSetSize, p2->PeakWorkingSetSize, asc);
			case 17: return SortHelper::Sort(p1->Attributes(), p2->Attributes(), asc);
			case 18: return SortHelper::Sort(p1->PagedPoolUsage, p2->PagedPoolUsage, asc);

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

bool ProcessesView::TryKillProcess(ProcessInfo& pi, bool& success) {
	m_ModalOpen = true;
	const std::string name(pi.GetImageName().begin(), pi.GetImageName().end());
	auto text = format("Kill process {} ({})?", pi.Id, name);

	auto result = SimpleMessageBox::ShowModal("Kill Process?", text.c_str(), MessageBoxButtons::OkCancel);
	if (result != MessageBoxResult::StillOpen) {
		m_ModalOpen = false;
		if (result == MessageBoxResult::OK) {
			success = KillProcess(m_SelectedProcess->Id);
			if (success)
				m_SelectedProcess.reset();
		}
		return true;
	}
	return false;
}

void ProcessesView::BuildTable() {
	auto& pm = Globals::ProcessManager();

	//(ImVec2(size.x, size.y / 2));
	if (BeginTable("processes", 19, ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | 0 * ImGuiTableFlags_NoSavedSettings |
		ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Hideable | ImGuiTableFlags_SizingFixedFit)) {
		TableSetupScrollFreeze(2, 1);
		TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder);
		TableSetupColumn("Id", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_NoReorder);
		TableSetupColumn("User Name", 0, 110.0f);
		TableSetupColumn("Session", ImGuiTableColumnFlags_NoResize, 60.0f);
		TableSetupColumn("CPU (%)");
		TableSetupColumn("Parent", ImGuiTableColumnFlags_None);
		TableSetupColumn("Created", ImGuiTableColumnFlags_NoResize);
		TableSetupColumn("Private Bytes");
		TableSetupColumn("Priority");
		TableSetupColumn("Threads");
		TableSetupColumn("Handles");
		TableSetupColumn("Working Set");
		TableSetupColumn("Executable Path", ImGuiTableColumnFlags_None);
		TableSetupColumn("CPU Time");
		TableSetupColumn("Peak Thr");
		TableSetupColumn("Virtual Size");
		TableSetupColumn("Peak WS");
		TableSetupColumn("Attributes");
		TableSetupColumn("Paged Pool");

		TableHeadersRow();

		if (IsKeyPressed(ImGuiKey_Space)) {
			TogglePause();
		}

		if (m_UpdateInterval > 0 && ::GetTickCount64() - m_Tick >= m_UpdateInterval) {
			auto empty = m_Processes.empty();
			if (empty) {
				m_Processes.reserve(1024);
			}
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
			auto& p = *m_Processes[i];
			p.Filtered = false;
			if (p.Update()) {
				// process terminated
				m_Processes.erase(m_Processes.begin() + i);
				i--;
				count--;
				continue;
			}
			if (filter[0]) {
				std::wstring name(p.GetImageName());
				if (!name.empty()) {
					_wcslwr_s(name.data(), name.length() + 1);
					wstring wfilter(filter.begin(), filter.end());
					if (name.find(wfilter.c_str()) == wstring::npos) {
						p.Filtered = true;
						continue;
					}
				}
			}
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
		auto special = false;
		static char buffer[64];
		std::string str;

		int popCount = 3;
		static bool selected = false;

		auto orgBackColor = GetStyle().Colors[ImGuiCol_TableRowBg];

		if (m_KillFailed && MessageBoxResult::StillOpen != SimpleMessageBox::ShowModal("Kill Process", "Failed to kill process!")) {
			m_KillFailed = false;
		}
		while (clipper.Step()) {
			for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
				int i = indices[j];
				auto& p = m_Processes[i];
				if (p->Filtered) {
					clipper.ItemsCount--;
					continue;
				}
				TableNextRow();

				if (special)
					PopStyleColor(popCount);

				//auto colors = p.GetColors(pm);
				//special = colors.first.x >= 0 || p == _selectedProcess;
				//if (special) {
				//	if (p == _selectedProcess) {
				//		const auto& color = GetStyle().Colors[ImGuiCol_TextSelectedBg];
				//		PushStyleColor(ImGuiCol_TableRowBg, color);
				//		PushStyleColor(ImGuiCol_TableRowBgAlt, color);
				//		PushStyleColor(ImGuiCol_Text, GetStyle().Colors[ImGuiCol_Text]);
				//	}
				//	else {
				//		PushStyleColor(ImGuiCol_TableRowBg, colors.first);
				//		PushStyleColor(ImGuiCol_TableRowBgAlt, colors.first);
				//		PushStyleColor(ImGuiCol_Text, colors.second);
				//	}
				//}

				TableSetColumnIndex(0);
				Image(p->Icon(), ImVec2(16, 16)); SameLine();
				const std::string name(p->GetImageName().begin(), p->GetImageName().end());
				str = format("{}##{}", name.c_str(), i);
				Selectable(str.c_str(), m_SelectedProcess.get() == p.get(), ImGuiSelectableFlags_SpanAllColumns);

				::StringCchPrintfA(buffer, sizeof(buffer), "##%d", i);

				if (IsItemClicked()) {
					m_SelectedProcess = p;
				}

				if (m_SelectedProcess != nullptr && IsKeyPressed(ImGuiKey_Delete) && GetIO().KeyShift)
					m_ModalOpen = true;

				if (BeginPopupContextItem(buffer)) {
					m_SelectedProcess = p;
					if (BuildPriorityClassMenu(*p)) {
						Separator();
					}
					if (m_SelectedProcess->Id > 4 && MenuItem("Kill", "Shift+DEL")) {
						m_ModalOpen = true;
						Separator();
					}
					if (MenuItem("Go to file location...")) {
						GotoFileLocation(*m_SelectedProcess);
					}
					Separator();
					//if (MenuItem("Properties...")) {
					//	GetOrAddProcessProperties(p);
					//}
					EndPopup();
				}

				TableSetColumnIndex(1);
				Text("%6u (0x%05X)", p->Id, p->Id);

				if (TableSetColumnIndex(2)) {
					auto& username = p->GetUserName(true);
					if (username.empty())
						TextColored(StandardColors::Gray, "<access denied>");
					else
						Text("%ws", username.c_str());
				}

				if (TableSetColumnIndex(3)) {
					Text("%4u", p->SessionId);
				}

				if (TableSetColumnIndex(4)) {
					if (p->CPU > 0 && !p->IsTerminated()) {
						auto value = p->CPU / 10000.0f;
						str = format("{:7.2f}  ", value);
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
				}

				if (TableSetColumnIndex(5)) {
					if (p->ParentId > 0) {
						auto parent = pm.GetProcessById(p->ParentId);
						if (parent && parent->CreateTime < p->CreateTime) {
							Text("%6d ", parent->Id);
							SameLine();
							Text("(%ws)", parent->GetImageName().c_str());
						}
						else {
							Text("%6d", p->ParentId);
						}
					}
				}

				if (TableSetColumnIndex(6)) {
					DWORD flags = FDTF_DEFAULT;
					Text(FormatHelpers::FormatDateTime(p->CreateTime).c_str());
				}

				if (TableSetColumnIndex(7)) {
					Text("%12ws K", FormatHelpers::FormatNumber(p->PrivatePageCount >> 10).c_str());
				}
				if (TableSetColumnIndex(8)) {
					Text("%5d", p->BasePriority);
				}
				if (TableSetColumnIndex(9)) {
					Text("%6ws", FormatHelpers::FormatNumber(p->ThreadCount).c_str());
				}
				if (TableSetColumnIndex(10)) {
					Text("%6ws", FormatHelpers::FormatNumber(p->HandleCount).c_str());
				}
				if (TableSetColumnIndex(11)) {
					Text("%12ws K", FormatHelpers::FormatNumber(p->WorkingSetSize >> 10).c_str());
				}
				if (TableSetColumnIndex(12))
					Text("%ws", p->GetExecutablePath().c_str());

				if (TableSetColumnIndex(13)) {
					TextUnformatted(FormatHelpers::FormatTimeSpan(p->UserTime + p->KernelTime).c_str());
				}
				if (TableSetColumnIndex(14)) {
					Text("%7ws", FormatHelpers::FormatNumber(p->PeakThreads).c_str());
				}

				if (TableSetColumnIndex(15)) {
					Text("%14ws K", FormatHelpers::FormatNumber(p->VirtualSize >> 10).c_str());
				}

				if (TableSetColumnIndex(16)) {
					Text("%12ws K", FormatHelpers::FormatNumber(p->PeakWorkingSetSize >> 10).c_str());
				}

				if (TableSetColumnIndex(17))
					TextUnformatted(ProcessAttributesToString(p->Attributes()).c_str());

				if (TableSetColumnIndex(18)) {
					Text("%9ws K", FormatHelpers::FormatNumber(p->PagedPoolUsage >> 10).c_str());
				}
			}
		}
		if (special)
			PopStyleColor(popCount);
		EndTable();

		if (m_ModalOpen) {
			bool success;
			m_ModalOpen = TryKillProcess(*m_SelectedProcess, success);
		}
	}
}

void ProcessesView::BuildViewMenu() {
	if (BeginMenu("View")) {
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
			if (MenuItem("Paused", "SPACE", m_UpdateInterval == 0))
				TogglePause();
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
}

void ProcessesView::BuildProcessMenu() {
	if (BeginMenu("Process")) {
		if (m_SelectedProcess) {
			BuildPriorityClassMenu(*m_SelectedProcess);
			Separator();
		}
		if (MenuItem("Kill", "Delete", false, m_SelectedProcess != nullptr)) {
			bool success;
			if (TryKillProcess(*m_SelectedProcess, success) && !success)
				m_KillFailed = true;
		}
		//Separator();
		ImGui::EndMenu();
	}
}

void ProcessesView::BuildToolBar() {
	Separator();
	SetNextItemWidth(100);
	if (GetIO().KeyCtrl && IsKeyPressed(ImGuiKey_F))
		SetKeyboardFocusHere();

	InputText("Filter", m_FilterText, _countof(m_FilterText), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EscapeClearsAll);

	SameLine(0, 20);
	PushStyleColor(ImGuiCol_Button, StandardColors::DarkRed);
	PushStyleColor(ImGuiCol_Text, StandardColors::White);

	if (ButtonEnabled("Kill", m_SelectedProcess != nullptr, ImVec2(40, 0))) {
		bool success;
		TryKillProcess(*m_SelectedProcess, success);
	}
	PopStyleColor(2);
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
	Text("Update Interval"); SameLine(0, 6);
	SetNextItemWidth(100);
	if (BeginCombo("##Update Interval", intervals[current].Text, ImGuiComboFlags_None)) {
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

	//if (BeginPopup("colors", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
	//	auto& colors = Globals::Get().GetSettings().ProcessColors;

	//	for (auto& c : colors) {
	//		Checkbox(c.Name, &c.Enabled);
	//		SameLine(150);
	//		ColorEdit4("Background##" + c.Name, (float*)&c.Color, ImGuiColorEditFlags_NoInputs);
	//		SameLine();
	//		if (Button("Reset##" + c.Name))
	//			c.Color = c.DefaultColor;

	//		SameLine();
	//		ColorEdit4("Text##" + c.Name, (float*)&c.TextColor, ImGuiColorEditFlags_NoInputs);
	//		SameLine();
	//		if (Button("Reset##Text" + c.Name))
	//			c.TextColor = c.DefaultTextColor;
	//	}

	//	EndPopup();
	//}
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



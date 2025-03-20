#include "pch.h"
#include "MainWindow.h"
#include "SecurityHelper.h"
#include "Globals.h"
#include "resource.h"
#include "TotalSysSettings.h"
#include <Psapi.h>

using namespace ImGui;
using namespace WinLL;
using namespace std;

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {
	Globals::SetMainWindow(this);
	UINT const icons[]{ IDI_PAUSE, IDI_SPLIT, IDI_WINDOW, IDI_RUNNING };

	for (auto icon : icons) {
		Globals::ImageManager().Add(icon);
	}
}

void BuildMainDockSpace(ImGuiViewport const* viewport, unsigned dockspace_flags) {
	// Submit a window filling the entire viewport
	SetNextWindowPos(viewport->WorkPos);
	ImVec2 size(viewport->WorkSize.x, viewport->WorkSize.y - GetFrameHeight() * 1.2f);
	SetNextWindowSize(size);
	SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags host_window_flags = 0;
	host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
	host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		host_window_flags |= ImGuiWindowFlags_NoBackground;

	if (dockspace_flags & ImGuiDockNodeFlags_KeepAliveOnly)
		host_window_flags |= ImGuiWindowFlags_NoMouseInputs;

	PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	Begin("maindockspace", nullptr, host_window_flags);
	PopStyleVar(3);

	// Submit the dockspace
	auto id = GetID("DockSpace");
	DockSpace(id, ImVec2(0.0f, 0.0f), dockspace_flags, nullptr);

	End();
}

void MainWindow::Build() {
	auto vp = GetMainViewport();

	BuildMainDockSpace(vp, ImGuiDockNodeFlags_PassthruCentralNode);

	// status bar

	auto size = ImVec2(vp->WorkSize.x, GetFrameHeight() * 1.2f);
	SetNextWindowDockID(1);

	auto pos = ImVec2(vp->Pos.x, vp->Pos.y + vp->Size.y - size.y);
	SetNextWindowPos(pos);
	SetNextWindowSize(size);
	SetNextWindowViewport(vp->ID);
	
	if (Begin("StatusBar", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoDecoration)) {
		BuildStatusBar();
	}
	End();

	static SimpleMessageBox about;

	//ImGui::ShowIDStackToolWindow();

	m_DoSave = false;
	if (BeginMainMenuBar()) {
		PushFont(Globals::VarFont());
		if (BeginMenu("File")) {
			if (!SecurityHelper::IsRunningElevated() && MenuItem("Run As Administrator...")) {
				if (SecurityHelper::RunElevated()) {
					::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
				}
			}
			m_DoSave = MenuItem("Save...");
			Separator();
			if (MenuItem("Exit")) {
				::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
			}
			ImGui::EndMenu();
		}
		if (BeginMenu("System")) {
			if (MenuItem("Processes", "F4", m_ProcessesView.IsOpen(), !m_ProcessesView.IsOpen())) {
				m_ProcessesView.Open();
			}
			if (MenuItem("Threads", nullptr, m_ThreadsView.IsOpen(), !m_ThreadsView.IsOpen())) {
				m_ThreadsView.Clear();
				m_ThreadsView.Open();
			}
			if (MenuItem("Handles", nullptr, m_HandlesView.IsOpen(), !m_HandlesView.IsOpen())) {
				m_HandlesView.Track(0);
				m_HandlesView.Open();
			}
			ImGui::EndMenu();
		}
		if (BeginMenu("Options")) {
			if (MenuItem("Always on Top", nullptr, IsAlwaysOnTop())) {
				ToggleAlwaysOnTop();
			}
			auto& settings = Globals::Settings();
			if (MenuItem("Hex IDs", nullptr, settings.HexIds())) {
				settings.HexIds(!settings.HexIds());
			}
			if (MenuItem("Resolve Symbols", nullptr, settings.ResolveSymbols())) {
				settings.ResolveSymbols(!settings.ResolveSymbols());
			}
			if (BeginMenu("Theme")) {
				if (MenuItem("Dark", nullptr, Globals::IsDarkMode() && !settings.ThemeAsSystem())) {
					Globals::SetDarkMode(true);
				}
				if (MenuItem("Light", nullptr, !Globals::IsDarkMode() && !settings.ThemeAsSystem())) {
					Globals::SetDarkMode(false);
				}
				Separator();
				if (MenuItem("As System", nullptr, settings.ThemeAsSystem())) {
					RegistryKey key;
					if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)")) {
						DWORD value;
						if (ERROR_SUCCESS == key.QueryDWORDValue(L"AppsUseLightTheme", value)) {
							Globals::SetAsSystem(value == 0);
						}
					}
				}
				ImGui::EndMenu();
			}
			if (BeginMenu("Highlight Duration")) {
				SliderInt("New Objects", settings.NewObjectsTimeAddress(), 1, 9, "%d Seconds");
				SliderInt("Old Objects", settings.OldObjectsTimeAddress(), 1, 9, "%d Seconds");
				ImGui::EndMenu();
			}
			auto replaced = SecurityHelper::IsExeReplaced(L"taskmgr.exe");
			if (MenuItem("Replace Task Manager", nullptr, replaced)) {
				WCHAR path[MAX_PATH];
				::GetModuleFileName(nullptr, path, _countof(path));
				SecurityHelper::ReplaceExe(L"taskmgr.exe", path, replaced);
			}
			ImGui::EndMenu();
		}
		if (BeginMenu("Help")) {
			if (MenuItem("About Total System...")) {			
				about.Init("About Total System", "Version 0.2 (C)2024 Pavel Yosifovich");
				about.SetFont(Globals::VarFont());
			}
			ImGui::EndMenu();
		}
		PopFont();
		EndMainMenuBar();
	}

	about.ShowModal();

	m_ProcessesView.Build();
	m_ThreadsView.Build();
	m_HandlesView.Build();

	for (size_t i = 0; i < m_Windows.size(); i++) {
		auto& win = m_Windows[i];
		win->Build();
		if (!win->IsOpen()) {
			m_Windows.erase(m_Windows.begin() + i);
			i--;
		}
	}
}

bool MainWindow::IsAlwaysOnTop() const {
	return (::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

bool MainWindow::ToggleAlwaysOnTop() {
	auto onTop = !IsAlwaysOnTop();
	::SetWindowPos(m_hWnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	Globals::Settings().AlwaysOnTop(onTop);

	return onTop;
}

bool MainWindow::SaveSelected() const {
	return m_DoSave;
}

bool MainWindow::HandleMessage(UINT msg, WPARAM wp, LPARAM lp) {
	static bool done = false;
	switch (msg) {
		case WM_SHOWWINDOW:
			if (!done) {
				done = true;
				Globals::Settings().LoadWindowPosition(m_hWnd, L"MainWindowPlacement");
				if (Globals::Settings().AlwaysOnTop())
					ToggleAlwaysOnTop();
			}
			break;

		case WM_CLOSE:
			Globals::Settings().SaveWindowPosition(m_hWnd, L"MainWindowPlacement");
			break;
	}
	return false;
}

bool MainWindow::AddWindow(std::unique_ptr<Window> win) {
	m_Windows.push_back(move(win));
	return true;
}

void MainWindow::SetTheme() {
	if (Globals::Settings().ThemeAsSystem()) {
		RegistryKey key;
		if (ERROR_SUCCESS == key.Open(HKEY_CURRENT_USER, LR"(Software\Microsoft\Windows\CurrentVersion\Themes\Personalize)")) {
			DWORD value;
			if (ERROR_SUCCESS == key.QueryDWORDValue(L"AppsUseLightTheme", value)) {
				Globals::SetAsSystem(value == 0);
			}
		}
	}
	else {
		Globals::SetDarkMode(Globals::IsDarkMode());
	}
}

void MainWindow::BuildStatusBar() {
	PushFont(Globals::VarFont());
	static PERFORMANCE_INFORMATION pi;
	if (NeedUpdate()) {
		::GetPerformanceInfo(&pi, sizeof(pi));
		UpdateTick();
	}
	Text("Processes: %u Threads: %u Handles: %u Commit: %u GB / %u GB RAM: %u GB / %u GB Kernel Paged: %u MB Non-Paged: %u MB",
		pi.ProcessCount, pi.ThreadCount, pi.HandleCount, pi.CommitTotal >> 18, pi.CommitLimit >> 18, 
		(pi.PhysicalTotal - pi.PhysicalAvailable) >> 18, pi.PhysicalTotal >> 18, pi.KernelPaged >> 8, pi.KernelNonpaged >> 8);
	PopFont();
}



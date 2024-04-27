#include "pch.h"
#include "MainWindow.h"
#include "SecurityHelper.h"
#include "Globals.h"
#include "resource.h"
#include "TotalSysSettings.h"
#include <Psapi.h>

using namespace ImGui;
using namespace WinLL;

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {
	Globals::SetMainWindow(this);
	UINT icons[]{ IDI_PAUSE, IDI_SPLIT, IDI_WINDOW, IDI_RUNNING };

	for (auto& icon : icons) {
		Globals::ImageManager().Add(icon);
	}
}

void MainWindow::BuildWindow() {
	auto viewport = GetMainViewport();
	DockSpaceOverViewport(viewport, ImGuiDockNodeFlags_None);

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
			ImGui::EndMenu();
		}
		if (BeginMenu("Options")) {
			if (MenuItem("Always on Top", nullptr, IsAlwaysOnTop())) {
				ToggleAlwaysOnTop();
			}
			auto& settings = Globals::Settings();
			if (BeginMenu("Theme")) {
				if (MenuItem("Dark", nullptr, Globals::IsDarkMode() && !settings.ThemeAsSystem)) {
					Globals::SetDarkMode(true);
				}
				if (MenuItem("Light", nullptr, !Globals::IsDarkMode() && !settings.ThemeAsSystem)) {
					Globals::SetDarkMode(false);
				}
				Separator();
				if (MenuItem("As System", nullptr, settings.ThemeAsSystem)) {
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
				SliderInt("New Objects", (int*)&settings.NewObjectsTime, 1, 9, "%d Seconds");
				SliderInt("Old Objects", (int*)&settings.OldObjectsTime, 1, 9, "%d Seconds");
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
				about.Init("About Total System", "Version 0.1.5 (C)2024 Pavel Yosifovich");
				about.SetFont(Globals::VarFont());
			}
			ImGui::EndMenu();
		}
		PopFont();
		EndMainMenuBar();
	}

	about.ShowModal();

	SetNextWindowPos(viewport->WorkPos, ImGuiCond_FirstUseEver);
	SetNextWindowSize(viewport->WorkSize, ImGuiCond_FirstUseEver);
	m_ProcessesView.BuildWindow();
	if (m_ThreadsView.IsOpen()) {
		m_ThreadsView.BuildWindow();
	}
}

bool MainWindow::IsAlwaysOnTop() const {
	return (::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

bool MainWindow::ToggleAlwaysOnTop() {
	auto onTop = !IsAlwaysOnTop();
	::SetWindowPos(m_hWnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	return onTop;
}

bool MainWindow::SaveSelected() const {
	return m_DoSave;
}



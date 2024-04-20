#include "pch.h"
#include "MainWindow.h"
#include "SecurityHelper.h"
#include "Globals.h"

using namespace ImGui;

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {
}

void MainWindow::BuildWindow() {
	DockSpaceOverViewport(GetMainViewport());

	if (BeginMainMenuBar()) {
		PushFont(Globals::VarFont());
		if(BeginMenu("File")) {
			if (!WinLL::SecurityHelper::IsRunningElevated() && MenuItem("Run As Administrator...")) {
				if(WinLL::SecurityHelper::RunElevated()) {
					::DestroyWindow(m_hWnd);
				}
			}
			if (MenuItem("Save")) {
			}
			Separator();
			if (MenuItem("Exit")) {
				::DestroyWindow(m_hWnd);
			}
			ImGui::EndMenu();
		}
		if (BeginMenu("View")) {
			if (MenuItem("Processes", "F4", m_ProcessesView.IsOpen(), !m_ProcessesView.IsOpen())) {
				m_ProcessesView.Open();
			}
			if (MenuItem("Threads", nullptr, m_ThreadsView.IsOpen(), !m_ThreadsView.IsOpen())) {
				m_ThreadsView.Open();
			}
			ImGui::EndMenu();
		}
		if (BeginMenu("Options")) {
			if (MenuItem("Always on Top", nullptr, IsAlwaysOnTop())) {
				ToggleAlwaysOnTop();
			}
			if (BeginMenu("Theme")) {
				if (MenuItem("Dark", nullptr, m_Dark)) {
					StyleColorsDark();
					m_Dark = true;
				}
				if (MenuItem("Light", nullptr, !m_Dark)) {
					ImGui::StyleColorsLight();
					m_Dark = false;
				}
				Separator();
				if (MenuItem("As System")) {
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenu();
		}
		if (BeginMenu("Help")) {
			if (MenuItem("About Process Manager...")) {
			}
			ImGui::EndMenu();
		}
		PopFont();
		EndMainMenuBar();
	}

	m_ProcessesView.BuildWindow();
	m_ThreadsView.BuildWindow();
}

bool MainWindow::IsAlwaysOnTop() const {
	return (::GetWindowLongPtr(m_hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

bool MainWindow::ToggleAlwaysOnTop() {
	auto onTop = !IsAlwaysOnTop();
	::SetWindowPos(m_hWnd, onTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	return onTop;
}


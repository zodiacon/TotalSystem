#include "pch.h"
#include "MainWindow.h"
#include "SecurityHelper.h"

using namespace ImGui;

MainWindow::MainWindow(HWND hWnd) : m_hWnd(hWnd) {
}

void MainWindow::BuildWindow() {
	if (BeginMainMenuBar()) {
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
		if (BeginMenu("Options")) {
			if (BeginMenu("Theme")) {
				if (MenuItem("Dark")) {
					StyleColorsDark();
				}
				if (MenuItem("Light")) {
					ImGui::StyleColorsLight();
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
		EndMainMenuBar();
	}
}

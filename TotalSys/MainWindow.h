#pragma once

#include "ProcessesView.h"

class MainWindow {
public:
	explicit MainWindow(HWND hWnd);
	void BuildWindow();
	bool IsAlwaysOnTop() const;
	bool ToggleAlwaysOnTop();
	bool SaveSelected() const;
	bool HandleMessage(UINT msg, WPARAM wp, LPARAM lp);

	static void SetTheme();

private:
	HWND m_hWnd;
	ProcessesView m_ProcessesView;
	ThreadsView m_ThreadsView;
	HandlesView m_HandlesView;
	DWORD64 m_LastCount{ 0 };
	bool m_DoSave{ false };
};


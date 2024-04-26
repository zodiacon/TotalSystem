#pragma once

#include "ProcessesView.h"

class MainWindow {
public:
	explicit MainWindow(HWND hWnd);
	void BuildWindow();
	bool IsAlwaysOnTop() const;
	bool ToggleAlwaysOnTop();
	bool SaveSelected() const;

private:
	HWND m_hWnd;
	ProcessesView m_ProcessesView;
	ThreadsView m_ThreadsView{ true };
	DWORD64 m_LastCount{ 0 };
	bool m_DoSave{ false };
};


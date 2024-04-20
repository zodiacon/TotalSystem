#pragma once

#include "ProcessesView.h"

class MainWindow {
public:
	explicit MainWindow(HWND hWnd);
	void BuildWindow();
	bool IsAlwaysOnTop() const;
	bool ToggleAlwaysOnTop();

private:
	HWND m_hWnd;
	ProcessesView m_ProcessesView;
	ThreadsView m_ThreadsView{ true };
	bool m_Dark{ true };
};


#pragma once

#include "ProcessesView.h"

class MainWindow {
public:
	explicit MainWindow(HWND hWnd);
	void BuildWindow();

private:
	HWND m_hWnd;
	ProcessesView m_ProcessesView;
};


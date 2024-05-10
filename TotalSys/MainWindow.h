#pragma once

#include "ProcessesView.h"
#include "Window.h"


class MainWindow : public Window {
public:
	explicit MainWindow(HWND hWnd);
	void Build() override;
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
	std::vector<std::unique_ptr<Window>> m_Windows;
	bool m_DoSave{ false };
};


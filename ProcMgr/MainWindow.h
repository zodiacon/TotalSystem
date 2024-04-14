#pragma once
class MainWindow {
public:
	explicit MainWindow(HWND hWnd);
	void BuildWindow();

private:
	HWND m_hWnd;
};


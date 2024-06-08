#pragma once

#include "ProcessInfoEx.h"
#include "ThreadInfoEx.h"
#include <ProcessManager.h>
#include "IconManager.h"

struct TotalSysSettings;
class MainWindow;

struct Globals abstract final {
	static void SetMonoFont(ImFont* font);
	static void SetVarFont(ImFont* font);
	static ImFont* MonoFont();
	static ImFont* VarFont();
	static bool IsDarkMode();
	static void SetDarkMode(bool dark);
	static IconManager& ImageManager();
	static TotalSysSettings& Settings();
	static MainWindow& RootWindow();
	static void SetMainWindow(MainWindow* win);
	static void SetAsSystem(bool dark);

private:
	inline static ImFont* s_pMonoFont;
	inline static ImFont* s_pVarFont;
	inline static MainWindow* s_MainWindow;
};


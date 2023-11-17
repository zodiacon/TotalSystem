#include "pch.h"
#include "MainWindow.h"

#pragma comment(lib, "ntdll")


int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	QApplication a(__argc, __argv);

	WinLL::SecurityHelper::EnablePrivilege(SE_DEBUG_NAME);
	WinLL::SecurityHelper::EnablePrivilege(SE_TCB_NAME);
	MainWindow w;
	w.show();
	return a.exec();
}

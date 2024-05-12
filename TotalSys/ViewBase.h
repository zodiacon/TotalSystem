#pragma once

#include <ProcessManager.h>
#include "Window.h"

class ProcessInfoEx;
class ThreadInfoEx;

using DefaultProcessManager = WinLL::ProcessManager<ProcessInfoEx, ThreadInfoEx>;

class ViewBase abstract : public Window {
public:
	bool IsRunning() const;
	virtual void TogglePause();
	int GetUpdateInterval() const;
	void SetUpdateInterval(int interval);
	virtual bool NeedUpdate() const;
	virtual void UpdateTick();
	bool BuildUpdateIntervalMenu();
	bool BuildUpdateIntervalToolBar();

private:
	int m_UpdateInterval{ 1000 }, m_OldInterval{ 0 };
	DWORD64 m_Tick{ 0 };
	bool m_Paused : 1 { false };
};


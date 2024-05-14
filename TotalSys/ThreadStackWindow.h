#pragma once

#include "Window.h"
#include <DbgHelp.h>
#include "ThreadInfoEx.h"

class ProcessInfoEx;

class ThreadStackWindow : public Window {
public:
	ThreadStackWindow(std::shared_ptr<ProcessInfoEx> p, std::shared_ptr<WinLL::ThreadInfo> t, std::vector<STACKFRAME64> const& frames);

	void Build() override;
	void BuildToolBar();

private:
	struct StackFrame {
		uint64_t Address;
	};

	std::shared_ptr<WinLL::ThreadInfo> m_Thread; 
	std::shared_ptr<ProcessInfoEx> m_Process;
	std::vector<StackFrame> m_Frames;
	std::string m_Title;
	int m_SelectedIndex{ -1 };
};


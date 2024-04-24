#pragma once

#include "ViewBase.h"
#include <ProcessModuleTracker.h>

class ProcessInfoEx;

class ModulesView : public ViewBase {
public:
	bool Track(uint32_t pid);
	void BuildTable(std::shared_ptr<ProcessInfoEx> p);

private:
	WinLLX::ProcessModuleTracker m_Tracker;
};


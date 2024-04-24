#include "pch.h"
#include "ModulesView.h"
#include "ProcessInfoEx.h"

bool ModulesView::Track(uint32_t pid) {
	return m_Tracker.TrackProcess(pid);
}

void ModulesView::BuildTable(std::shared_ptr<ProcessInfoEx> p) {
	if (m_Tracker.GetPid() == p->Id) {
	}
}

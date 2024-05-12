#include "pch.h"
#include "ViewBase.h"

using namespace ImGui;

bool ViewBase::IsRunning() const {
	return m_UpdateInterval > 0;
}

void ViewBase::TogglePause() {
	if (m_UpdateInterval == 0) {
		m_UpdateInterval = m_OldInterval;
		m_OldInterval = 0;
	}
	else {
		m_OldInterval = m_UpdateInterval;
		m_UpdateInterval = 0;
	}
}

int ViewBase::GetUpdateInterval() const {
	return m_UpdateInterval;
}

void ViewBase::SetUpdateInterval(int interval) {
	m_UpdateInterval = interval;
}

bool ViewBase::NeedUpdate() const {
	return m_UpdateInterval > 0 && ::GetTickCount64() - m_Tick >= m_UpdateInterval;
}

void ViewBase::UpdateTick() {
	m_Tick = ::GetTickCount64();
}

bool ViewBase::BuildUpdateIntervalMenu() {
	if (BeginMenu("Update Interval")) {
		auto interval = GetUpdateInterval(), old = interval;
		if (MenuItem("500 ms", nullptr, interval == 500))
			interval = 500;
		if (MenuItem("1 second", nullptr, interval == 1000))
			interval = 1000;
		if (MenuItem("2 seconds", nullptr, interval == 2000))
			interval = 2000;
		if (MenuItem("5 seconds", nullptr, interval == 5000))
			interval = 5000;
		if (interval != old) {
			SetUpdateInterval(interval);
			return true;
		}
		Separator();
		if (MenuItem("Paused", "SPACE", !IsRunning())) {
			TogglePause();
		}
		ImGui::EndMenu();
	}
	return false;
}

bool ViewBase::BuildUpdateIntervalToolBar() {
	static const struct {
		const char* Text;
		int Interval;
	} intervals[] = {
		{ "500 msec", 500 },
		{ "1 Second", 1000 },
		{ "2 Seconds", 2000 },
		{ "5 Seconds", 5000 },
		{ "Paused", 0 },
	};
	int current = 0;
	for (int i = 0; i < _countof(intervals); i++) {
		if (intervals[i].Interval == GetUpdateInterval()) {
			current = i;
			break;
		}
	}
	SetNextItemWidth(100);
	
	if (BeginCombo("Update", intervals[current].Text, ImGuiComboFlags_None)) {
		for (auto& item : intervals) {
			if (item.Interval == 0)
				break;
			if (MenuItem(item.Text, nullptr, GetUpdateInterval() == item.Interval)) {
				SetUpdateInterval(item.Interval);
				EndCombo();
				return true;
			}
		}
		Separator();
		if (MenuItem("Paused", "SPACE", !IsRunning())) {
			TogglePause();
		}
		EndCombo();
	}
	return false;
}

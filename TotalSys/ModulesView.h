#pragma once

#include "ViewBase.h"
#include <ProcessModuleTracker.h>
#include <functional>
#include "SortedFilteredVector.h"
#include "TransientObject.h"

struct ModuleInfoEx : WinLLX::ModuleInfo, TransientObject {
};

class ModulesView : public ViewBase {
public:
	bool Track(uint32_t pid);
	void BuildTable();
	bool Refresh(bool now = false);

private:
	enum class Column {
		Name, Type, Size, BaseAddress, ImageBase, Path, Characteristics,
	};

	struct ColumnInfo {
		PCSTR Header;
		std::function<void(std::shared_ptr<ModuleInfoEx>&)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
	};

	WinLLX::ProcessModuleTracker<ModuleInfoEx> m_Tracker;
	std::shared_ptr<ModuleInfoEx> m_SelectedModule;
	SortedFilteredVector<std::shared_ptr<ModuleInfoEx>> m_Modules;
};


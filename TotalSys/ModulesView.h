#pragma once

#include "ViewBase.h"
#include <ProcessModuleTracker.h>
#include <functional>
#include "SortedFilteredVector.h"
#include "TransientObject.h"
#include <d3d11Image.h>

struct ModuleInfoEx : WinLLX::ModuleInfo, TransientObject {
};

class ModulesView : public ViewBase {
public:
	bool Track(uint32_t pid);
	void BuildTable();
	bool Refresh(uint32_t pid, bool now = false);
	static void Init();
	void Build() override;

private:
	void DoSort(int col, bool asc);

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
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	inline static D3D11Image s_Icons[2];
};


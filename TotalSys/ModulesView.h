#pragma once

#include "ViewBase.h"
#include <ProcessModuleTracker.h>
#include <KernelModuleTracker.h>
#include <functional>
#include "SortedFilteredVector.h"
#include "TransientObject.h"
#include <d3d11Image.h>

struct ModuleInfoEx : WinLLX::KernelModuleInfo, TransientObject {
	std::wstring const& GetDescription() const;
	std::wstring const& GetCompanyName() const;

private:
	mutable std::wstring m_Desc, m_Company;

};

class ModulesView : public ViewBase {
public:
	void InitColumns();
	bool Track(uint32_t pid);
	void BuildTable();
	bool Refresh(uint32_t pid, bool now = false);
	static void Init();
	void Build() override;
	bool IsKernel() const;

private:
	void DoSort(int col, bool asc);

	enum class Column {
		Name, Type, Size, BaseAddress, ImageBase, Path, Characteristics,
		Description, Company,
	};

	struct ColumnInfo {
		Column Type;
		PCSTR Header;
		std::function<void(std::shared_ptr<ModuleInfoEx>&)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
	};
	std::vector<ColumnInfo> m_Columns;
	WinLLX::ProcessModuleTracker<ModuleInfoEx> m_Tracker;
	WinLLX::KernelModuleTracker<ModuleInfoEx> m_KernelTracker;
	std::shared_ptr<ModuleInfoEx> m_SelectedModule;
	SortedFilteredVector<std::shared_ptr<ModuleInfoEx>> m_Modules;
	WinLLX::ModuleTrackerBase<ModuleInfoEx>* m_TheTracker{ nullptr };
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	uint32_t m_Pid{ 0 };
	bool m_KernelModules{ false };
	inline static D3D11Image s_Icons[3];
};


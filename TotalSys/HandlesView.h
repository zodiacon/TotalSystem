#pragma once

#include "ViewBase.h"
#include "TransientObject.h"
#include <ProcessHandleTracker.h>
#include "SortedFilteredVector.h"
#include <ProcessManager.h>
#include <d3d11Image.h>
#include <WinLowLevel.h>

struct HandleInfoEx : WinLLX::HandleInfo, TransientObject {
	std::wstring Type;
	std::wstring ProcessName;
	bool NameChecked : 1{ false }, ProcessNameChecked: 1{ false };
};

class HandlesView : public ViewBase {
public:
	static void Init();
	bool Track(uint32_t pid, PCWSTR type = L"");
	void BuildTable();
	void BuildWindow();
	void BuildToolBar();
	bool Refresh(uint32_t pid, bool now = false);
	std::wstring const& GetObjectName(HandleInfoEx* hi) const;
	std::wstring const& GetObjectType(HandleInfoEx* hi) const;
	std::wstring const& GetProcessName(HandleInfoEx* hi) const;

private:
	void DoSort(int col, bool asc);

	enum class Column {
		Handle, Type, Name, PID, ProcessName, Access, Address, Attributes, DecodedAccess, Details,
	};

	struct ColumnInfo {
		PCSTR Header;
		std::function<void(std::shared_ptr<HandleInfoEx>&)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
	};

private:
	WinLLX::ProcessHandleTracker<HandleInfoEx> m_Tracker;
	std::shared_ptr<HandleInfoEx> m_SelectedHandle;
	SortedFilteredVector<std::shared_ptr<HandleInfoEx>> m_Handles;
	std::function<bool(std::shared_ptr<HandleInfoEx> const&, size_t)> m_Filter{ nullptr };
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	WinLL::ProcessManager<> m_ProcMgr;
	bool m_Updating{ false };
	bool m_NamedObjects{ true };
	bool m_UpdateNow{ false };
	inline static std::unordered_map<std::wstring, D3D11Image> s_Icons;
};


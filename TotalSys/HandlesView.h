#pragma once

#include "ViewBase.h"
#include "TransientObject.h"
#include <ProcessHandleTracker.h>
#include "SortedFilteredVector.h"

struct HandleInfoEx : WinLLX::HandleInfo, TransientObject {
	std::wstring Type;
	std::wstring ProcessName;
	bool NameChecked : 1{ false }, ProcessNameChecked: 1{ false };
};

class HandlesView : public ViewBase {
public:
	bool Track(uint32_t pid, PCWSTR type = L"");
	void BuildTable();
	void BuildWindow();
	bool Refresh(uint32_t pid, bool now = false);
	std::wstring const& GetObjectName(HandleInfoEx* hi) const;
	std::wstring const& GetObjectType(HandleInfoEx* hi) const;

private:
	void DoSort(int col, bool asc);

	enum class Column {
		Handle, Type, Name, Access, Address, Attributes, DecodedAccess, PID, ProcessName,
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
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	bool m_Updating{ false };
};


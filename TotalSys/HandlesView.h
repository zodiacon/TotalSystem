#pragma once

#include "ViewBase.h"
#include "TransientObject.h"
#include <ProcessHandleTracker.h>
#include "SortedFilteredVector.h"
#include <ProcessManager.h>
#include <d3d11Image.h>
#include <SimpleMessageBox.h>

struct HandleInfoEx : WinLLX::HandleInfo, TransientObject {
	std::wstring Type;
	std::wstring ProcessName;
	bool NameChecked : 1{ false }, ProcessNameChecked: 1{ false };
};

class HandlesView : public ViewBase {
public:
	static void Init();
	HandlesView();
	~HandlesView();
	void InitColumns();

	bool Track(uint32_t pid, PCWSTR type = L"");
	void BuildTable() noexcept;
	void Build() override;
	void BuildToolBar() noexcept;
	bool Refresh(uint32_t pid, bool now = false) noexcept;
	std::wstring const& GetObjectName(HandleInfoEx* hi) const noexcept;
	std::wstring const& GetObjectType(HandleInfoEx* hi) const noexcept;
	std::wstring const& GetProcessName(HandleInfoEx* hi) const noexcept;

private:
	void DoSort(int col, bool asc) noexcept;
	void PromptCloseHandle(HandleInfoEx* hi) const;
	std::string DoCopy(HandleInfoEx* hi, int c) const noexcept;
	bool DoContextMenu(HandleInfoEx* hi, int c) const noexcept;
	bool Filter(std::shared_ptr<HandleInfoEx> const& h) const;

	enum class Column {
		Handle, Type, Name, PID, ProcessName, Access, Address, Attributes, DecodedAccess, Details,
		_Count
	};

	struct ColumnInfo {
		Column Type;
		PCSTR Header;
		std::function<void(std::shared_ptr<HandleInfoEx>&)> Callback{ };
		ImGuiTableColumnFlags Flags{ ImGuiTableColumnFlags_None };
		float Width{ 0.0f };
	};

private:
	WinLLX::ProcessHandleTracker<HandleInfoEx> m_Tracker;
	std::vector<ColumnInfo> m_Columns;
	std::shared_ptr<HandleInfoEx> m_SelectedHandle;
	SortedFilteredVector<std::shared_ptr<HandleInfoEx>> m_Handles;
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	WinLL::ProcessManager<> m_ProcMgr;
	mutable SimpleMessageBox m_ModalBox;
	char m_FilterText[32]{};
	int m_HoveredColumn{ -1 };
	bool m_Updating: 1{ false }, m_AllHandles: 1{ false };
	bool m_UpdateNow : 1{ false }, m_FilterChanged : 1 { false };;
	bool m_NamedObjects { true };
	inline static std::unordered_map<std::wstring, D3D11Image> s_Icons;
};


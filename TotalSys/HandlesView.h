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
	void PromptCloseHandle(HandleInfoEx* hi);
	void DoCopy(HandleInfoEx* hi, int c);
	bool DoContextMenu(HandleInfoEx* hi, int c);
	bool Filter(std::shared_ptr<HandleInfoEx> const& h) const;

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
	ImGuiTableSortSpecs* m_Specs{ nullptr };
	WinLL::ProcessManager<> m_ProcMgr;
	SimpleMessageBox m_ModalBox;
	char m_FilterText[32]{};
	int m_HoveredColumn{ -1 };
	bool m_Updating: 1{ false }, m_AllHandles: 1{ false };
	bool m_UpdateNow : 1{ false }, m_FilterChanged : 1 { false };;
	bool m_NamedObjects { true };
	inline static std::unordered_map<std::wstring, D3D11Image> s_Icons;
};


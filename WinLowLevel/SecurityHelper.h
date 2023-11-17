#pragma once

namespace WinLL {
	struct SecurityHelper abstract final {
		[[nodiscard]] static bool IsRunningElevated();
		static bool RunElevated(PCWSTR param = nullptr, bool ui = true);
		static bool EnablePrivilege(PCWSTR privName, bool enable = true);
		[[nodiscard]] static std::wstring GetSidFromUser(PCWSTR name);
	};
}

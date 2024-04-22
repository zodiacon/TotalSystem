#pragma once

namespace WinLL {
	struct FileHelper abstract final {
		static std::wstring GetDosNameFromNtName(PCWSTR name);
	};
}

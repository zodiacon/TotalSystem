#pragma once

struct FormatHelpers abstract final {
	static std::string FormatDateTime(int64_t ft);
	static std::string FormatTimeSpan(int64_t ts);
	static std::wstring FormatNumber(int64_t number);
};


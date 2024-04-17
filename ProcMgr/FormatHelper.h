#pragma once

struct FormatHelper abstract final {
	static std::string FormatDateTime(int64_t ft);
	static std::string FormatTimeSpan(int64_t ts);
	static std::wstring FormatNumber(int64_t number);
	static ImVec4 ColorWithAlpha(const ImVec4& color, float alpha);
};


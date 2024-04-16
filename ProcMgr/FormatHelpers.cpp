#include "pch.h"
#include "FormatHelpers.h"
#include <shellapi.h>

std::string FormatHelpers::FormatDateTime(int64_t time) {
	TIME_FIELDS tf;
	RtlTimeToTimeFields((PLARGE_INTEGER)&time, &tf);
	return std::format("{:04}/{:02}/{:02} {:02}:{:02}:{:02}.{:03}",
		tf.Year, tf.Month, tf.Day, tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

std::string FormatHelpers::FormatTimeSpan(int64_t ts) {
	TIME_FIELDS tf;
	RtlTimeToElapsedTimeFields((PLARGE_INTEGER)&ts, &tf);
	return std::format("{:2}.{:02}:{:02}:{:02}.{:03}", tf.Day, tf.Hour, tf.Minute, tf.Second, tf.Milliseconds);
}

std::wstring FormatHelpers::FormatNumber(int64_t number) {
	static WCHAR sep[4], group[8];
	if (sep[0] == 0) {
		::GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_STHOUSAND, sep, _countof(sep));
		::GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SGROUPING, group, _countof(group));
	}
	static const NUMBERFMT fmt{ 0, 0, (UINT)_wtoi(group), (PWSTR)L".", sep, 0 };
	WCHAR result[32];
	int r = ::GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, std::to_wstring(number).c_str(), &fmt, result, _countof(result));
	return result;
}

ImVec4 FormatHelpers::ColorWithAlpha(const ImVec4& color, float alpha) {
	return ImVec4(color.x, color.y, color.z, alpha);
}

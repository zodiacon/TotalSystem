#pragma once

struct FormatHelper abstract final {
    template<typename T>
    static std::wstring FormatNumber(T const& number, int decimal = 0) {
        WCHAR buffer[64];
        int chars = ::GetNumberFormatEx(LOCALE_NAME_USER_DEFAULT, 0, std::to_wstring(number).c_str(), nullptr, buffer, _countof(buffer));
        if (chars == 0)
            return std::to_wstring(number);
        *(wcsrchr(buffer, L'.') + decimal + (decimal > 0 ? 1 : 0)) = 0;
        return buffer;
    }

    static std::wstring FormatDateTime(int64_t time) {
        SYSTEMTIME st;
        ::FileTimeToLocalFileTime((FILETIME*)&time, (FILETIME*)&time);
        ::FileTimeToSystemTime((FILETIME*)&time, &st);
        WCHAR buffer[32], buffer2[32];
        ::GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &st, nullptr, buffer, _countof(buffer), nullptr);
        ::GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &st, nullptr, buffer2, _countof(buffer2));
        return std::wstring(buffer) + L" " + buffer2;
    }
};


#include "pch.h"
#include "IniFile.h"
#include <sstream>

using namespace std;

IniFile::IniFile(PCWSTR path) : m_Path(path) {
}

bool IniFile::IsValid() const {
    return ::GetFileAttributes(m_Path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

wstring IniFile::ReadString(PCWSTR section, PCWSTR name, PCWSTR defaultValue) {
    wstring result;
    result.resize(128);
    auto count = ::GetPrivateProfileString(section, name, defaultValue, result.data(), (DWORD)result.length(), m_Path.c_str());
    result.resize(count);
    return result;
}

bool IniFile::ReadBool(PCWSTR section, PCWSTR name, bool& value) {
    int ivalue;
    if (ReadInt(section, name, ivalue)) {
        value = ivalue ? true : false;
        return true;
    }
    return false;
}

bool IniFile::ReadInt(PCWSTR section, PCWSTR name, int& value) {
    value = ::GetPrivateProfileInt(section, name, INT_MIN, m_Path.c_str());
    return value != INT_MIN;
}

COLORREF IniFile::ReadColor(PCWSTR section, PCWSTR name, COLORREF defaultValue) {
    auto text = ReadString(section, name);
    if (text.empty())
        return defaultValue;

    if (text.substr(0, 2) == L"0x")
        return ParseHexColor(text.substr(2));

    if (text.find(L',') != wstring::npos)
        return ParseDecColor(text);

    return ParseHexColor(text);
}

COLORREF IniFile::ParseHexColor(const wstring& hex) {
    std::wstringstream ss;
    DWORD color;
    ss << std::hex << hex;
    ss >> color;
    return color;
}

COLORREF IniFile::ParseDecColor(const std::wstring& text) {
    std::vector<BYTE> values;
    size_t start = 0, i;
    while ((i = text.find(L',', start)) != wstring::npos) {
        values.push_back((BYTE)_wtoi(text.substr(start, i - start).c_str()));
        start = i + 1;
    }
    if(values.size() != 3)
        return CLR_INVALID;

    return RGB(values[0], values[1], values[2]);
}

std::vector<wstring> IniFile::ReadSection(PCWSTR section) {
    WCHAR buffer[1 << 10];
    std::vector<wstring> names;
    if (0 == ::GetPrivateProfileSection(section, buffer, _countof(buffer), m_Path.c_str()))
        return names;

    names.reserve(8);
    for (auto p = buffer; *p; ) {
        names.push_back(p);
        p += names.back().length() + 1;
    }
    return names;
}

bool IniFile::WriteString(PCWSTR section, PCWSTR name, PCWSTR value) {
    return ::WritePrivateProfileString(section, name, value, m_Path.c_str());
}

bool IniFile::WriteInt(PCWSTR section, PCWSTR name, int value, bool hex) {
    auto text = hex ? format(L"0x{:X}", value) : format(L"{}", value);
    return WriteString(section, name, text.c_str());
}

bool IniFile::WriteBool(PCWSTR section, PCWSTR name, bool value) {
    return WriteInt(section, name, value ? 1 : 0);
}

bool IniFile::WriteColor(PCWSTR section, PCWSTR name, COLORREF color) {
    auto text = format(L"{},{},{}", GetRValue(color), GetGValue(color), GetBValue(color));
    return WriteString(section, name, text.c_str());
}

bool IniFile::WriteFont(PCWSTR section, PCWSTR name, const LOGFONT& font) {
    return ::WritePrivateProfileStruct(section, name, (PVOID)&font, sizeof(font), m_Path.c_str());
}

bool IniFile::WriteBinary(PCWSTR section, PCWSTR name, void* data, unsigned size) {
    WriteInt(section, (name + wstring(L"_size")).c_str(), size);
    return ::WritePrivateProfileStruct(section, name, data, size, m_Path.c_str());
}

bool IniFile::ReadFont(PCWSTR section, PCWSTR name, LOGFONT& font) {
    return ::GetPrivateProfileStruct(section, name, &font, sizeof(font), m_Path.c_str());
}

std::unique_ptr<uint8_t[]> IniFile::ReadBinary(PCWSTR section, PCWSTR name, int& size) {
    if (!ReadInt(section, (name + wstring(L"_size")).c_str(), size) || size == 0)
        return nullptr;

    auto buffer = std::make_unique<uint8_t[]>(size);
    if (buffer == nullptr)
        return nullptr;
    ::GetPrivateProfileStruct(section, name, buffer.get(), size, m_Path.c_str());
    return buffer;
}


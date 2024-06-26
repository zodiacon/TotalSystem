#pragma once

#include <vector>
#include <string>

class IniFile {
public:
	explicit IniFile(PCWSTR path);

	bool IsValid() const;

	std::wstring ReadString(PCWSTR section, PCWSTR name, PCWSTR defaultValue = nullptr);
	bool ReadInt(PCWSTR section, PCWSTR name, int& value);
	COLORREF ReadColor(PCWSTR section, PCWSTR name, COLORREF defaultValue = CLR_INVALID);
	std::vector<std::wstring> ReadSection(PCWSTR section);
	bool ReadBool(PCWSTR section, PCWSTR name, bool& valye);
	bool ReadFont(PCWSTR section, PCWSTR name, LOGFONT& font);
	std::unique_ptr<uint8_t[]> ReadBinary(PCWSTR section, PCWSTR name, int& size);

	bool WriteString(PCWSTR section, PCWSTR name, PCWSTR value);
	bool WriteInt(PCWSTR section, PCWSTR name, int value, bool hex = false);
	bool WriteBool(PCWSTR section, PCWSTR name, bool value);
	bool WriteColor(PCWSTR section, PCWSTR name, COLORREF color);
	bool WriteFont(PCWSTR section, PCWSTR name, const LOGFONT& font);
	bool WriteBinary(PCWSTR section, PCWSTR name, void* data, unsigned size);

protected:
	COLORREF ParseHexColor(const std::wstring& hex);
	COLORREF ParseDecColor(const std::wstring& text);

private:
	std::wstring m_Path;
};


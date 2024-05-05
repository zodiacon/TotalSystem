#include "pch.h"
#include "SettingsBase.h"
#include "IniFile.h"
#include <WinLowLevel.h>

using namespace std;
using namespace WinLL;

bool SettingsBase::LoadFromKey(PCWSTR registryPath) {
	if (registryPath == nullptr)
		registryPath = m_path.c_str();
	else
		m_path = registryPath;
	assert(registryPath);
	if (registryPath == nullptr)
		return false;

	RegistryKey key;
	if (ERROR_SUCCESS != key.Open(HKEY_CURRENT_USER, registryPath))
		return false;

	WCHAR name[128];
	auto size = 1 << 16;
	auto value = std::make_unique<BYTE[]>(size);
	DWORD type;
	for (DWORD i = 0;; ++i) {
		DWORD lname = _countof(name), lvalue = size;
		auto error = ::RegEnumValue((HKEY)key.Handle(), i, name, &lname, nullptr, &type, value.get(), &lvalue);
		if (ERROR_NO_MORE_ITEMS == error)
			break;

		if (error != ERROR_SUCCESS)
			continue;

		auto it = m_settings.find(name);
		if (it == m_settings.end())
			m_settings.insert({ name, Setting(name, value.get(), lvalue, (SettingType)type) });
		else
			it->second.Set(value.get(), lvalue);
	}
	return true;
}

bool SettingsBase::SaveToKey(PCWSTR registryPath) const {
	if (registryPath == nullptr)
		registryPath = m_path.c_str();

	assert(registryPath);
	if (registryPath == nullptr)
		return false;

	RegistryKey key;
	key.Create(HKEY_CURRENT_USER, registryPath, KEY_WRITE);
	if (!key)
		return false;

	for (auto& [name, setting] : m_settings) {
		key.SetValue(name.c_str(), (DWORD)setting.Type, setting.Buffer.get(), setting.Size);
	}
	return true;
}

bool SettingsBase::LoadFromFile(PCWSTR path) {
	if (path == nullptr)
		path = m_path.c_str();

	assert(path);
	if (path == nullptr)
		return false;
	else
		m_path = path;

	IniFile file(path);
	if (!file.IsValid())
		return false;

	PCWSTR section = L"General";
	for (auto& [name, setting] : m_settings) {
		switch (setting.Type) {
			case SettingType::String:
				setting.SetString(file.ReadString(section, name.c_str()).c_str());
				break;

			case SettingType::Int32:
				int value;
				if(file.ReadInt(section, name.c_str(), value))
					setting.Set<int>(value);
				break;

			default:
				int size;
				auto data = file.ReadBinary(section, name.c_str(), size);
				if (data && size > 0)
					setting.Set(data.get(), size);
				break;
		}
	}

	return LoadCustom(file);
}

bool SettingsBase::SaveToFile(PCWSTR path) const {
	if (path == nullptr)
		path = m_path.c_str();

	assert(path);
	if (path == nullptr)
		return false;

	IniFile file(path);

	PCWSTR section = L"General";
	for (auto& [name, setting] : m_settings) {
		switch (setting.Type) {
			case SettingType::String:
				file.WriteString(section, name.c_str(), (PCWSTR)setting.Buffer.get());
				break;

			case SettingType::Int32:
				file.WriteInt(section, name.c_str(), *(DWORD*)setting.Buffer.get());
				break;

			default:
				file.WriteBinary(section, name.c_str(), setting.Buffer.get(), setting.Size);
				break;
		}
	}
	return SaveCustom(file);
}

bool SettingsBase::Load(PCWSTR path) {
	WCHAR fullpath[MAX_PATH];
	::GetModuleFileName(nullptr, fullpath, _countof(fullpath));
	auto dot = wcsrchr(fullpath, L'.');
	assert(dot);
	if (!dot)
		return false;

	*dot = 0;
	wcscat_s(fullpath, L".ini");

	if (::GetFileAttributes(fullpath) == INVALID_FILE_ATTRIBUTES) {
		//
		// ini file does not exist, use Registry
		//
		return LoadFromKey(path);
	}
	return LoadFromFile(fullpath);
}

bool SettingsBase::Save() const {
	if (m_path.empty())
		return false;

	return m_path[1] == L':' ? SaveToFile() : SaveToKey();
}

void SettingsBase::Set(PCWSTR name, int value) {
	return Set(name, value, SettingType::Int32);
}

void SettingsBase::Set(PCWSTR name, std::vector<std::wstring> const& values) {
	Setting s(name, values);
	m_settings.erase(name);
	m_settings.insert({ name, std::move(s) });
}

void SettingsBase::SetString(PCWSTR name, PCWSTR value) {
	auto it = m_settings.find(name);
	if (it != m_settings.end()) {
		it->second.SetString(value);
	}
	else {
		Setting s(name, value);
		m_settings.insert({ name, std::move(s) });
	}
}

bool SettingsBase::SaveWindowPosition(HWND hWnd, PCWSTR name) {
	WINDOWPLACEMENT wp = { sizeof(wp) };
	if(!::GetWindowPlacement(hWnd, &wp))
		return false;

	Set(name, wp);
	return true;
}

bool SettingsBase::LoadWindowPosition(HWND hWnd, PCWSTR name) const {
	const auto wp = GetBinary<WINDOWPLACEMENT>(name);
	if (wp == nullptr)
		return false;

	::SetWindowPlacement(hWnd, wp);
	return true;
}

std::wstring SettingsBase::GetString(PCWSTR name) const {
	auto it = m_settings.find(name);
	if (it == m_settings.end())
		return L"";
	return (PCWSTR)it->second.Buffer.get();
}

int SettingsBase::GetInt32(PCWSTR name) const {
	return GetValue<int>(name);
}

void Setting::SetString(PCWSTR value) {
	Buffer = std::make_unique<uint8_t[]>(Size = (1 + (int)::wcslen(value)) * sizeof(wchar_t));
	::memcpy(Buffer.get(), value, Size);
	Type = SettingType::String;
}

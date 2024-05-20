#pragma once

#include <map>
#include <string>
#include <algorithm>
#include <assert.h>

enum class SettingType {
	String = REG_SZ,
	Int32 = REG_DWORD,
	Int64 = REG_QWORD,
	Binary = REG_BINARY,
	Bool = REG_DWORD,
	MultiString = REG_MULTI_SZ,
};

struct Setting {
	std::wstring Name;
	SettingType Type{ SettingType::String };
	std::unique_ptr<uint8_t[]> Buffer;
	uint32_t Size;

	Setting(Setting const&) = delete;
	Setting(Setting&&) = default;
	Setting& operator=(Setting const&) = delete;
	Setting& operator=(Setting&&) = default;

	Setting(std::wstring name, const std::wstring& value) : Name(std::move(name)) {
		Buffer = std::make_unique<uint8_t[]>(Size = (1 + (int)value.size()) * sizeof(wchar_t));
		::memcpy(Buffer.get(), value.data(), Size);
	}
	template<typename T> requires std::is_trivially_copy_assignable_v<T>
	Setting(std::wstring name, const T& value, SettingType type) : Name(std::move(name)), Type(type) {
		Buffer = std::make_unique<uint8_t[]>(Size = (uint32_t)std::max(sizeof(int), sizeof(T)));
		::memset(Buffer.get(), 0, Size);
		::memcpy(Buffer.get(), &value, Size);
	}

	template<typename T> requires std::is_trivially_copy_assignable_v<T>
	Setting(std::wstring name, std::vector<T> const& values) : Name(std::move(name)), Type(SettingType::Binary) {
		Buffer = std::make_unique<uint8_t[]>(Size = uint32_t(sizeof(T) * values.size()));
		::memset(Buffer.get(), 0, Size);
		::memcpy(Buffer.get(), values.data(), Size);
	}

	template<typename T, size_t N> requires std::is_move_assignable_v<T>
	Setting(std::wstring name, std::array<T, N> const& values) : Name(std::move(name)), Type(SettingType::Binary) {
		Buffer = std::make_unique<uint8_t[]>(Size = uint32_t(sizeof(T) * values.size()));
		::memset(Buffer.get(), 0, Size);
		::memcpy(Buffer.get(), values.data(), Size);
	}

	Setting(std::wstring name, void const* value, uint32_t size, SettingType type = SettingType::Binary) : Name(std::move(name)), Type(type) {
		Buffer = std::make_unique<uint8_t[]>(Size = size);
		::memcpy(Buffer.get(), value, Size);
	}

	Setting(std::wstring_view name, std::vector<std::wstring> const& value) : Name(std::move(name)), Type(SettingType::MultiString) {
		size_t size = sizeof(WCHAR);
		std::for_each(value.begin(), value.end(), [&](auto& str) { size += sizeof(WCHAR) * (1 + str.length()); });
		Buffer = std::make_unique<uint8_t[]>(Size = (uint32_t)size);
		auto p = Buffer.get();
		std::for_each(value.begin(), value.end(), [&](auto& str) {
			auto count = (str.length() + 1) * sizeof(WCHAR);
			::memcpy(p, str.c_str(), count);
			p += count;
			});
		p[0] = p[1] = 0;
	}

	template<typename T>
	void Set(const T& value) {
		int size;
		Buffer = std::make_unique<uint8_t[]>(size = (uint32_t)std::max(sizeof(int), sizeof(T)));
		memset(Buffer.get(), 0, size);
		memcpy(Buffer.get(), &value, sizeof(T));
	}

	template<typename T>
	void Set(const std::vector<T>& value) {
		int size;
		Buffer = std::make_unique<uint8_t[]>(size = (uint32_t)value.size() * sizeof(T));
		memcpy(Buffer.get(), value.data(), size);
	}

	void SetString(PCWSTR value);

	void Set(const void* value, int size) {
		Buffer = std::make_unique<uint8_t[]>(Size = size);
		memcpy(Buffer.get(), value, size);
	}
};

#define BEGIN_SETTINGS(className)	\
	void InitSettings() {	\

#define END_SETTINGS } 

#define SETTING_STRING(name, value)	m_Settings.insert({ L#name, Setting(L#name, value) })
#define SETTING(name, value, type)	m_Settings.insert({ L#name, Setting(L#name, value, type) })
#define SETTING_VEC(name, value)	m_Settings.insert({ L#name, Setting(L#name, value) })

#define DEF_SETTING_STRING(name) \
	std::wstring name() const { return GetString(L#name); }	\
	void name(const std::wstring& value) { SetString(L#name, value.c_str()); }

#define DEF_SETTING(name, type) \
	type name() const { return GetValueOrDefault<type>(L#name); }	\
	type* name##Address() { return GetAddress<type>(L#name); }	\
	void name(const type& value) { Set<type>(L#name, value); }

#define DEF_SETTING_REF(name, type) \
	type& name() const { return GetValueRef<type>(L#name); }

#define DEF_SETTING_MULTI(name) \
	std::vector<std::wstring> name() const { return GetMultiString(L#name); }	\
	void name(std::vector<std::wstring> const& value) { Set(L#name, value); }

#define DEF_SETTING_VEC(name, T) \
	std::vector<T> const& name() const { return GetVec<T>(L#name); }	\
	void name(std::vector<T> const& value) { Set(L#name, value); }

class IniFile;

class SettingsBase {
public:
	SettingsBase() = default;

	bool LoadFromKey(PCWSTR registryPath = nullptr);
	bool SaveToKey(PCWSTR registryPath = nullptr) const;
	bool LoadFromFile(PCWSTR path = nullptr);
	bool SaveToFile(PCWSTR path = nullptr) const;
	bool Load(PCWSTR path);
	bool Save() const;
	virtual bool SaveCustom(IniFile& file) const {
		return true;
	}

	virtual bool LoadCustom(IniFile& file) {
		return true;
	}

	template<typename T>
	void Set(const std::wstring& name, const T& value, SettingType type = SettingType::Binary) {
		auto it = m_Settings.find(name);
		if (it != m_Settings.end()) {
			it->second.Set(value);
		}
		else {
			Setting s(name, value, type);
			m_Settings.insert({ name, std::move(s) });
		}
	}

	void Set(PCWSTR name, int value);
	void Set(PCWSTR name, std::vector<std::wstring> const& values);
	
	template<typename T> requires std::is_trivially_copy_assignable_v<T>
	void Set(PCWSTR name, std::vector<T> const& values) {
		Setting s(name, values);
		m_Settings.erase(name);
		m_Settings.insert({ name, std::move(s) });
	}

	void SetString(PCWSTR name, PCWSTR value);
	bool SaveWindowPosition(HWND hWnd, PCWSTR name);
	bool LoadWindowPosition(HWND hWnd, PCWSTR name) const;

	std::wstring GetString(PCWSTR name) const;

	template<typename T>
	T GetValue(PCWSTR name) const {
		auto it = m_Settings.find(name);
		assert(it != m_Settings.end());
		assert(it->second.Size == sizeof(T));
		return *(T*)it->second.Buffer.get();
	}

	template<typename T>
	T* GetAddress(PCWSTR name) {
		auto it = m_Settings.find(name);
		assert(it != m_Settings.end());
		return (T*)it->second.Buffer.get();
	}

	template<typename T>
	T const* GetAddress(PCWSTR name) const {
		auto it = m_Settings.find(name);
		assert(it != m_Settings.end());
		return (T const*)it->second.Buffer.get();
	}

	template<typename T>
	T& GetValueRef(PCWSTR name) const {
		auto it = m_Settings.find(name);
		assert(it != m_Settings.end());
		return *(T*)it->second.Buffer.get();
	}

	template<typename T>
	T GetValueOrDefault(PCWSTR name, const T& def = T()) const {
		auto it = m_Settings.find(name);
		if (it == m_Settings.end())
			return def;
		assert(it->second.Size >= sizeof(T));
		return *(T*)it->second.Buffer.get();
	}

	int GetInt32(PCWSTR name) const;

	std::vector<std::wstring> GetMultiString(PCWSTR name) const;

	template<typename T>
	std::vector<T> GetVec(PCWSTR name) const {
		auto it = m_Settings.find(name);
		if (it == m_Settings.end())
			return {};

		auto p = it->second.Buffer.get();
		std::vector<T> values;
		values.resize(it->second.Size / sizeof(T));
		memcpy(values.data(), p, it->second.Size);
		return values;
	}

	template<typename T>
	const T* GetBinary(PCWSTR name) const {
		auto it = m_Settings.find(name);
		if (it == m_Settings.end())
			return nullptr;
		assert(it->second.Size == sizeof(T));
		return (T*)it->second.Buffer.get();
	}

protected:
	struct LessNoCase {
		bool operator()(const std::wstring& s1, const std::wstring& s2) const {
			return ::_wcsicmp(s1.c_str(), s2.c_str()) < 0;
		}
	};
	std::map<std::wstring, Setting, LessNoCase> m_Settings;
	std::wstring m_path;
};


#pragma once

#include "ObjectManager.h"
#include <unordered_set>
#include <assert.h>


namespace WinLLX {
	struct HandleKey {
		ULONG HandleValue;
		ULONG Pid;
		USHORT TypeIndex;

		bool operator==(HandleKey const& other) const {
			return HandleValue == other.HandleValue && Pid == other.Pid && TypeIndex == other.TypeIndex;
		}
	};
}

template<>
struct ::std::hash<WinLLX::HandleKey> {
	size_t operator()(const WinLLX::HandleKey& key) const {
		return (size_t)key.HandleValue ^ ((size_t)key.TypeIndex << 16) ^ ((size_t)key.Pid << 32);
	}
};

namespace WinLLX {
	template<typename TInfo = HandleInfo>
		requires std::is_base_of_v<HandleInfo, TInfo>
	class ProcessHandleTracker final {
	public:
		bool Track(uint32_t pid, PCWSTR typeName = L"") {
			if (m_Pid == pid && typeName == m_Type)
				return false;

			m_Pid = pid;
			m_Type = typeName;
			m_Handles.clear();
			return true;
		}

		uint32_t GetPid() const {
			return m_Pid;
		}

		ULONG Update(bool clearHistory = false) {
			if (clearHistory)
				m_Handles.clear();

			m_NewHandles.clear();
			m_ClosedHandles.clear();
			if (m_Handles.empty()) {
				m_NewHandles.reserve(512);
				m_ClosedHandles.reserve(32);
			}

			auto handles = ObjectManager::EnumHandles2<TInfo>(m_Type.c_str(), m_Pid, false, true);
			if (m_Handles.empty()) {
				m_Handles.reserve(handles.size());
				m_NewHandles = std::move(handles);
				for (auto& entry : m_NewHandles) {
					HandleKey key = { entry->HandleValue, entry->ProcessId, (uint16_t)entry->ObjectTypeIndex };
					m_Handles.insert({ key, entry });
				}
			}
			else {
				auto oldHandles = m_Handles;
				for (auto& entry : handles) {
					HandleKey key = { entry->HandleValue, entry->ProcessId, (uint16_t)entry->ObjectTypeIndex };
					if (m_Handles.find(key) == m_Handles.end()) {
						// new handle
						m_NewHandles.push_back(entry);
						m_Handles.insert({ key, entry });
					}
					else {
						// existing handle
						oldHandles.erase(key);
					}
				}
				for (auto const& [key, entry] : oldHandles) {
					m_ClosedHandles.push_back(entry);
					m_Handles.erase(key);
				}
			}

			return static_cast<uint32_t>(m_Handles.size());
		}

		const std::vector<std::shared_ptr<TInfo>>& GetHandles() const {
			return m_Handles;
		}

		const std::vector<std::shared_ptr<TInfo>>& GetNewHandles() const {
			return m_NewHandles;
		}

		const std::vector<std::shared_ptr<TInfo>>& GetClosedHandles() const {
			return m_ClosedHandles;
		}

	private:
		ObjectManager m_Mgr;
		DWORD m_Pid{ (DWORD)-1 };
		std::wstring m_Type;
		std::vector<std::shared_ptr<TInfo>> m_ClosedHandles, m_NewHandles;
		std::unordered_map<HandleKey, std::shared_ptr<TInfo>> m_Handles;
	};
}


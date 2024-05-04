#include "pch.h"
#include "WinLowLevel.h"
#include <string_view>

using namespace std;

namespace WinLL {
	Event::Event(EventType type, wstring_view name, bool signaled) {
		m_hObject.reset(::CreateEvent(nullptr, type == EventType::ManualReset, signaled, name.data()));
	}

	Event::Event(EventType type, bool signaled) {
		m_hObject.reset(::CreateEvent(nullptr, type == EventType::ManualReset, signaled, nullptr));
	}

	bool Event::Create(EventType type, bool signaled) {
		m_hObject.reset(::CreateEvent(nullptr, type == EventType::ManualReset, signaled, nullptr));
		return *this;
	}

	bool Event::Create(EventType type, std::wstring_view name, bool signaled) {
		m_hObject.reset(::CreateEvent(nullptr, type == EventType::ManualReset, signaled, name.data()));
		return *this;
	}

	bool Event::Open(EventAccessMask access, wstring_view name, bool inherit) {
		m_hObject.reset(::OpenEvent(static_cast<DWORD>(access), inherit, name.data()));
		return *this;
	}

	bool Event::Set() {
		return ::SetEvent(Handle());
	}

	bool Event::Reset() {
		return ::ResetEvent(Handle());
	}

	bool Event::Pulse() {
		return ::PulseEvent(Handle());
	}
}

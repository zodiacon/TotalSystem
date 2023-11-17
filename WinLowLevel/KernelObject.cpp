#include "pch.h"
#include "WinLowLevel.h"

namespace WinLL {
	KernelObject::KernelObject(HANDLE hObject) : m_hObject(hObject) {}

	void KernelObject::Close() {
		m_hObject.reset();
	}

	HANDLE KernelObject::Handle() const {
		return m_hObject.get();
	}

	KernelObject::operator bool() const {
		return m_hObject != nullptr;
	}

	HANDLE KernelObject::Attach(HANDLE hObject) {
		auto h = m_hObject.release();
		m_hObject.reset(hObject);
		return h;
	}

	HANDLE KernelObject::Detach() {
		return m_hObject.release();
	}

	bool KernelObject::ChangeAccess(AccessMask newAccess) {
		HANDLE hNew;
		if (!NT_SUCCESS(::NtDuplicateObject(NtCurrentProcess(), m_hObject.get(), NtCurrentProcess(), &hNew, newAccess, 0, 0)))
			return false;

		m_hObject.reset(hNew);
		return true;
	}

	bool KernelObject::IsSameObject(KernelObject const& other) const {
		auto const static pCompareObjectHandles = (decltype(&::CompareObjectHandles))::GetProcAddress(::GetModuleHandle(L"kernelbase"), "CompareObjectHandles");
		return pCompareObjectHandles(m_hObject.get(), other.Handle());
	}

	std::wstring KernelObject::GetName() const {
		ULONG len;
		auto status = ::NtQueryObject(m_hObject.get(), ObjectNameInformation, nullptr, 0, &len);
		if (!NT_SUCCESS(status) || len == 0)
			return {};

		std::wstring name;
		name.resize(len / sizeof(WCHAR));
		status = ::NtQueryObject(m_hObject.get(), ObjectNameInformation, name.data(), len, nullptr);
		return NT_SUCCESS(status) ? name : L"";
	}
}
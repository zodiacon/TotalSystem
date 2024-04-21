#include "pch.h"
#include "ThreadInfoEx.h"

using namespace WinLL;

void ThreadInfoEx::Term(uint32_t ms) {
	TransientObject::Term(ms);

	State = ThreadState::Terminated;
	CPU = 0;
}

int ThreadInfoEx::GetMemoryPriority() const {
	Thread t;
	if (t.Open(Id))
		return t.GetMemoryPriority();
	return -1;
}

int ThreadInfoEx::GetSuspendCount() const {
	Thread t;
	if(!t.Open(Id))
		return 0;
	return t.GetSuspendCount();
}

IoPriority ThreadInfoEx::GetIoPriority() const {
	Thread t;
	if (t.Open(Id))
		return t.GetIoPriority();
	return IoPriority::Unknown;
}

std::wstring const& ThreadInfoEx::GetServiceName() const {
	if (m_Service == L" ") {
		Thread t;
		if (t.Open(Id)) {
			m_Service = t.GetServiceNameByTag(ProcessId);
		}
	}
	return m_Service;
}

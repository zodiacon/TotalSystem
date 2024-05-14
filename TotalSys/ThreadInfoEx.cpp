#include "pch.h"
#include "ThreadInfoEx.h"
#include "ProcessInfoEx.h"
#include "ProcessSymbols.h"
#include "UI.h"

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

std::string ThreadInfoEx::GetModuleName(ProcessInfoEx* p, uint64_t baseAddress) {
	auto& symbols = p->GetSymbols();
	auto name = symbols.GetModuleName(baseAddress);
	char text[256];
	sprintf_s(text, "%ws", name.c_str());
	return text;
}

bool ThreadInfoEx::IsSuspended() const {
	Thread t;
	return t.Open(Id) && t.GetSuspendCount() > 0;
}

std::wstring const& ThreadInfoEx::GetDescription() const {
	if (::GetTickCount64() > m_TargetDesc) {
		m_TargetDesc = ::GetTickCount64() + 1000;
		Thread t;
		if (t.Open(Id))
			m_Desc = t.GetDescription();
	}
	return m_Desc;
}

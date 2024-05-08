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

std::string ThreadInfoEx::GetStartAddressSymbol(std::shared_ptr<ProcessInfoEx> p, std::shared_ptr<ThreadInfoEx> t) {
	if (!t->m_AttemptStartAddress) {
		t->m_AttemptStartAddress = true;
		UI::SubmitWorkWithResult([p, t]() -> string* {
			auto& symbols = p->GetSymbols();
			if (symbols) {
				uint64_t disp, modBase;
				const SYMBOL_INFO* sym;
				{
					lock_guard locker(s_Lock);
					sym = symbols.GetSymbolFromAddress((uint64_t)t->StartAddress, &disp, &modBase);
				}
				if (sym) {
					auto result = new string();
					if (disp == 0)
						*result = sym->Name;
					else
						*result = std::format("{}+0x{:X}", sym->Name, disp);
					*result = GetModuleName(p.get(), modBase) + "!" + *result;
					return result;
				}
			}
			return nullptr;
			}, [t](auto result) {
				if (result) {
					auto name = (string*)result;
					t->m_StartAddressName = move(*name);
					delete name;
				}
			});
	}
	return t->m_StartAddressName;
}

std::string ThreadInfoEx::GetWin32StartAddressSymbol(std::shared_ptr<ProcessInfoEx> p, std::shared_ptr<ThreadInfoEx> t) {
	if (!t->m_AttemptWin32StartAddress) {
		t->m_AttemptWin32StartAddress = true;

		UI::SubmitWorkWithResult([p, t]() -> string* {
			auto& symbols = p->GetSymbols();
			if (symbols) {
				uint64_t disp, modBase;
				const SYMBOL_INFO* sym;
				{
					lock_guard locker(s_Lock);
					sym = symbols.GetSymbolFromAddress((uint64_t)t->Win32StartAddress, &disp, &modBase);
				}
				if (sym) {
					auto result = new string();
					if (disp == 0)
						*result = sym->Name;
					else
						*result = std::format("{}+0x{:X}", sym->Name, disp);
					*result = GetModuleName(p.get(), modBase) + "!" + *result;
					return result;
				}
			}
			return nullptr;
			}, [t](auto result) {
				if (result) {
					auto name = (string*)result;
					t->m_Win32StartAddressName = move(*name);
					delete name;
				}
				});
	}
	return t->m_Win32StartAddressName;
}

std::string ThreadInfoEx::GetModuleName(ProcessInfoEx* p, uint64_t baseAddress) {
	auto& symbols = p->GetSymbols();
	auto name = symbols.GetModuleName(baseAddress);
	char text[256];
	sprintf_s(text, "%ws", name.c_str());
	return text;
}

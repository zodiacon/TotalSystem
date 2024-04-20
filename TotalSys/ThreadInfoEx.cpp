#include "pch.h"
#include "ThreadInfoEx.h"

void ThreadInfoEx::Term(uint32_t ms) {
	TransientObject::Term(ms);

	State = WinLL::ThreadState::Terminated;
	CPU = 0;
}

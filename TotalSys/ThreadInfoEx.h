#pragma once

#include <ThreadInfo.h>
#include "TransientObject.h"

class ThreadInfoEx : public WinLL::ThreadInfo, public TransientObject {
public:
	void Term(uint32_t ms) override;

	int GetMemoryPriority() const;
};


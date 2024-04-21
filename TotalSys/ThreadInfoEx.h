#pragma once

#include <ThreadInfo.h>
#include "TransientObject.h"
#include <WinLowLevel.h>

class ThreadInfoEx : public WinLL::ThreadInfo, public TransientObject {
public:
	void Term(uint32_t ms) override;

	[[nodiscard]] int GetMemoryPriority() const;
	[[nodiscard]] int GetSuspendCount() const;
	[[nodiscard]] WinLL::IoPriority GetIoPriority() const;
	[[nodiscard]] std::wstring const& GetServiceName() const;

private:
	mutable std::wstring m_Service{ L" " };
};


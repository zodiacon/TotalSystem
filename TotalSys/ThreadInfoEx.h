#pragma once

#include <ThreadInfo.h>
#include "TransientObject.h"
#include <WinLowLevel.h>
#include <mutex>

class ProcessInfoEx;

class ThreadInfoEx : public WinLL::ThreadInfo, public TransientObject {
public:
	void Term(uint32_t ms) override;

	[[nodiscard]] int GetMemoryPriority() const;
	[[nodiscard]] int GetSuspendCount() const;
	[[nodiscard]] WinLL::IoPriority GetIoPriority() const;
	[[nodiscard]] std::wstring const& GetServiceName() const;
	[[nodiscard]] static std::string GetModuleName(ProcessInfoEx* p, uint64_t baseAddress);
	[[nodiscard]] bool IsSuspended() const;

private:
	mutable std::wstring m_Service{ L" " };
};


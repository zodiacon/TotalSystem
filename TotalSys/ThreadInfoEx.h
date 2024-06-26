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
	[[nodiscard]] std::wstring const& GetDescription() const;
	bool Suspend() const;
	bool Resume() const;
	bool Terminate(uint32_t code = 0) const;

private:
	mutable std::wstring m_Service{ L" " };
	mutable std::wstring m_Desc;
	mutable DWORD64 m_TargetDesc{ 0 };
};


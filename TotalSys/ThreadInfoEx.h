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
	[[nodiscard]] static std::string GetStartAddressSymbol(std::shared_ptr<ProcessInfoEx> p, std::shared_ptr<ThreadInfoEx> t);
	[[nodiscard]] static std::string GetWin32StartAddressSymbol(std::shared_ptr<ProcessInfoEx> p, std::shared_ptr<ThreadInfoEx> t);
	[[nodiscard]] static std::string GetModuleName(ProcessInfoEx* p, uint64_t baseAddress);

private:
	mutable std::wstring m_Service{ L" " };
	mutable std::string m_StartAddressName, m_Win32StartAddressName;
	mutable bool m_AttemptStartAddress : 1 { false }, m_AttemptWin32StartAddress : 1 { false };
	inline static std::mutex s_Lock;
};


#pragma once

#include <string>
#include "Keys.h"

namespace WinLL {
	enum class ThreadState : uint32_t {
		Initialized = 0,
		Ready = 1,
		Running = 2,
		Standby = 3,
		Terminated = 4,
		Waiting = 5,
		Transition = 6,
		DeferredReady = 7,
		GateWaitObsolete = 8,
		WaitingForProcessInSwap = 9
	};

	enum class WaitReason : uint32_t {
		Executive,
		FreePage,
		PageIn,
		PoolAllocation,
		DelayExecution,
		Suspended,
		UserRequest,
		WrExecutive,
		WrFreePage,
		WrPageIn,
		WrPoolAllocation,
		WrDelayExecution,
		WrSuspended,
		WrUserRequest,
		WrEventPair,
		WrQueue,
		WrLpcReceive,
		WrLpcReply,
		WrVirtualMemory,
		WrPageOut,
		WrRendezvous,
		WrKeyedEvent,
		WrTerminated,
		WrProcessInSwap,
		WrCpuRateControl,
		WrCalloutStack,
		WrKernel,
		WrResource,
		WrPushLock,
		WrMutex,
		WrQuantumEnd,
		WrDispatchInt,
		WrPreempted,
		WrYieldExecution,
		WrFastMutex,
		WrGuardedMutex,
		WrRundown,
		WrAlertByThreadId,
		WrDeferredPreempt,
		MaximumWaitReason
	};

	struct ThreadInfo {
		template<typename TProcessInfo, typename TThreadInfo>
		friend class ProcessManager;
	public:
		const std::wstring& GetProcessImageName() const {
			return m_ProcessName;
		}

		uint64_t KernelTime;
		uint64_t UserTime;
		uint64_t CreateTime;
		uint32_t WaitTime;
		uint32_t Id, ProcessId;
		int32_t Priority;
		int32_t BasePriority;
		uint32_t ContextSwitches;
		ThreadState State;
		WaitReason WaitReason;
		int32_t CPU;
		void* StartAddress;
		void* StackBase;
		void* StackLimit;
		void* Win32StartAddress;
		void* TebBase;

		ProcessOrThreadKey Key;

	private:
		std::wstring m_ProcessName;
	};
}


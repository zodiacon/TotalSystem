#pragma once

#include <memory>
#include <vector>
#include <type_traits>
#include <unordered_map>
#include <wil\resource.h>
#include "Keys.h"
#include "ProcessInfo.h"
#include "ThreadInfo.h"
#include "SecurityHelper.h"
#include <VersionHelpers.h>

#ifdef __cplusplus
#if _MSC_VER >= 1300
#define TYPE_ALIGNMENT( t ) __alignof(t)
#endif
#else
#define TYPE_ALIGNMENT( t ) \
	FIELD_OFFSET( struct { char x; t test; }, test )
#endif

namespace WinLL {
	using namespace std;
	template<typename TProcessInfo = ProcessInfo, typename TThreadInfo = ThreadInfo>
	class ProcessManager {
		static_assert(is_base_of_v<ProcessInfo, TProcessInfo>);
		static_assert(is_base_of_v<ThreadInfo, TThreadInfo>);
	public:
		ProcessManager(const ProcessManager&) = delete;
		ProcessManager& operator=(const ProcessManager&) = delete;

		void Update() {
			Update(false, 0);
		}
		void UpdateWithThreads(uint32_t pid = 0) {
			Update(true, pid);
		}

		[[nodiscard]] vector<shared_ptr<TProcessInfo>> const& GetTerminatedProcesses() const {
			return m_terminatedProcesses;
		}
		[[nodiscard]] vector<shared_ptr<TProcessInfo>> const& GetNewProcesses() const {
			return m_newProcesses;
		}

		[[nodiscard]] wstring GetProcessNameById(uint32_t pid) const {
			if (pid == 0)
				return L"";
			auto pi = GetProcessById(pid);
			return pi ? pi->GetImageName() : L"";
		}

		ProcessManager() {
			if (s_TotalProcessors == 0) {
				s_TotalProcessors = ::GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
				s_IsElevated = SecurityHelper::IsRunningElevated();
			}
			m_BufferSize = 1 << 22;
			m_Buffer.reset((BYTE*)::VirtualAlloc(nullptr, m_BufferSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
		}

		void Update(bool includeThreads, uint32_t pid) {
			vector<shared_ptr<TProcessInfo>> processes;
			processes.reserve(m_processes.empty() ? 512 : m_processes.size() + 10);
			ProcessMap processesByKey;
			processesByKey.reserve(m_processes.size() == 0 ? 512 : m_processes.size() + 10);
			m_ProcessesById.clear();
			m_ProcessesById.reserve(m_processes.capacity());

			m_newProcesses.clear();

			ThreadMap threadsByKey;
			if (includeThreads) {
				threadsByKey.reserve(4096);
				m_newThreads.clear();
				if (m_threads.empty())
					m_newThreads.reserve(4096);
				m_threads.clear();
				m_threadsById.clear();
			}

			ULONG len;

			// get timing info as close as possible to the API call

			LARGE_INTEGER ticks;
			::QueryPerformanceCounter(&ticks);
			auto delta = ticks.QuadPart - m_prevTicks.QuadPart;

			NTSTATUS status;
			bool extended;
			if (s_IsElevated && IsWindows8OrGreater()) {
				status = NtQuerySystemInformation(SystemFullProcessInformation, m_Buffer.get(), m_BufferSize, &len);
				extended = true;
			}
			else {
				extended = false;
				status = NtQuerySystemInformation(SystemExtendedProcessInformation, m_Buffer.get(), m_BufferSize, &len);
			}
			if (NT_SUCCESS(status)) {
				auto p = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>(m_Buffer.get());

				for (;;) {
					if (pid == 0 || pid == HandleToULong(p->UniqueProcessId)) {
						ProcessOrThreadKey key = { p->CreateTime.QuadPart, HandleToULong(p->UniqueProcessId) };
						shared_ptr<TProcessInfo> pi;
						if (auto it = m_ProcessesByKey.find(key); it == m_ProcessesByKey.end()) {
							// new process
							pi = BuildProcessInfo(p, includeThreads, threadsByKey, delta, pi, extended);
							m_newProcesses.push_back(pi);
							pi->CPU = pi->KernelCPU = 0;
						}
						else {
							const auto& pi2 = it->second;
							auto kcpu = delta == 0 ? 0 : (int32_t)((p->KernelTime.QuadPart - pi2->KernelTime) * 1000000 / delta / s_TotalProcessors);
							auto cpu = delta == 0 ? 0 : (int32_t)((p->KernelTime.QuadPart + p->UserTime.QuadPart - pi2->UserTime - pi2->KernelTime) * 1000000 / delta / s_TotalProcessors);
							pi = BuildProcessInfo(p, includeThreads, threadsByKey, delta, pi2, extended);
							pi->CPU = cpu;
							pi->KernelCPU = kcpu;

							// remove from known processes
							m_ProcessesByKey.erase(key);
						}
						processes.push_back(pi);
						//
						// add process to maps
						//
						processesByKey.insert({ key, pi });
						m_ProcessesById.insert({ pi->Id, pi });
					}
					if (p->NextEntryOffset == 0)
						break;
					p = reinterpret_cast<SYSTEM_PROCESS_INFORMATION*>((BYTE*)p + p->NextEntryOffset);
				}
			}
			m_processes = move(processes);

			//
			// remaining processes are terminated ones
			//
			m_terminatedProcesses.clear();
			m_terminatedProcesses.reserve(m_ProcessesByKey.size());
			for (const auto& [key, pi] : m_ProcessesByKey)
				m_terminatedProcesses.push_back(pi);

			m_ProcessesByKey = move(processesByKey);

			if (includeThreads) {
				m_terminatedThreads.clear();
				m_terminatedThreads.reserve(m_threadsByKey.size());
				for (const auto& [key, ti] : m_threadsByKey)
					m_terminatedThreads.push_back(ti);

				m_threadsByKey = move(threadsByKey);
			}

			m_prevTicks = ticks;
		}

		[[nodiscard]] vector<shared_ptr<TProcessInfo>> const& GetProcesses() const {
			return m_processes;
		}

		[[nodiscard]] shared_ptr<TProcessInfo> GetProcessInfo(int index) const {
			return m_processes[index];
		}

		[[nodiscard]] shared_ptr<TProcessInfo> GetProcessById(uint32_t pid) const {
			auto it = m_ProcessesById.find(pid);
			return it == m_ProcessesById.end() ? nullptr : it->second;
		}

		[[nodiscard]] shared_ptr<TProcessInfo> GetProcessByKey(const ProcessOrThreadKey& key) const {
			auto it = m_ProcessesByKey.find(key);
			return it == m_ProcessesByKey.end() ? nullptr : it->second;
		}

		vector<shared_ptr<TThreadInfo>> const& GetThreads() const {
			return m_threads;
		}

		[[nodiscard]] size_t GetProcessCount() const {
			return m_processes.size();
		}

		[[nodiscard]] shared_ptr<TThreadInfo> GetThreadInfo(int index) const {
			return m_threads[index];
		}

		[[nodiscard]] shared_ptr<TThreadInfo> GetThreadByKey(const ProcessOrThreadKey& key) const {
			auto it = m_threadsByKey.find(key);
			return it == m_threadsByKey.end() ? nullptr : it->second;
		}

		[[nodiscard]] const vector<shared_ptr<TThreadInfo>>& GetTerminatedThreads() const {
			return m_terminatedThreads;
		}

		[[nodiscard]] const vector<shared_ptr<TThreadInfo>>& GetNewThreads() const {
			return m_newThreads;
		}

		[[nodiscard]] size_t GetThreadCount() const {
			return m_threads.size();
		}

		[[nodiscard]] vector<pair<shared_ptr<TProcessInfo>, int>> BuildProcessTree() {
			vector<pair<shared_ptr<TProcessInfo>, int>> tree;
			auto count = Update(false, 0);
			tree.reserve(count);

			auto map = m_ProcessesById;
			for (auto& p : m_processes) {
				auto it = m_ProcessesById.find(p->ParentId);
				if (p->ParentId == 0 || it == m_ProcessesById.end() || (it != m_ProcessesById.end() && it->second->CreateTime > p->CreateTime)) {
					// root
					DbgPrint((PSTR)"Root: %ws (%u) (Parent: %u)\n", p->GetImageName().c_str(), p->Id, p->ParentId);
					tree.push_back(make_pair(p, 0));
					map.erase(p->Id);
					if (p->Id == 0)
						continue;
					auto children = FindChildren(map, p.get(), 1);
					for (auto& child : children)
						tree.push_back(make_pair(m_ProcessesById[child.first], child.second));
				}
			}
			return tree;
		}

		vector<pair<uint32_t, int>> FindChildren(unordered_map<uint32_t, shared_ptr<TProcessInfo>>& map, TProcessInfo* parent, int indent) {
			vector<pair<uint32_t, int>> children;
			for (auto& p : m_processes) {
				if (p->ParentId == parent->Id && p->CreateTime > parent->CreateTime) {
					children.push_back(make_pair(p->Id, indent));
					map.erase(p->Id);
					auto children2 = FindChildren(map, p.get(), indent + 1);
					children.insert(children.end(), children2.begin(), children2.end());
				}
			}
			return children;
		}

	private:
		using ProcessMap = unordered_map<ProcessOrThreadKey, shared_ptr<TProcessInfo>>;
		using ThreadMap = unordered_map<ProcessOrThreadKey, shared_ptr<TThreadInfo>>;

		shared_ptr<TProcessInfo> BuildProcessInfo(const SYSTEM_PROCESS_INFORMATION* info, bool includeThreads,
			ThreadMap& threadsByKey, int64_t delta, shared_ptr<TProcessInfo> pi, bool extended) {
			SYSTEM_PROCESS_INFORMATION_EXTENSION* ext = nullptr;
			if (pi == nullptr) {
				pi = make_shared<TProcessInfo>();
				pi->Id = HandleToULong(info->UniqueProcessId);
				pi->SessionId = info->SessionId;
				pi->CreateTime = info->CreateTime.QuadPart;
				pi->Key.Created = pi->CreateTime;
				pi->Key.Id = pi->Id;
				pi->ParentId = HandleToULong(info->InheritedFromUniqueProcessId);
				pi->ClearThreads();
				auto name = info->UniqueProcessId == 0 ? L"(Idle)" : wstring(info->ImageName.Buffer, info->ImageName.Length / sizeof(WCHAR));
				if(extended) {
					ext = (SYSTEM_PROCESS_INFORMATION_EXTENSION*)((BYTE*)info +
						FIELD_OFFSET(SYSTEM_PROCESS_INFORMATION, Threads) + sizeof(SYSTEM_EXTENDED_THREAD_INFORMATION) * info->NumberOfThreads);
					pi->JobObjectId = ext->JobObjectId;
					auto index = name.rfind(L'\\');
					::memcpy(pi->UserSid, (BYTE*)ext + ext->UserSidOffset, sizeof(pi->UserSid));
					pi->m_ProcessName = index == wstring::npos ? name : name.substr(index + 1);
					pi->m_NativeImagePath = name;
					if (ext->PackageFullNameOffset > 0) {
						pi->m_PackageFullName = (const wchar_t*)((BYTE*)ext + ext->PackageFullNameOffset);
					}
				}
				else {
					pi->m_ProcessName = move(name);
					pi->JobObjectId = 0;
				}
			}
			pi->ThreadCount = info->NumberOfThreads;
			pi->BasePriority = info->BasePriority;
			pi->UserTime = info->UserTime.QuadPart;
			pi->KernelTime = info->KernelTime.QuadPart;
			pi->HandleCount = info->HandleCount;
			pi->PageFaultCount = info->PageFaultCount;
			pi->PeakThreads = info->NumberOfThreadsHighWatermark;
			pi->PeakVirtualSize = info->PeakVirtualSize;
			pi->VirtualSize = info->VirtualSize;
			pi->WorkingSetSize = info->WorkingSetSize;
			pi->PeakWorkingSetSize = info->PeakWorkingSetSize;
			pi->PagefileUsage = info->PagefileUsage;
			pi->OtherOperationCount = info->OtherOperationCount.QuadPart;
			pi->ReadOperationCount = info->ReadOperationCount.QuadPart;
			pi->WriteOperationCount = info->WriteOperationCount.QuadPart;
			pi->HardFaultCount = info->HardFaultCount;
			pi->OtherTransferCount = info->OtherTransferCount.QuadPart;
			pi->ReadTransferCount = info->ReadTransferCount.QuadPart;
			pi->WriteTransferCount = info->WriteTransferCount.QuadPart;
			pi->PeakPagefileUsage = info->PeakPagefileUsage;
			pi->CycleTime = info->CycleTime;
			pi->NonPagedPoolUsage = info->QuotaNonPagedPoolUsage;
			pi->PagedPoolUsage = info->QuotaPagedPoolUsage;
			pi->PeakNonPagedPoolUsage = info->QuotaPeakNonPagedPoolUsage;
			pi->PeakPagedPoolUsage = info->QuotaPeakPagedPoolUsage;
			pi->PrivatePageCount = info->PrivatePageCount;
			if (ext) {
				pi->DiskCounters = ext->DiskCounters;
				pi->ContextSwitches = ext->ContextSwitches;
			}
			if (includeThreads) {
				auto threadCount = info->NumberOfThreads;
				for (ULONG i = 0; i < threadCount; i++) {
					auto tinfo = ((SYSTEM_EXTENDED_THREAD_INFORMATION*)info->Threads) + i;
					if (pi->Id == 0)
						tinfo->ThreadInfo.ClientId.UniqueThread = ULongToHandle(i);
					const auto& baseInfo = tinfo->ThreadInfo;
					ProcessOrThreadKey key = { baseInfo.CreateTime.QuadPart, HandleToULong(baseInfo.ClientId.UniqueThread) };
					shared_ptr<TThreadInfo> thread;
					shared_ptr<TThreadInfo> ti2;
					bool newobject = true;
					int64_t cpuTime = 0;
					if (auto it = m_threadsByKey.find(key); it != m_threadsByKey.end()) {
						thread = it->second;
						cpuTime = thread->UserTime + thread->KernelTime;
						newobject = false;
					}
					if (newobject) {
						thread = make_shared<TThreadInfo>();
						thread->m_ProcessName = pi->GetImageName();
						thread->Id = HandleToULong(baseInfo.ClientId.UniqueThread);
						thread->ProcessId = HandleToULong(baseInfo.ClientId.UniqueProcess);
						thread->CreateTime = baseInfo.CreateTime.QuadPart;
						thread->StartAddress = baseInfo.StartAddress;
						thread->StackBase = tinfo->StackBase;
						thread->StackLimit = tinfo->StackLimit;
						thread->Win32StartAddress = tinfo->Win32StartAddress;
						thread->TebBase = tinfo->TebBase;
						thread->Key = key;
						pi->AddThread(thread);
					}
					thread->KernelTime = baseInfo.KernelTime.QuadPart;
					thread->UserTime = baseInfo.UserTime.QuadPart;
					thread->Priority = baseInfo.Priority;
					thread->BasePriority = baseInfo.BasePriority;
					thread->ThreadState = (ThreadState)baseInfo.ThreadState;
					thread->WaitReason = (WaitReason)baseInfo.WaitReason;
					thread->WaitTime = baseInfo.WaitTime;
					thread->ContextSwitches = baseInfo.ContextSwitches;

					m_threads.push_back(thread);
					if (newobject) {
						// new thread
						thread->CPU = 0;
						m_newThreads.push_back(thread);
					}
					else {
						thread->CPU = delta == 0 ? 0 : (int32_t)((thread->KernelTime + thread->UserTime - cpuTime) * 1000000LL / delta / s_TotalProcessors);
						m_threadsByKey.erase(thread->Key);
					}
					threadsByKey.insert(make_pair(thread->Key, thread));
					m_threadsById.insert(make_pair(thread->Id, thread));
				}
			}
			return pi;
		}

		// processes

		unordered_map<uint32_t, shared_ptr<TProcessInfo>> m_ProcessesById;
		vector<shared_ptr<TProcessInfo>> m_processes;
		vector<shared_ptr<TProcessInfo>> m_terminatedProcesses;
		vector<shared_ptr<TProcessInfo>> m_newProcesses;
		ProcessMap m_ProcessesByKey;

		// threads

		vector<shared_ptr<TThreadInfo>> m_threads;
		vector<shared_ptr<TThreadInfo>> m_newThreads;
		vector<shared_ptr<TThreadInfo>> m_terminatedThreads;
		unordered_map<uint32_t, shared_ptr<TThreadInfo>> m_threadsById;
		ThreadMap m_threadsByKey;

		LARGE_INTEGER m_prevTicks{};
		inline static uint32_t s_TotalProcessors;
		inline static bool s_IsElevated;

		wil::unique_virtualalloc_ptr<BYTE> m_Buffer;
		ULONG m_BufferSize;
	};
}

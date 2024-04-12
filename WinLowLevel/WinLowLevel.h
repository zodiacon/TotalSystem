#pragma once

#ifdef CreateProcess
#undef CreateProcess
#endif

#ifdef GetUserName
#undef GetUserName
#endif

#ifdef GetCommandLine
#undef GetCommandLine
#endif

#include <wil\resource.h>

//#define DEFINE_ENUM(Name, Type) \
//    struct Name {   \
//        using _Type = Type; \
//        Type _Value;    \
//        operator Type() const { return _Value; }   \
//        bool HasFlags(Name flag) const { return ((_Type)flag & _Value) == flag; }   \
//    template<typename T>    \
//        Name(T init) : _Value((Type)init) {}  \
//        enum : Type {
//
//#define ENUM_VALUE(Name, Value) Name = Value,
//#define END_ENUM() \
//    };  \
//};
//
//#define DEFINE_ENUM_EXT(Name, Base) \
//    struct Name : Base {   \
//        using Base::Base;   \
//        bool HasFlags(Name flag) const { return (flag & _Value) == flag; }   \
//        enum : typename Base::_Type {
//
//DEFINE_ENUM(GenericAccessMask, DWORD)
//ENUM_VALUE(ReadControl, READ_CONTROL)
//ENUM_VALUE(GenericRead, GENERIC_READ)
//ENUM_VALUE(GenericWrite, GENERIC_WRITE)
//ENUM_VALUE(GenericExecute, GENERIC_EXECUTE)
//ENUM_VALUE(GenericAll, GENERIC_ALL)
//END_ENUM()
//
//DEFINE_ENUM_EXT(DispatcherObjectAccessMask, GenericAccessMask)
//ENUM_VALUE(Synchronize, SYNCHRONIZE)
//END_ENUM()
//
//DEFINE_ENUM_EXT(ProcessAccessMask, DispatcherObjectAccessMask)
//ENUM_VALUE(Terminate, PROCESS_TERMINATE)
//ENUM_VALUE(SetSessionId, PROCESS_SET_SESSIONID)
//ENUM_VALUE(SetLimitedInformation, PROCESS_SET_LIMITED_INFORMATION)
//ENUM_VALUE(QueryInformation, PROCESS_QUERY_INFORMATION)
//ENUM_VALUE(DupHandle, PROCESS_DUP_HANDLE)
//ENUM_VALUE(QueryLimitedInformation, PROCESS_QUERY_LIMITED_INFORMATION)
//ENUM_VALUE(CreateThread, PROCESS_CREATE_THREAD)
//ENUM_VALUE(SuspendResume, PROCESS_SUSPEND_RESUME)
//ENUM_VALUE(VmOperation, PROCESS_VM_OPERATION)
//ENUM_VALUE(VmRead, PROCESS_VM_READ)
//ENUM_VALUE(VmWrite, PROCESS_VM_WRITE)
//ENUM_VALUE(CreateProcess, PROCESS_CREATE_PROCESS)
//ENUM_VALUE(SetQuota, PROCESS_SET_QUOTA)
//ENUM_VALUE(SetInformation, PROCESS_SET_INFORMATION)
//ENUM_VALUE(All, PROCESS_ALL_ACCESS)
//END_ENUM()
//
//DEFINE_ENUM_EXT(ThreadAccessMask, DispatcherObjectAccessMask)
//ENUM_VALUE(Terminate, THREAD_TERMINATE)
//ENUM_VALUE(SetLimitedInformation, THREAD_SET_LIMITED_INFORMATION)
//ENUM_VALUE(SetInformation, THREAD_SET_INFORMATION)
//ENUM_VALUE(QueryInformation, THREAD_QUERY_INFORMATION)
//ENUM_VALUE(QueryLimitedInformation, THREAD_QUERY_LIMITED_INFORMATION)
//ENUM_VALUE(GetContext, THREAD_GET_CONTEXT)
//ENUM_VALUE(SetContext, THREAD_SET_CONTEXT)
//ENUM_VALUE(SuspendResume, THREAD_SUSPEND_RESUME)
//ENUM_VALUE(Resume, THREAD_RESUME)
//ENUM_VALUE(Impersonate, THREAD_IMPERSONATE)
//ENUM_VALUE(DirectImpersonation, THREAD_DIRECT_IMPERSONATION)
//ENUM_VALUE(SetToken, THREAD_SET_THREAD_TOKEN)
//ENUM_VALUE(All, THREAD_ALL_ACCESS)
//END_ENUM()
//
//DEFINE_ENUM_EXT(TokenAccessMask, GenericAccessMask)
//ENUM_VALUE(Query, TOKEN_QUERY)
//ENUM_VALUE(AssignPrimary, TOKEN_ASSIGN_PRIMARY)
//ENUM_VALUE(QuerySource, TOKEN_QUERY_SOURCE)
//ENUM_VALUE(Duplicate, TOKEN_DUPLICATE)
//ENUM_VALUE(Impersonate, TOKEN_IMPERSONATE)
//ENUM_VALUE(AdjustDefault, TOKEN_ADJUST_DEFAULT)
//ENUM_VALUE(AdjustGroups, TOKEN_ADJUST_GROUPS)
//ENUM_VALUE(AdjustPrivileges, TOKEN_ADJUST_PRIVILEGES)
//ENUM_VALUE(AdjustSessionId, TOKEN_ADJUST_SESSIONID)
//ENUM_VALUE(All, TOKEN_ALL_ACCESS)
//END_ENUM()

namespace WinLL {
	using namespace std;

	struct AccessMask {
		AccessMask(ULONG access = 0) : Access(access) {}
		operator ULONG() const {
			return Access;
		}
		template<typename T>
		bool HasFlag(T flag) const {
			return ((ULONG)flag & Access) == flag;
		}

		union {
			struct {
				USHORT SpecificRights;
				UCHAR StandardRights;
				UCHAR AccessSystemAcl : 1;
				UCHAR _Reserved : 3;
				UCHAR GenericAll : 1;
				UCHAR GenericExecute : 1;
				UCHAR GenericWrite : 1;
				UCHAR GenericRead : 1;
			} Bits;
			ULONG Access;
		};
	};

	struct GenericAccessMask : AccessMask {
		using AccessMask::AccessMask;

		enum : uint32_t {
			ReadControl = READ_CONTROL,
			GenericRead = GENERIC_READ,
			GenericWrite = GENERIC_WRITE,
			GenericExecute = GENERIC_EXECUTE,
			GenericAll = GENERIC_ALL,
		};
	};

	struct DispatcherAccessMask : GenericAccessMask {
		using GenericAccessMask::GenericAccessMask;

		enum {
			Syncronize = SYNCHRONIZE,
		};
	};

	struct ProcessAccessMask : DispatcherAccessMask {
		using DispatcherAccessMask::DispatcherAccessMask;

		enum : uint32_t {
			Terminate = PROCESS_TERMINATE,
			SetSessionId = PROCESS_SET_SESSIONID,
			SetLimitedInformation = PROCESS_SET_LIMITED_INFORMATION,
			QueryInformation = PROCESS_QUERY_INFORMATION,
			DupHandle = PROCESS_DUP_HANDLE,
			QueryLimitedInformation = PROCESS_QUERY_LIMITED_INFORMATION,
			CreateThread = PROCESS_CREATE_THREAD,
			SuspendResume = PROCESS_SUSPEND_RESUME,
			VmOperation = PROCESS_VM_OPERATION,
			VmRead = PROCESS_VM_READ,
			VmWrite = PROCESS_VM_WRITE,
			CreateProcess = PROCESS_CREATE_PROCESS,
			SetQuota = PROCESS_SET_QUOTA,
			SetInformation = PROCESS_SET_INFORMATION,
			All = PROCESS_ALL_ACCESS,
		};
	};

	struct TokenAccessMask : GenericAccessMask {
		using GenericAccessMask::GenericAccessMask;
		enum : uint32_t {
			Query = TOKEN_QUERY,
			AssignPrimary = TOKEN_ASSIGN_PRIMARY,
			QuerySource = TOKEN_QUERY_SOURCE,
			Duplicate = TOKEN_DUPLICATE,
			Impersonate = TOKEN_IMPERSONATE,
			AdjustDefault = TOKEN_ADJUST_DEFAULT,
			AdjustGroups = TOKEN_ADJUST_GROUPS,
			AdjustPrivileges = TOKEN_ADJUST_PRIVILEGES,
			AdjustSessionId = TOKEN_ADJUST_SESSIONID,
			All = TOKEN_ALL_ACCESS,
		};
	};

	enum class DuplicateHandleOptions : uint32_t {
		None = 0,
		SameAccess = DUPLICATE_SAME_ACCESS,
		CloseSource = DUPLICATE_CLOSE_SOURCE,
	};
	DEFINE_ENUM_FLAG_OPERATORS(DuplicateHandleOptions);

	struct Timeout {
		enum : uint32_t {
			Infinite = INFINITE
		};
	};

	class Process;

	class KernelObject abstract {
	public:
		explicit KernelObject(HANDLE hObject = nullptr);
		void Close();
		HANDLE Handle() const;
		operator bool() const;

		HANDLE Attach(HANDLE hObject);
		HANDLE Detach();

		bool ChangeAccess(AccessMask newAccess);
		static HANDLE Duplicate(Process const& srcProcess, KernelObject const& srcObject, Process const& targetProcess,
			AccessMask access = 0, DuplicateHandleOptions options = DuplicateHandleOptions::SameAccess);
		HANDLE Duplicate(Process const& srcProcess, Process const& targetProcess,
			AccessMask access = 0, DuplicateHandleOptions options = DuplicateHandleOptions::SameAccess);

		template<typename TAccess>
		static HANDLE Duplicate(Process const& srcProcess, KernelObject const& srcObject, Process const& targetProcess, TAccess access, bool closeSource = false) {
			return Duplicate(srcProcess, srcObject, targetProcess, (DWORD)access, closeSource ? DuplicateHandleOptions::CloseSource : DuplicateHandleOptions::None);
		}

		bool IsSameObject(KernelObject const& other) const;
		wstring GetName() const;

	protected:
		wil::unique_any_handle_null<decltype(&::NtClose), ::NtClose> m_hObject;
	};

	enum class WaitOneResult : uint32_t {
		Success = WAIT_OBJECT_0,
		Timeout = WAIT_TIMEOUT,
		Abandoned = WAIT_ABANDONED,
		Error = WAIT_FAILED,
	};

	struct WaitAllResult {
		enum : DWORD {
			Success = WAIT_OBJECT_0,
			Timeout = WAIT_TIMEOUT,
			Abandoned = WAIT_ABANDONED,
			Error = WAIT_FAILED,
			Index = 0,
		};
	};

	template<typename TLock>
	struct Locker {
		Locker(TLock& lock) : m_Lock(lock) {
			m_Lock.Lock();
		}
		~Locker() {
			m_Lock.Unlock();
		}
		TLock& m_Lock;
	};

	class CriticalSection final : CRITICAL_SECTION {
	public:
		explicit CriticalSection(uint32_t spinCount = 0);
		void Lock();
		void Unlock();
		void TryLock();
	};

	class DispatcherObject abstract : public KernelObject {
	public:
		using KernelObject::KernelObject;

		void Lock();
		void Unlock();
		WaitOneResult WaitOne(uint32_t msec = Timeout::Infinite) const;
		WaitAllResult WaitAll(uint32_t msec = Timeout::Infinite) const;
		bool IsSignaled() const;
	};

	class Mutex final : public DispatcherObject {
	public:
		using DispatcherObject::DispatcherObject;

		WaitOneResult Lock(uint32_t timeout = Timeout::Infinite);
		bool Unlock();
	};

	enum class EventType {
		AutoReset,
		ManualReset,
	};

	class Event final : public DispatcherObject {
	public:
		using DispatcherObject::DispatcherObject;

		Event(EventType type, bool signaled = false);
		bool Set();
		bool Reset();
		bool Pulse();
	};

	enum class PriorityClass {
		Error = 0,
		Normal = NORMAL_PRIORITY_CLASS,
		BelowNormal = BELOW_NORMAL_PRIORITY_CLASS,
		AboveNormal = ABOVE_NORMAL_PRIORITY_CLASS,
		Idle = IDLE_PRIORITY_CLASS,
		High = HIGH_PRIORITY_CLASS,
		Realtime = REALTIME_PRIORITY_CLASS,
	};

	enum class IntegrityLevel : uint32_t {
		Untrusted = 0,
		Low = SECURITY_MANDATORY_LOW_RID,
		Medium = SECURITY_MANDATORY_MEDIUM_RID,
		MediumPlus = SECURITY_MANDATORY_MEDIUM_PLUS_RID,
		High = SECURITY_MANDATORY_HIGH_RID,
		System = SECURITY_MANDATORY_SYSTEM_RID,
		Protected = SECURITY_MANDATORY_PROTECTED_PROCESS_RID,
	};

	enum class ProcessProtectionSigner : uint8_t {
		None,
		Authenticode,
		CodeGen,
		Antimalware,
		Lsa,
		Windows,
		WinTcb,
		WinSystem,
		App,
	};

	enum class VirtualizationState {
		Unknown,
		NotAllowed,
		Enabled,
		Disabled
	};

	enum class DpiAwareness {
		Unknown = -1,
		None = DPI_AWARENESS_UNAWARE,
		System = DPI_AWARENESS_SYSTEM_AWARE,
		PerMonitor = DPI_AWARENESS_PER_MONITOR_AWARE,
	};

	struct ProcessProtection {
		union {
			uint8_t Level;
			struct {
				uint8_t Type : 3;
				uint8_t Audit : 1;
				ProcessProtectionSigner Signer : 4;
			};
		};
	};

	typedef struct _PEB* PPEB;

	class Process : public DispatcherObject {
	public:
		using DispatcherObject::DispatcherObject;

		bool Open(uint32_t pid, ProcessAccessMask access = ProcessAccessMask::QueryInformation);
		static Process Current();

		int32_t GetExitCode() const;
		uint32_t GetId() const;
		PPEB GetPeb() const;
		wstring GetCommandLine() const;
		int32_t GetSessionId() const;
		wstring GetImagePath() const;
		wstring GetImageName() const;
		wstring GetUserName(bool includeDomain = false) const;
		wstring GetPackageName() const;
		wstring GetAppUserModelId() const;
		wstring GetAppId() const;
		PriorityClass GetPriorityClass() const;
		bool SetPriorityClass(PriorityClass pc);
		bool Terminate(int32_t exitCode = 0);
		std::wstring GetFullImageName() const;

		bool Suspend();
		bool Resume();

		bool IsImmersive() const noexcept;
		bool IsProtected() const;
		bool IsSecure() const;
		bool IsInJob(HANDLE hJob = nullptr) const;
		bool IsWow64Process() const;
		bool IsManaged() const;

	private:
		bool GetExtendedInfo(PROCESS_EXTENDED_BASIC_INFORMATION* info) const;
	};

	enum class ThreadPriorityLevel {
		Idle = THREAD_PRIORITY_IDLE,
		Lowest = THREAD_PRIORITY_LOWEST,
		BelowNormal = THREAD_PRIORITY_BELOW_NORMAL,
		Normal = THREAD_PRIORITY_NORMAL,
		AboveNormal = THREAD_PRIORITY_ABOVE_NORMAL,
		Highest = THREAD_PRIORITY_HIGHEST,
		TimeCritical = THREAD_PRIORITY_TIME_CRITICAL
	};

	struct AttributesFlags {
		AttributesFlags(uint32_t value = 0) : Value(value) {}

		operator uint32_t() const {
			return Value;
		}
		enum : uint32_t {
			None = 0,
			KernelHandle = OBJ_KERNEL_HANDLE,
			Permanent = OBJ_PERMANENT,
			Exclusive = OBJ_EXCLUSIVE,
			Inherit = OBJ_INHERIT,
			OpenIf = OBJ_OPENIF,
			OpenLink = OBJ_OPENLINK,
			CaseInsensitive = OBJ_CASE_INSENSITIVE,
		};
		uint32_t Value;
	};

	struct ObjectAttributes : OBJECT_ATTRIBUTES {
		ObjectAttributes(PUNICODE_STRING name, AttributesFlags attributes = AttributesFlags::None);

		operator POBJECT_ATTRIBUTES() const {
			return (POBJECT_ATTRIBUTES)this;
		}
	};

	class Thread : public DispatcherObject {
	public:
		using DispatcherObject::DispatcherObject;
	};

	class Token : public KernelObject {
	public:
		using KernelObject::KernelObject;

		bool Open(TokenAccessMask access, uint32_t pid = 0);

		wstring GetUserName(bool includeDomain = false) const;
		//Sid GetUserSid() const;
	};

	class File : public DispatcherObject {
	public:
	};

	class Job : public DispatcherObject {
	public:
		using DispatcherObject::DispatcherObject;
	};
}

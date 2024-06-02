#pragma once

#pragma once

#define DRIVER_CURRENT_VERSION 0x0100
#define DEVICE_TOTALSYS 0x8012

#define IOCTL_KTOTALSYS_OPEN_OBJECT				CTL_CODE(DEVICE_TOTALSYS, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_DUP_HANDLE				CTL_CODE(DEVICE_TOTALSYS, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_EVENT_BY_NAME		CTL_CODE(DEVICE_TOTALSYS, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_SEMAPHORE_BY_NAME	CTL_CODE(DEVICE_TOTALSYS, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_JOB_BY_NAME		CTL_CODE(DEVICE_TOTALSYS, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_DESKTOP_BY_NAME	CTL_CODE(DEVICE_TOTALSYS, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_PROCESS			CTL_CODE(DEVICE_TOTALSYS, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_GET_VERSION				CTL_CODE(DEVICE_TOTALSYS, 0x807, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_GET_OBJECT_ADDRESS		CTL_CODE(DEVICE_TOTALSYS, 0x808, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_THREAD				CTL_CODE(DEVICE_TOTALSYS, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_WINSTA_BY_NAME		CTL_CODE(DEVICE_TOTALSYS, 0x80a, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_MUTEX_BY_NAME		CTL_CODE(DEVICE_TOTALSYS, 0x80b, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KTOTALSYS_OPEN_PROCESS_TOKEN		CTL_CODE(DEVICE_TOTALSYS, 0x80c, METHOD_BUFFERED, FILE_ANY_ACCESS)

struct OpenObjectData {
	void* Address;
	ACCESS_MASK Access;
};

struct DupHandleData {
	ULONG Handle;
	ULONG SourcePid;
	ACCESS_MASK AccessMask;
	ULONG Flags;
};

struct OpenProcessThreadData {
	ULONG Id;
	ACCESS_MASK AccessMask;
};


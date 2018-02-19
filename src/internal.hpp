#pragma once
#include <Windows.h>
#include <winternl.h>
#include <Aclapi.h>
#include <stdint.h>

#define RTL_CONSTANT_STRING(s) UNICODE_STRING {sizeof(s) - sizeof((s)[0]), sizeof(s), s}

#define SystemSuperfetchInformation static_cast<SYSTEM_INFORMATION_CLASS>(79)

#define STATUS_BUFFER_TOO_SMALL static_cast<NTSTATUS>(0xC0000023)

#define SE_DEBUG_PRIVILEGE                (20L)
#define SE_PROF_SINGLE_PROCESS_PRIVILEGE  (13L)

#define SUPERFETCH_VERSION      45
#define SUPERFETCH_MAGIC        'kuhC'

typedef enum _SUPERFETCH_INFORMATION_CLASS {
	SuperfetchRetrieveTrace = 1,        // Query
	SuperfetchSystemParameters = 2,     // Query
	SuperfetchLogEvent = 3,             // Set
	SuperfetchGenerateTrace = 4,        // Set
	SuperfetchPrefetch = 5,             // Set
	SuperfetchPfnQuery = 6,             // Query
	SuperfetchPfnSetPriority = 7,       // Set
	SuperfetchPrivSourceQuery = 8,      // Query
	SuperfetchSequenceNumberQuery = 9,  // Query
	SuperfetchScenarioPhase = 10,       // Set
	SuperfetchWorkerPriority = 11,      // Set
	SuperfetchScenarioQuery = 12,       // Query
	SuperfetchScenarioPrefetch = 13,    // Set
	SuperfetchRobustnessControl = 14,   // Set
	SuperfetchTimeControl = 15,         // Set
	SuperfetchMemoryListQuery = 16,     // Query
	SuperfetchMemoryRangesQuery = 17,   // Query
	SuperfetchTracingControl = 18,       // Set
	SuperfetchTrimWhileAgingControl = 19,
	SuperfetchInformationMax = 20
} SUPERFETCH_INFORMATION_CLASS;

typedef struct _SUPERFETCH_INFORMATION {
	ULONG Version;
	ULONG Magic;
	SUPERFETCH_INFORMATION_CLASS InfoClass;
	PVOID Data;
	ULONG Length;
} SUPERFETCH_INFORMATION, *PSUPERFETCH_INFORMATION;

typedef struct _PF_PHYSICAL_MEMORY_RANGE {
	ULONG_PTR BasePfn;
	ULONG_PTR PageCount;
} PF_PHYSICAL_MEMORY_RANGE, *PPF_PHYSICAL_MEMORY_RANGE;

typedef struct _PF_MEMORY_RANGE_INFO {
	ULONG Version;
	ULONG RangeCount;
	PF_PHYSICAL_MEMORY_RANGE Ranges[ANYSIZE_ARRAY];
} PF_MEMORY_RANGE_INFO, *PPF_MEMORY_RANGE_INFO;

typedef struct _PHYSICAL_MEMORY_RUN {
	SIZE_T BasePage;
	SIZE_T PageCount;
} PHYSICAL_MEMORY_RUN, *PPHYSICAL_MEMORY_RUN;

struct RTL_PROCESS_MODULE_INFORMATION
{
	unsigned int Section;
	void* MappedBase;
	void* ImageBase;
	unsigned int ImageSize;
	unsigned int Flags;
	unsigned short LoadOrderIndex;
	unsigned short InitOrderIndex;
	unsigned short LoadCount;
	unsigned short OffsetToFileName;
	char FullPathName[256];
};

struct SYSTEM_HANDLE
{
	ULONG ProcessId;
	BYTE ObjectTypeNumber;
	BYTE Flags;
	USHORT Handle;
	PVOID Object;
	ACCESS_MASK GrantedAccess;
};

typedef struct _POOL_HEADER
{
	union
	{
		struct
		{
			ULONG	PreviousSize : 8;
			ULONG	PoolIndex : 8;
			ULONG	BlockSize : 8;
			ULONG	PoolType : 8;
		};
		ULONG	Ulong1;
	};
	ULONG	PoolTag;
	union
	{
		void	*ProcessBilled;
		struct
		{
			USHORT	AllocatorBackTraceIndex;
			USHORT	PoolTagHash;
		};
	};
} POOL_HEADER, *PPOOL_HEADER;

typedef enum _SECTION_INHERIT {
	ViewShare = 1,
	ViewUnmap = 2
} SECTION_INHERIT;

extern "C" NTSTATUS NtOpenSection(
	_Out_ PHANDLE            SectionHandle,
	_In_  ACCESS_MASK        DesiredAccess,
	_In_  POBJECT_ATTRIBUTES ObjectAttributes
);

extern "C" NTSTATUS NtMapViewOfSection(
	_In_        HANDLE          SectionHandle,
	_In_        HANDLE          ProcessHandle,
	_Inout_     PVOID           *BaseAddress,
	_In_        ULONG_PTR       ZeroBits,
	_In_        SIZE_T          CommitSize,
	_Inout_opt_ PLARGE_INTEGER  SectionOffset,
	_Inout_     PSIZE_T         ViewSize,
	_In_        SECTION_INHERIT InheritDisposition,
	_In_        ULONG           AllocationType,
	_In_        ULONG           Win32Protect
);

extern "C" NTSTATUS NtUnmapViewOfSection(
	_In_     HANDLE ProcessHandle,
	_In_opt_ PVOID  BaseAddress
);

extern "C" NTSTATUS NtLoadDriver(
	_In_ PUNICODE_STRING DriverServiceName
);

extern "C" NTSTATUS NtUnloadDriver(
	_In_ PUNICODE_STRING DriverServiceName
);

extern "C" NTSTATUS NTAPI RtlAdjustPrivilege(
	IN ULONG Privilege,
	IN BOOLEAN NewValue,
	IN BOOLEAN ForThread,
	OUT PBOOLEAN OldValue
);

extern "C" NTSTATUS WINAPI NtQuerySystemInformation(
	IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
	OUT PVOID SystemInformation,
	IN ULONG SystemInformationLength,
	OUT PULONG ReturnLength OPTIONAL
);


extern "C" NTSTATUS RtlAdjustPrivilege(ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, PBOOLEAN Enabled);

// Internal privilege value
const ULONG SeLoadDriverPrivilege = 10;

#pragma once
#include <Windows.h>
#include <cstdint>

typedef struct _OBJECT_HEADER
{
	LONG	PointerCount;
	union
	{
		LONG	HandleCount;
		PVOID	NextToFree;
	};
	uint64_t	Lock;
	UCHAR		TypeIndex;
	union
	{
		UCHAR	TraceFlags;
		struct
		{
			UCHAR	DbgRefTrace : 1;
			UCHAR	DbgTracePermanent : 1;
			UCHAR	Reserved : 6;
		};
	};
	UCHAR	InfoMask;
	union
	{
		UCHAR	Flags;
		struct
		{
			UCHAR	NewObject : 1;
			UCHAR	KernelObject : 1;
			UCHAR	KernelOnlyAccess : 1;
			UCHAR	ExclusiveObject : 1;
			UCHAR	PermanentObject : 1;
			UCHAR	DefaultSecurityQuota : 1;
			UCHAR	SingleHandleEntry : 1;
			UCHAR	DeletedInline : 1;
		};
	};
	union
	{
		PVOID	ObjectCreateInfo;
		PVOID	QuotaBlockCharged;
	};
	PVOID	SecurityDescriptor;
	PVOID	Body;
} OBJECT_HEADER, *POBJECT_HEADER;

namespace dk {
	struct object_patcher {
		object_patcher(POBJECT_HEADER header);
		~object_patcher();

	private:
		POBJECT_HEADER header_;
	};

	struct acl_patcher {
		acl_patcher(HANDLE handle);
		~acl_patcher();

	private:
		HANDLE handle_;
		PACL previous_dacl_;
	};
}

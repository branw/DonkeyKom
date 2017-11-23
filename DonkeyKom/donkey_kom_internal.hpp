#pragma once
#include <windows.h>
#include <winternl.h>
#include <AccCtrl.h>
#include <Aclapi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdexcept>

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

extern "C" VOID RtlInitUnicodeString(
	_Out_    PUNICODE_STRING DestinationString,
	_In_opt_ PCWSTR          SourceString
);

extern "C" NTSTATUS ZwOpenSection(
	_Out_ PHANDLE            SectionHandle,
	_In_  ACCESS_MASK        DesiredAccess,
	_In_  POBJECT_ATTRIBUTES ObjectAttributes
);

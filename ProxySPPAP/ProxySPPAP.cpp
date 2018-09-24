// ProxySPPAP.cpp: определяет экспортированные функции для приложения DLL.
//

#include "stdafx.h"

EXTERN_C_START

#include "internals.h"

SpLsaModeInitializeFn * MsvSpLsaModeInitialize = NULL;
SpInitializeFn * MsvSpInitialize = NULL;
PLSA_AP_LOGON_USER_EX2  MsvLsaApLogonUserEx2 = NULL;



NTSTATUS SpInitialize(
	_In_ ULONG_PTR PackageId,
	_In_ PSECPKG_PARAMETERS Parameters,
	_In_ PLSA_SECPKG_FUNCTION_TABLE FunctionTable
)
{
	pLsaDispatch = FunctionTable;
	if ((*MsvSpInitialize)(PackageId, Parameters, FunctionTable) != SEC_E_OK)
	{
		LOG_ERROR("[%s]:Не удалось инициализировать Msv1_0", __FUNCDNAME__ );
		return STATUS_INVALID_HANDLE;
	}
	return(SEC_E_OK);
}

NTSTATUS LsaApLogonEx2(
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
		LOG_ERROR("[%s]:Не удалось авторизоваться в пакете Msv1_0", __FUNCDNAME__);
		return *SubStatus;
	}

	return(SEC_E_OK);
}

NTSTATUS SpLsaModeInitialize(
	ULONG LsaVersion,
	PULONG PackageVersion,
	PSECPKG_FUNCTION_TABLE *ppTables,
	PULONG pcTables
)
{

	MsvSpLsaModeInitialize = (SpLsaModeInitializeFn*)GetProcAddress(MsvPackage, "SpLsaModeInitialize");
	MsvSpInitialize = (SpInitializeFn*)GetProcAddress(MsvPackage, "SpInitialize");
	MsvLsaApLogonUserEx2 = (PLSA_AP_LOGON_USER_EX2)GetProcAddress(MsvPackage, "LsaApLogonUserEx2");


	if (!MsvSpLsaModeInitialize || !MsvSpInitialize || !MsvLsaApLogonUserEx2)
	{
		LOG_ERROR("Не удалось получить обязательные функции из Msv1_0");
		return STATUS_NO_MEMORY;
	}

	if ((*MsvSpLsaModeInitialize)(LsaVersion, PackageVersion, ppTables, pcTables) != SEC_E_OK)
	{
		LOG_ERROR("Не удалось инициализировать Msv1_0");
		return STATUS_INVALID_HANDLE;
	}

	*PackageVersion = 0x100AA001;
	*pcTables = 1;

	(*ppTables)->Initialize = (SpInitializeFn*)SpInitialize;
	(*ppTables)->LogonUserEx2 = (PLSA_AP_LOGON_USER_EX2)LsaApLogonEx2;

	return SEC_E_OK;
}

EXTERN_C_END
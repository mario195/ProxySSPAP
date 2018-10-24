// ProxySPPAP.cpp: определяет экспортированные функции для приложения DLL.
//

#include "stdafx.h"

EXTERN_C_START

#include "internals.h"


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
	LOG_TRACE(L"MsvLsaApInitializePackage");
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

	LOG_TRACE(L"AllocateLsaStringLsa");
	*AuthenticationPackageName = AllocateLsaStringLsa(NTLKAP_NAME_A);

	if (NULL == *AuthenticationPackageName)
	{
		status = STATUS_NO_MEMORY;
		LOG_ERROR(L"Can't allocate memory of authentication package name");
	}

Cleanup:
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
	(*MsvLsaApLogonTerminated)(LogonId);
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

	return(SEC_E_OK);
}

EXTERN_C_END
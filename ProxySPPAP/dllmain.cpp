// dllmain.cpp: определяет точку входа для приложения DLL.
#include "stdafx.h"

EXTERN_C_START

#include "internals.h"
#include <clocale>
#include "sam.h"

PSECPKG_FUNCTION_TABLE pMsvTables;

/* указатель на таблицу функций */
PLSA_DISPATCH_TABLE pLsaDispatch;

/* указатель загружаемый модуль */
HMODULE MsvPackage;

PLSA_AP_INITIALIZE_PACKAGE MsvLsaApInitializePackage = NULL;
PLSA_AP_LOGON_USER_EX2  MsvLsaApLogonUserEx2 = NULL;
PLSA_AP_LOGON_TERMINATED MsvLsaApLogonTerminated = NULL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
	UNREFERENCED_PARAMETER(hModule);
	UNREFERENCED_PARAMETER(lpReserved);


    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
		
		/* выставим русскую локаль*/
		setlocale(LC_CTYPE, "rus");
		LoggerInit(File, NTLKAP_NAME);
		/* загрузим библиотеку */
		MsvPackage = GetModuleHandle(L"Msv1_0");
		if (!MsvPackage)
		{
			LOG_ERROR(L"Error loading Msv1_0.dll");
			return FALSE;
		}

		MsvLsaApInitializePackage = (PLSA_AP_INITIALIZE_PACKAGE)GetProcAddress(MsvPackage, "LsaApInitializePackage");
		MsvLsaApLogonUserEx2 = (PLSA_AP_LOGON_USER_EX2)GetProcAddress(MsvPackage, "LsaApLogonUserEx2");
		MsvLsaApLogonTerminated = (PLSA_AP_LOGON_TERMINATED)GetProcAddress(MsvPackage, "LsaApLogonTerminated");


		LOG_INFO(L"Библиотека загружена");
		break;
    case DLL_THREAD_ATTACH:
		LOG_INFO(L"Загружена в поток %p", GetCurrentThread());
		break;
    case DLL_THREAD_DETACH:
		LOG_INFO(L"Выгружена из потока %p", GetCurrentThread());
		break;
    case DLL_PROCESS_DETACH:
		if (MsvPackage)
		{
			FreeModule(MsvPackage);
			LOG_INFO(L"Библиотека выгружена из процесса");
		}		
		SamFreeResource();
		LoggerFree();
        return TRUE;
    }
	LOG_FUNCTION_CALL();
	LOG_TRACE(L"Reason: %ul", ul_reason_for_call);
    return TRUE;
}

EXTERN_C_END
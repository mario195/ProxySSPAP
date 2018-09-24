// dllmain.cpp: определяет точку входа для приложения DLL.
#include "stdafx.h"

EXTERN_C_START

#include "internals.h"
#include <clocale>

/* указатель на таблицу функций */
PLSA_SECPKG_FUNCTION_TABLE pLsaDispatch;

/* указатель загружаемый модуль */
HMODULE MsvPackage;

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

		/* загрузим библиотеку */
		MsvPackage = LoadLibraryW(L"%SystemRoot%\\System32\\Msv1_0.dll");
		if (!MsvPackage)
		{
			LOG_ERROR("Не удалось загрузить проксируемый пакет Msv1_0.dll");
			return FALSE;
		}
		LOG_INFO("Библиотека загружена");
		break;
    case DLL_THREAD_ATTACH:
		LOG_INFO("Загружена в поток %p", GetCurrentThread());
		break;
    case DLL_THREAD_DETACH:
		LOG_INFO("Выгружена из потока %p", GetCurrentThread());
		break;
    case DLL_PROCESS_DETACH:
		if (MsvPackage)
		{
			FreeLibrary(MsvPackage);
			LOG_INFO("Библиотека выгружена из процесса")
		}		
        break;
    }
    return TRUE;
}

EXTERN_C_END
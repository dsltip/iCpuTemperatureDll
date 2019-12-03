#ifndef	__ICPUTEMP_H
#define	__ICPUTEMP_H

#ifdef	PMDLL_EXPORTS
#define	DLL_DECLARE __declspec(dllexport)
#else
#define	DLL_DECLARE __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

//driver initialize
DLL_DECLARE BYTE LoadDriver();
DLL_DECLARE BYTE LoadDriverMY();
DLL_DECLARE VOID UnloadDriver();

DLL_DECLARE BOOL InitCPUTemp();
DLL_DECLARE WORD ReadCPUTemp();

#ifdef __cplusplus
}
#endif

#endif	//__PMDLL_H
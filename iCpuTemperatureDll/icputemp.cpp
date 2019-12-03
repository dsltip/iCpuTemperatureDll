/*++
Ver: 1.0
Date: 2019/11/30
--*/

#include <windows.h>
#include <winioctl.h>
#include <intrin.h>
#include "icputemp.h"
#include "phymem.h"

HANDLE hDriver=INVALID_HANDLE_VALUE;
DWORD m_dwCpuCoreCount,m_dwCpuCount;
DWORD m_dwNumOfAvailableProcesses;
short* m_pEachCpuCoreTemp;
int m_nAllTemp;
PBYTE m_pTjMax;
KAFFINITY* m_pMasks;
short m_sCur;
unsigned short m_sFamily, m_sModel, m_sStepping;

BOOL InstallDriver(PCSTR pszDriverPath, PCSTR pszDriverName);
BOOL RemoveDriver(PCSTR pszDriverName);
BOOL StartDriver(PCSTR pszDriverName);
BOOL StopDriver(PCSTR pszDriverName);

//get driver(chipsec_hlpr.sys) full path
static BOOL GetDriverPath(PSTR szDriverPath)
{
	PSTR pszSlash;

	if (!GetModuleFileName(GetModuleHandle(NULL), szDriverPath, MAX_PATH))
		return FALSE;

	pszSlash=strrchr(szDriverPath, '\\');

	if (pszSlash)
		pszSlash[1]='\0';
	else
		return FALSE;

	return TRUE;
}

BYTE LoadDriverMY()
{
	return 77;
}
//install and start driver

BYTE LoadDriver()
{
	BOOL bResult;
	CHAR szDriverPath[MAX_PATH];

	hDriver=CreateFile( "\\\\.\\WinRing0_1_2_0",
						GENERIC_READ|GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

	//If the driver is not running, install it
	if (hDriver==INVALID_HANDLE_VALUE)
	{
		GetDriverPath(szDriverPath);
		strcat(szDriverPath, "winring0.sys");

		bResult=InstallDriver(szDriverPath, "PHYRING");

		if (!bResult)
			return 1;

		bResult=StartDriver("PHYRING");

		if (!bResult)
			return 2;

		hDriver=CreateFile( "\\\\.\\WinRing0_1_2_0",
							GENERIC_READ | GENERIC_WRITE,
							0,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);

		if (hDriver==INVALID_HANDLE_VALUE)
			return 3;
	}

	return 0;
}

//stop and remove driver
VOID UnloadDriver()
{
	if (hDriver!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(hDriver);
		hDriver=INVALID_HANDLE_VALUE;
	}
	if (m_pMasks)
		delete[] m_pMasks;
	if (m_pEachCpuCoreTemp)
		delete[] m_pEachCpuCoreTemp; 
	RemoveDriver("PHYRING");
}
const bool _GetIsMsr( )
{
	int info[4];
	__cpuid(info, 1);
	return !!( ( info[3] >> 5 ) & 1 );
} 
const bool __IsNT( )
{
	OSVERSIONINFOEX osie = { sizeof( OSVERSIONINFOEX ), 0, 0, 0, VER_PLATFORM_WIN32_NT };
	return !!VerifyVersionInfo(&osie, VER_PLATFORMID, VerSetConditionMask(0, VER_PLATFORMID, VER_EQUAL));
}
void __InitProcessorsInfo()
{
	DWORD len;
	GetLogicalProcessorInformationEx(RelationProcessorCore,	NULL,&len);
	PBYTE buffer = new BYTE[len];
	GetLogicalProcessorInformationEx(RelationProcessorCore,	reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>( buffer),&len);
	m_dwCpuCoreCount = 0;
	for (PBYTE bufferTmp = buffer;
		 bufferTmp < buffer + len;
		 bufferTmp += reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>( bufferTmp )->Size)
		++m_dwCpuCoreCount;
	if (m_dwCpuCoreCount)
		{
			m_pMasks = new KAFFINITY[m_dwCpuCoreCount];
			m_dwCpuCount = 0;
			size_t i = 0;
			for (PBYTE bufferTmp = buffer;
				 bufferTmp < buffer + len;
				 bufferTmp += reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>( bufferTmp )->Size,++i)
			{
				auto ptr = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>( bufferTmp );
				if (ptr->Processor.GroupCount != 1)
					m_pMasks[i] = 0;
				else
				{
					m_pMasks[i] = ptr->Processor.GroupMask[0].Mask;
					for (auto mask = m_pMasks[i]; mask; mask &= mask - 1)
						++m_dwCpuCount;
				}
			}
		}
	delete[] buffer;
	m_pEachCpuCoreTemp = new short[m_dwCpuCoreCount];
}
DWORD GetCpuCoreCount(){
	return m_dwCpuCoreCount;
}
BOOL __ReadMsr(unsigned long index, KAFFINITY mask, unsigned long long* ret)//const
{
	if (!_GetIsMsr( ))	return FALSE;

	DWORD returnedLength = 0;
	BOOL result = FALSE;
	HANDLE hThread = nullptr;
	DWORD_PTR oldMask;
	if (__IsNT())
	{
		hThread = GetCurrentThread( );
		oldMask = SetThreadAffinityMask(hThread, mask);
		if (!oldMask)
			return FALSE;
	}
	result = DeviceIoControl(hDriver, dwIOCTL_READ_MSR,	&index,	sizeof index,ret,sizeof *ret,&returnedLength,nullptr);
	if (__IsNT())SetThreadAffinityMask(hThread, oldMask);
	if (!result)return FALSE;
	return TRUE;
}
KAFFINITY _GetMask(DWORD dw)
	{ return dw < m_dwCpuCoreCount ? m_pMasks[dw] : 0; } 

void __InitTjMax(BYTE tjMax)
{
	if (m_pTjMax)
		delete[] m_pTjMax;
	m_pTjMax = new BYTE[GetCpuCoreCount( )];
	for (size_t i = 0; i < 4; ++i)
		m_pTjMax[i] = tjMax;
}
void __InitTjMaxFromMsr( )
{
	if (m_pTjMax)
		delete[] m_pTjMax;
	m_pTjMax = new BYTE[GetCpuCoreCount( )]( );
	if (m_pTjMax)
		for (DWORD i = 0; i < GetCpuCoreCount( ); ++i)
		{
			unsigned long long val;
			if (TRUE != __ReadMsr(nIA32_TEMPERATURE_TARGET, _GetMask(i), &val))
				m_pTjMax[i] = 0;
			else
				m_pTjMax[i] = val >> 16 & 0xff;
		}
}

void _Init(){
		__try
		{
			int info[4];
			__cpuid(info, 1);
			m_sFamily = info[0] >> 8 & 0xf;
			m_sModel = info[0] >> 4 & 0xf;
			m_sStepping = info[0] & 0xf;
			if (0xf == m_sFamily ||
				6 == m_sFamily)
				m_sModel |= info[0] >> 12 & 0xf0;
			if (0xf == m_sFamily)
				m_sFamily += info[0] >> 20 & 0xff;
		}
		__except (EXCEPTION_EXECUTE_HANDLER){}

		switch (m_sFamily)
		{
		case 0x06: {
			switch (m_sModel) {
			case 0x0F: // Intel Core 2 (65nm)
				switch (m_sStepping) {
				case 0x06: // B2
					switch (m_dwCpuCount) {
					case 2: __InitTjMax(80 + 10); break;
					case 4: __InitTjMax(90 + 10); break;
					default: __InitTjMax(85 + 10); break;
					}
				case 0x0B: // G0
					__InitTjMax(90 + 10); break;
				case 0x0D: // M0
					__InitTjMax(85 + 10); break;
				default:
					__InitTjMax(85 + 10); break;
				} break;
			case 0x17: // Intel Core 2 (45nm)
				__InitTjMax(100); break;
			case 0x1C: // Intel Atom (45nm)
				switch (m_sStepping) {
				case 0x02: // C0
					__InitTjMax(90); break;
				case 0x0A: // A0, B0
					__InitTjMax(100); break;
				default:
					__InitTjMax(90); break;
				} break;
			case 0x1A: // Intel Core i7 LGA1366 (45nm)
			case 0x1E: // Intel Core i5, i7 LGA1156 (45nm)
			case 0x1F: // Intel Core i5, i7 
			case 0x25: // Intel Core i3, i5, i7 LGA1156 (32nm)
			case 0x2C: // Intel Core i7 LGA1366 (32nm) 6 Core
			case 0x2E: // Intel Xeon Processor 7500 series (45nm)
			case 0x2F: // Intel Xeon Processor (32nm)
				__InitTjMaxFromMsr( );
				break;
			case 0x2A: // Intel Core i5, i7 2xxx LGA1155 (32nm)
			case 0x2D: // Next Generation Intel Xeon, i7 3xxx LGA2011 (32nm)
				__InitTjMaxFromMsr( );
				break;
			case 0x3A: // Intel Core i5, i7 3xxx LGA1155 (22nm)
			case 0x3E: // Intel Core i7 4xxx LGA2011 (22nm)
				__InitTjMaxFromMsr( );
				break;
			case 0x3C: // Intel Core i5, i7 4xxx LGA1150 (22nm)              
			case 0x3F: // Intel Xeon E5-2600/1600 v3, Core i7-59xx
				// LGA2011-v3, Haswell-E (22nm)
			case 0x45: // Intel Core i5, i7 4xxxU (22nm)
			case 0x46:
				__InitTjMaxFromMsr( );
				break;
			case 0x3D: // Intel Core M-5xxx (14nm)
				__InitTjMaxFromMsr( );
				break;
			case 0x36: // Intel Atom S1xxx, D2xxx, N2xxx (32nm)
				__InitTjMaxFromMsr( );
				break;
			case 0x37: // Intel Atom E3xxx, Z3xxx (22nm)
			case 0x4A:
			case 0x4D: // Intel Atom C2xxx (22nm)
			case 0x5A:
			case 0x5D:
				__InitTjMaxFromMsr( );
				break;
			default:
				__InitTjMax(100);
				break;
			}
		} break;
		case 0x0F: {
			switch (m_sModel) {
			case 0x00: // Pentium 4 (180nm)
			case 0x01: // Pentium 4 (130nm)
			case 0x02: // Pentium 4 (130nm)
			case 0x03: // Pentium 4, Celeron D (90nm)
			case 0x04: // Pentium 4, Pentium D, Celeron D (90nm)
			case 0x06: // Pentium 4, Pentium D, Celeron D (65nm)
				__InitTjMax(100);
				break;
			default:
				__InitTjMax(100);
				break;
			}
		} break;
		default:
			__InitTjMax(100);
			break;
		}

}
void Update()
{
	m_nAllTemp = 0;
	m_dwNumOfAvailableProcesses = 0;
	for (DWORD i = 0; i < GetCpuCoreCount( ); ++i)
	{
		unsigned long long val;
		if (TRUE != __ReadMsr(nIA32_THERM_STATUS_MSR, _GetMask(i), &val))
		{
			if (m_pEachCpuCoreTemp)
				m_pEachCpuCoreTemp[i] = SHRT_MIN;
		}
		else
		{
			short temp = (short)m_pTjMax[i] - (short)( val >> 16 & 0x7f );
			if (m_pEachCpuCoreTemp)
				m_pEachCpuCoreTemp[i] = temp;
			m_nAllTemp += temp;
			++m_dwNumOfAvailableProcesses;
		}
	}
	if (m_dwNumOfAvailableProcesses)
		m_sCur = short(m_nAllTemp / m_dwNumOfAvailableProcesses);
}
BOOL InitCPUTemp()
{
	__InitProcessorsInfo();
	//__InitTjMaxFromMsr();
	_Init();

	return TRUE;

}
WORD ReadCPUTemp()
{
	Update();
	return m_sCur;
}


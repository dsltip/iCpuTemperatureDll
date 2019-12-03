Helper library for winring0.sys to read Intel CPU temperature.
Based on https://github.com/mewiteor/MonitorBar

it exports:
DLL_DECLARE BYTE LoadDriver();
DLL_DECLARE BYTE LoadDriverMY();
DLL_DECLARE VOID UnloadDriver();

DLL_DECLARE BOOL InitCPUTemp();
DLL_DECLARE WORD ReadCPUTemp();

Example:

	BYTE b=LoadDriver();
	
	if (b!=0)
	{
		printf("load winring0.sys failed, code  %d\n",b);
		exit(-1);
	}

	InitCPUTemp();
	DWORD vv;
	vv = ReadCPUTemp();
	printf("CPU temp= %d\n", vv);
	getchar();
	UnloadDriver();

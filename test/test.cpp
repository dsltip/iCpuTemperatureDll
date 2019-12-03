#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include "icputemp.h"


int main()
{
	BYTE b=LoadDriver();
	//BYTE bb=LoadDriverMY();printf("Num=  %d\n",bb);
	
	if (b!=0)
	{
		printf("load winring0.sys failed, code  %d\n",b);
		exit(-1);
	}

	InitCPUTemp();
	DWORD vv;
	vv = ReadCPUTemp();
	printf("CPU temp= %d\n",	vv);
	vv = ReadCPUTemp();
	printf("CPU temp= %d\n",	vv);
	vv = ReadCPUTemp();
	printf("CPU temp= %d\n",	vv);

	//getchar();
	UnloadDriver();
	return 0;
}

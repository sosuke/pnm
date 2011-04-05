/************************************************************/
/* [�v���b�g�t�H�[�����擾]�i�e�X�g���Ă��痘�p�j         */
/*   + GetDeviceType                                        */
/*   + GetPlatformInfo                                      */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

#ifdef P2PHOSTWM
////////////////////////////////////////////
//
// GetDeviceType
//
////////////////////////////////////////////
DWORD GetDeviceType(void)
{
	DWORD dwDeviceType = 0;
	WCHAR szPlatform[64];

	DBGOUTEX2("[INFO0001]GetPlatformInfo Function Starts.\n");

    //+++ ������ +++
	ZeroMemory(szPlatform, sizeof(szPlatform));

	//+++ �f�o�C�X�⑫�����擾 +++
	::SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(szPlatform), (PVOID)szPlatform, 0);
	if(::wcsncmp(szPlatform, L"PocketPC", 64) == 0)
	{
		dwDeviceType = DEVICE_TYPE_POCKETPC;
	}
	else if(::wcsncmp(szPlatform, L"SmartPhone", 64) == 0)
	{
		dwDeviceType = DEVICE_TYPE_SMARTPHONE;
	} 
	else
	{
		dwDeviceType = DEVICE_TYPE_NONE;
	}
	DBGOUTEX2("[INFO0002]GetPlatformInfo Function Ends.\n");
	return dwDeviceType;
}

////////////////////////////////////////////
//
// GetPlatformInfo
//
////////////////////////////////////////////
DWORD GetPlatformInfo(void)
{
	DWORD dwPlatformType = PLATFORM_TYPE_NONE;
	OSVERSIONINFO osvi;

	DBGOUTEX2("[INFO0001]GetPlatformInfo Function Starts.\n");

	//+++ ������ +++
	::ZeroMemory(&osvi, sizeof(osvi));

	//+++ Platform ���擾 {+++
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&osvi); 

	if ((osvi.dwMajorVersion == 6) && (osvi.dwMinorVersion == 01)) 
	{
		dwPlatformType = PLATFORM_TYPE_WM60;
	}
	else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 01)) 
	{
		dwPlatformType = PLATFORM_TYPE_WM50;
	}
	else if ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion == 20)) 
	{
		dwPlatformType = PLATFORM_TYPE_WM2003;
	}
	else if ((osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion == 21)) 
	{
		dwPlatformType = PLATFORM_TYPE_WM2003SE;
	}
	else
	{
		dwPlatformType = PLATFORM_TYPE_NONE;
	}

	DBGOUTEX2("[INFO0002]GetPlatformInfo Function Ends.\n");

	return dwPlatformType;
}
#endif P2PHOSTWM
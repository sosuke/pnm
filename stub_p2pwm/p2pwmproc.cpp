#include "stdafx.h"
#include "stub_p2pwm.h"
#include <windows.h>
#include <commctrl.h>
#include "p2pwm.h"

//プロトタイプ宣言(Private)
void DisplayObject(PDATA_UNIT pData);
GUID DisplayPeopleNearMe(PDATA_UNIT pData);
GUID GetFirstPeerGuid(void);

CONST GUID MESSAGE_GUID1 = { 
    0x3e082b49,
    0xb6c4,
    0x4526,
    {0xa5, 0xfa, 0x2d, 0xd7, 0xad, 0xe0, 0x9b, 0xf9}
};

// p2pwm.dllエクスポートAPI
typedef HRESULT (WINAPI *LPWMPEERCOLLABSTARTUP)(WORD); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABSHUTDOWN)(); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABSIGNIN)(HWND, DWORD); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABSIGNOUT)(DWORD); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABSETOBJECT)(PCPEER_OBJECT); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABDELETEOBJECT)(GUID*); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABENUMOBJECTS)(GUID*, GUID*, HANDLE*); 
typedef HRESULT (WINAPI *LPWMPEERGETITEMCOUNT)(HANDLE, DWORD*); 
typedef HRESULT (WINAPI *LPWMPEERENDENUMERATION)(HANDLE); 
typedef HRESULT (WINAPI *LPWMPEERGETNEXTITEM)(HANDLE, DWORD*, PVOID**); 
typedef VOID (WINAPI *LPWMPEERFREEDATA)(PVOID**, DWORD); 
typedef HRESULT (WINAPI *LPWMPEERENDENUMERATION)(HANDLE); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABENUMPEOPLENEARME)(HANDLE*); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABREGISTEREVENT)(HANDLE, DWORD, PPEER_COLLAB_EVENT_REGISTRATION, HANDLE*); 
typedef HRESULT (WINAPI *LPWMPEERCOLLABGETEVENTDATA)(HANDLE, PPEER_COLLAB_EVENT_DATA); 
typedef VOID (WINAPI *LPWMPEERFREEEVENTDATA)(PEER_COLLAB_EVENT_DATA*); 

LPWMPEERCOLLABSTARTUP           lpWmPeerCollabStartup; 
LPWMPEERCOLLABSHUTDOWN          lpWmPeerCollabShutdown;
LPWMPEERCOLLABSIGNIN	        lpWmPeerCollabSignin; 
LPWMPEERCOLLABSIGNOUT	        lpWmPeerCollabSignout; 
LPWMPEERCOLLABSETOBJECT	        lpWmPeerCollabSetObject; 
LPWMPEERCOLLABDELETEOBJECT      lpWmPeerCollabDeleteObject; 
LPWMPEERCOLLABENUMOBJECTS	    lpWmPeerCollabEnumObjects; 
LPWMPEERCOLLABENUMPEOPLENEARME  lpWmPeerCollabEnumPeopleNearMe;
LPWMPEERGETITEMCOUNT		    lpWmPeerGetItemCount; 
LPWMPEERGETNEXTITEM			    lpWmPeerGetNextItem;
LPWMPEERENDENUMERATION		    lpWmPeerEndEnumeration;
LPWMPEERFREEDATA			    lpWmPeerFreeData;
LPWMPEERCOLLABREGISTEREVENT     lpWmPeerCollabRegisterEvent;
LPWMPEERCOLLABGETEVENTDATA      lpWmPeerCollabGetEventData;
LPWMPEERFREEEVENTDATA           lpWmPeerFreeEventData;

// グローバル変数:
HINSTANCE           hLib;               // p2pwm.dllライブラリハンドル

//
//  関数 : InitP2PLibrary()
//
//  目的 : p2pwm.dll使用前準備
//
//  コメント:
//
BOOL InitP2PLibrary()
{
	BOOL bRetValue = TRUE;

	hLib = ::LoadLibrary(L"p2pwm.dll");
	if(hLib == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}

	lpWmPeerCollabSignin =
                    (LPWMPEERCOLLABSIGNIN)GetProcAddress(hLib, L"WmPeerCollabSignin");
	if(lpWmPeerCollabSignin == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabSignout =
                    (LPWMPEERCOLLABSIGNOUT)GetProcAddress(hLib, L"WmPeerCollabSignout");
	if(lpWmPeerCollabSignout == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabSetObject =
                    (LPWMPEERCOLLABSETOBJECT)GetProcAddress(hLib, L"WmPeerCollabSetObject");
	if(lpWmPeerCollabSetObject == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabDeleteObject =
                    (LPWMPEERCOLLABDELETEOBJECT)GetProcAddress(hLib, L"WmPeerCollabDeleteObject");
	if(lpWmPeerCollabDeleteObject == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}	
	lpWmPeerCollabEnumObjects =
                    (LPWMPEERCOLLABENUMOBJECTS)GetProcAddress(hLib, L"WmPeerCollabEnumObjects");
	if(lpWmPeerCollabEnumObjects == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerGetItemCount =
                    (LPWMPEERGETITEMCOUNT)GetProcAddress(hLib, L"WmPeerGetItemCount");
	if(lpWmPeerGetItemCount == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerEndEnumeration =
                    (LPWMPEERENDENUMERATION)GetProcAddress(hLib, L"WmPeerEndEnumeration");
	if(lpWmPeerEndEnumeration == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerGetNextItem =
                    (LPWMPEERGETNEXTITEM)GetProcAddress(hLib, L"WmPeerGetNextItem");
	if(lpWmPeerGetNextItem == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerFreeData =
                    (LPWMPEERFREEDATA)GetProcAddress(hLib, L"WmPeerFreeData");
	if(lpWmPeerFreeData == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabEnumPeopleNearMe =
                    (LPWMPEERCOLLABENUMPEOPLENEARME)GetProcAddress(hLib, L"WmPeerCollabEnumPeopleNearMe");
	if(lpWmPeerCollabEnumPeopleNearMe == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabRegisterEvent =
                    (LPWMPEERCOLLABREGISTEREVENT)GetProcAddress(hLib, L"WmPeerCollabRegisterEvent");
	if(lpWmPeerCollabRegisterEvent == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabGetEventData =
                    (LPWMPEERCOLLABGETEVENTDATA)GetProcAddress(hLib, L"WmPeerCollabGetEventData");
	if(lpWmPeerCollabGetEventData == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerFreeEventData =
                    (LPWMPEERFREEEVENTDATA)GetProcAddress(hLib, L"WmPeerFreeEventData");
	if(lpWmPeerFreeEventData == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabStartup =
                    (LPWMPEERCOLLABSTARTUP)GetProcAddress(hLib, L"WmPeerCollabStartup");
	if(lpWmPeerCollabStartup == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
	lpWmPeerCollabShutdown =
                    (LPWMPEERCOLLABSHUTDOWN)GetProcAddress(hLib, L"WmPeerCollabShutdown");
	if(lpWmPeerCollabShutdown == NULL)
	{
		bRetValue = FALSE;
		goto EXIT;
	}

EXIT:
	return bRetValue;
}


//
//  関数 : terminate()
//
//  目的 : 各種開放処理
//
//  コメント:
//
void terminate(void)
{
	if(hLib != NULL)
	{
		::FreeLibrary(hLib); // DLL をメモリから解放します。
	}	
}

//
//  関数 : EventHandler()
//
//  目的 : イベント通知
//
//  コメント:
//
DWORD WINAPI EventHandler(LPVOID lpv)
{
    HANDLE                hPeerEvent = NULL;
    HRESULT               hr = S_OK;
    PEER_COLLAB_EVENT_REGISTRATION  eventReg[2];
    HANDLE                hEvent;
	DWORD count = 0;
	DATA_UNIT**       ppObjects = NULL;

    hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (hEvent == NULL)
    {
        return E_OUTOFMEMORY;
    }

    eventReg[0].eventType = PEER_EVENT_ENDPOINT_OBJECT_CHANGED;
    eventReg[0].pInstance = NULL;

    eventReg[1].eventType = PEER_EVENT_PEOPLE_NEAR_ME_CHANGED;
    eventReg[1].pInstance = NULL;

    hr = (*lpWmPeerCollabRegisterEvent)(hEvent, 2, eventReg, &hPeerEvent);  

	if (SUCCEEDED(hr))
    {
		while (WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0)
		{
			PEER_COLLAB_EVENT_DATA EventData;

			if((*lpWmPeerCollabGetEventData)(hPeerEvent, &EventData) != S_OK)
			{
				continue;
			}
            switch(EventData.eventType)
			{
			case PEER_EVENT_ENDPOINT_OBJECT_CHANGED:
				for(int i = 0; i < EventData.dwItemCount; i++)
				{
					DisplayObject((PDATA_UNIT)EventData.ppItems[i]);
				}		
				break;
			case PEER_EVENT_PEOPLE_NEAR_ME_CHANGED:
				for(int i = 0; i < EventData.dwItemCount; i++)
				{
					DisplayPeopleNearMe((PDATA_UNIT)EventData.ppItems[i]);
				}				
                break;

            }
            (*lpWmPeerFreeEventData)(&EventData);
		}
    }

	return 0;
}

void SignInProc(void)
{
	(*lpWmPeerCollabSignin)(NULL, 0);
}

void SignOutProc(void)
{
	(*lpWmPeerCollabSignout)(0);
}

void StartupProc(void)
{
	(*lpWmPeerCollabStartup)(0);
}

void ShutdownProc(void)
{
	(*lpWmPeerCollabShutdown)();
}


void RegiesterEvent(void)
{
	::CreateThread(NULL, 0, EventHandler, NULL, 0, NULL);
}

void SetObjectProc()
{
	WCHAR         szBuff[256] = {0};
	PEER_OBJECT   object;
	HRESULT bRet;

	::ZeroMemory(szBuff, sizeof(szBuff));
	::wsprintf(szBuff, L"PEEROBJECT1");
	object.id = MESSAGE_GUID1;
    object.data.cbData = (ULONG) (wcslen(szBuff) + 1) * sizeof(WCHAR);
    object.data.pbData = (PBYTE) szBuff;
    object.dwPublicationScope = 1;

	bRet = (*lpWmPeerCollabSetObject)(&object);
}

void DeleteObjectProc()
{
	GUID id = MESSAGE_GUID1;
	HRESULT bRet;
	bRet = (*lpWmPeerCollabDeleteObject)(&id);
}

GUID DisplayPeopleNearMe(PDATA_UNIT pData)
{
	WCHAR data[100];
	GUID globalId = pData->dataId;
	::wcsncpy(data, (WCHAR*)pData->pbData, 100);
	return globalId;
}

void EnumPeopleNearMe()
{
	HANDLE  hPnmEnum = NULL;
	HRESULT bRet;
	DWORD dwPnmCount;
	DATA_UNIT**  ppPnms = NULL;

	bRet = (*lpWmPeerCollabEnumPeopleNearMe)(&hPnmEnum);
	bRet = (*lpWmPeerGetItemCount)(hPnmEnum, &dwPnmCount);
	bRet = (*lpWmPeerGetNextItem)(hPnmEnum, &dwPnmCount, (PVOID **) &ppPnms);
	for (int i = 0; i < dwPnmCount; i++)
	{
		DisplayPeopleNearMe(ppPnms[i]);
	}
	(*lpWmPeerFreeData)((PVOID **)&ppPnms, dwPnmCount);
	(*lpWmPeerEndEnumeration)(hPnmEnum);
}

GUID GetFirstPeerGuid()
{
	HANDLE  hPnmEnum = NULL;
	HRESULT bRet;
	DWORD dwPnmCount;
	DATA_UNIT**  ppPnms = NULL;
	GUID peerGuid;

	::ZeroMemory(&peerGuid, sizeof(peerGuid));

	bRet = (*lpWmPeerCollabEnumPeopleNearMe)(&hPnmEnum);
	bRet = (*lpWmPeerGetItemCount)(hPnmEnum, &dwPnmCount);
	bRet = (*lpWmPeerGetNextItem)(hPnmEnum, &dwPnmCount, (PVOID **) &ppPnms);
	for (int i = 0; i < dwPnmCount; i++)
	{
		peerGuid = DisplayPeopleNearMe(ppPnms[i]);
		break;
	}
	(*lpWmPeerFreeData)((PVOID **)&ppPnms, dwPnmCount);
	(*lpWmPeerEndEnumeration)(hPnmEnum);

	return peerGuid;
}

void DisplayObject(PDATA_UNIT pData)
{
	WCHAR data[2000];
	::wcscpy(data, (WCHAR*)pData->pbData);
}

void EnumObjects()
{
	HANDLE hObjectEnum = NULL;
	HRESULT bRet;
	DWORD dwObjCount;
	DATA_UNIT** ppObjects = NULL;
	GUID globalId = MESSAGE_GUID1;
	GUID deviceId;

	deviceId = GetFirstPeerGuid();
	bRet = (*lpWmPeerCollabEnumObjects)(&deviceId, &globalId, &hObjectEnum);	
	
	bRet = (*lpWmPeerGetItemCount)(hObjectEnum, &dwObjCount);
	bRet = (*lpWmPeerGetNextItem)(hObjectEnum, &dwObjCount, (PVOID **) &ppObjects);
	for (int i = 0; i < dwObjCount; i++)
	{
		DisplayObject(ppObjects[i]);
	}
	(*lpWmPeerFreeData)((PVOID **)&ppObjects, dwObjCount);
	(*lpWmPeerEndEnumeration)(hObjectEnum);
}
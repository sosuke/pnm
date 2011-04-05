/************************************************************/
/* [イベントデータ管理]                                     */
/*   + SetEventNotify                                       */
/*   + InitDeviceProfile                                    */
/*   + WndProc                                              */
/*   + WmSignInProc                                         */
/*   + WmSignOutProc                                        */
/************************************************************/
//
//【イベントデータメモリ構造】
//   | EventType | EventAction | PEER_ID | count | GUID | cbSize | pData | GUID | cbSize | pData | ...
//                                       ↑ここからが実際のイベントデータ（基本的にデータは１つ）
//
#include "stdafx.h"
#include "p2phostwm.h"

//--　グローバル変数
BOOL g_IsWatchObject = FALSE;   //Objectイベント監視フラグ
BOOL g_IsWatchPnm = FALSE;      //PNMイベント監視フラグ
HANDLE hRemoteEvent;            //プロセス間イベント

////////////////////////////////////////////
//
// SetEventNotify
//
////////////////////////////////////////////
HRESULT SetEventNotify(DWORD dwEventType, DWORD dwEventAction, GUID* deviceId, PVOID lpEventData, DWORD dwDataSize)
{	
	DBGOUTEX2("[INFO0001]SetEventNotify Started.\n");

	HRESULT nRetValue = S_OK;    //戻り値
	HANDLE hMap = NULL;          //共有メモリハンドル
	LPVOID pAddr = NULL;         //共有メモリアドレス
	char* ptr = NULL;            //作業用ポインタ

	hMap = ::CreateFileMapping(
		(HANDLE)INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,//|SEC_COMMIT,
		0, 
		0x1000, 
		FM_EVENTDATA ); // = OPEN
	if(hMap == NULL)
	{
		DBGOUTEX2("[EROR1002]CreateFileMapping failed. \n");
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	pAddr = ::MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0x1000 );
	if(pAddr == NULL)
	{
		DBGOUTEX2("[EROR1002]MapViewOfFile failed.\n");
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	ptr = (char*)pAddr;

	::memcpy(ptr, &dwEventType, sizeof(DWORD));  
	ptr += sizeof(DWORD);
	::memcpy(ptr, &dwEventAction, sizeof(DWORD));
	ptr += sizeof(DWORD);
	::memcpy(ptr, deviceId, sizeof(GUID));
	ptr += sizeof(GUID);
	::memcpy(ptr, (char*)lpEventData, dwDataSize); 

	PulseEvent(hRemoteEvent);
	//SetEvent(hRemoteEvent);

EXIT:
	if(nRetValue != S_OK)
	{
		if(hMap != NULL)
		{
			::CloseHandle(hMap);
		}
	}
	//共有メモリハンドルは今後使用するためにここで開放しない
	//if(pAddr != NULL)
	//{
	//	::UnmapViewOfFile(pAddr);
	//}
	DBGOUTEX2("[INFO0002]SetEventNotify Ended.\n");
	return nRetValue;
}

////////////////////////////////////////////
//
// PnmEventNotify
//
////////////////////////////////////////////
HRESULT PnmEventNotify(PEER_CHANGE_TYPE actionType, GUID* deviceId, WCHAR* userName)
{
	//| count | DEVIDE_GUID | cbSize(UserNameSize) | pData(UseName) | 

	HRESULT nRetValue = S_OK;    //戻り値
	DWORD dwUserNameSize = 0;    //ユーザ名長
	DWORD dwDataSize = 0;        //PNMイベントデータ長
	char* lpData = NULL;         //PNMイベントデータ
	char* ptr = NULL;            //作業用ポインタ
	DWORD count = 1;             //PNMイベントの場合ユニット数は1であることが前提

	if(g_IsWatchPnm)
	{		
		//dwUserNameSize = (::wcslen(userName)+1)*sizeof(WCHAR);		
		dwUserNameSize = ::wcslen(userName)*sizeof(WCHAR);		
		dwDataSize = sizeof(DWORD)+sizeof(GUID)+sizeof(DWORD)+ dwUserNameSize; 
		lpData = (char*)::malloc(dwDataSize);
		if(lpData == NULL)
		{
			DBGOUTEX2("[EROR1002]malloc failed.\n");
			nRetValue = E_INVALIDARG;
			goto EXIT;
		}
		ptr = lpData;
		::memcpy(ptr, &count, sizeof(DWORD));
		ptr += sizeof(DWORD);
		::memcpy(ptr, deviceId, sizeof(GUID));
		ptr += sizeof(GUID);
		::memcpy(ptr, &dwUserNameSize, sizeof(DWORD));
		ptr += sizeof(DWORD);
		::memcpy(ptr, userName, dwUserNameSize);

		nRetValue = SetEventNotify(
			PEER_EVENT_PEOPLE_NEAR_ME_CHANGED, 
			actionType,
			deviceId,
			lpData, 
			dwDataSize
			);	
	}

EXIT:
	if(lpData != NULL)
	{
		::free(lpData);
	}	
	return nRetValue;
}

////////////////////////////////////////////
//
// ObjectEventNotify
//
////////////////////////////////////////////
HRESULT ObjectEventNotify(PEER_CHANGE_TYPE actionType, GUID* objectId, GUID* deviceId)
{
	// + DELETEの場合
	//   {イベントデータ} = オブジェクト問い合わせリプライ
	// + Add/Updateの場合 **
	//   | count | OBJECT_GUID | cbSize(OBJECT_SIZE) | pData(OBJECT_DATA) | 

	HRESULT nRetValue = S_OK;              //戻り値
	struct sockaddr_in6 to;                //OBJECT問い合わせ先アドレス
	SOCKET conSock = INVALID_SOCKET;       //OBJECT要求用ソケット(TCP)
	DWORD dwEmptyObjectSize = 0;           //実データサイズ(0)
	DWORD count = 1;                       //OBJECT_DELイベントの場合ユニット数は1であることが前提
	char send[SIZEOF_WCHAR+SIZEOF_GUID+1]; //オブジェク取得依頼パケット
	char* lpData = NULL;                   //イベントデータポインタ
	DWORD dwDataSize = 0;                  //イベントデータサイズ
	char* ptr = NULL;                      //作業用ポインタ
	char recv[MAX_RECV_OBJECTS];           //全オブジェクトデータ		
	int len = 0;                           //パケット受信データサイズ

	if(g_IsWatchObject)
	{
		//-- 初期化
		::ZeroMemory(&to, sizeof(to));
		::ZeroMemory(send, sizeof(send));
		::ZeroMemory(recv, sizeof(recv));
			
		switch(actionType)
		{
		case PEER_CHANGE_ADDED:
		case PEER_CHANGE_UPDATED:
			GetPeerAddress(*deviceId, &to);
			to.sin6_port = htons(TCP_PORT);
			
			//+++ TCPソケット作成 +++
			if ((conSock = ::socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
				nRetValue = PEER_S_NO_CONNECTIVITY;
				goto EXIT;
			}
			//+++ 接続 +++
			if(::connect(conSock, (struct sockaddr *)&to, sizeof(struct sockaddr_in6)) == SOCKET_ERROR)
			{
				DBGOUTEX2("[EROR1002]connect failed\n");
				nRetValue = PEER_S_NO_CONNECTIVITY;
				goto EXIT;
			}
			//+++ オブジェクト要求をSEND +++
			::memset(send, 'B', sizeof(char));
			::memcpy(send+sizeof(char), objectId, sizeof(GUID));
			dwDataSize = sizeof(char)+sizeof(GUID);
			if(::send(conSock, send, dwDataSize, 0)==-1)
			{
				DBGOUTEX2("[EROR1002]send failed\n");
				nRetValue = PEER_S_NO_CONNECTIVITY;
				goto EXIT;
			}
			//+++ RECV +++
			if( (len = ::recv(conSock, recv, sizeof(recv), 0)) == -1)
			{
				DBGOUTEX2("[EROR1002]recv failed\n");
				nRetValue = PEER_S_NO_CONNECTIVITY;
				goto EXIT;
			}
			dwDataSize = len;
			lpData = (char*)::malloc(len);
			if(lpData == NULL)
			{
				DBGOUTEX2("[EROR1002]malloc failed\n");
				nRetValue = E_OUTOFMEMORY;
				goto EXIT;
			}
			::memcpy(lpData, recv, len);
			break;

		case PEER_CHANGE_DELETED:
			dwDataSize = sizeof(DWORD)+sizeof(GUID)+sizeof(DWORD);
			lpData = (char*)::malloc(dwDataSize);
			if(lpData == NULL)
			{
				DBGOUTEX2("[EROR1002]malloc failed\n");
				nRetValue = E_OUTOFMEMORY;
				goto EXIT;
			}
			ptr = lpData;
			::memcpy(ptr, &count, sizeof(DWORD));
			ptr += sizeof(DWORD);
			::memcpy(ptr, objectId, sizeof(GUID));
			ptr += sizeof(GUID);
			::memcpy(ptr, &dwEmptyObjectSize, sizeof(DWORD));
			break;

		default:
			break;
		}
		nRetValue = SetEventNotify(
			PEER_EVENT_ENDPOINT_OBJECT_CHANGED, 
			actionType,
			deviceId,
			lpData, 
			dwDataSize
			);	
	}

EXIT:
	if(conSock != INVALID_SOCKET)
	{
		::closesocket(conSock);
	}
	if(lpData != NULL)
	{
		::free(lpData);
	}
	return nRetValue;
}

////////////////////////////////////////////
//
// RegisterEvent
//
////////////////////////////////////////////
LRESULT RegisterEvent(PCOPYDATASTRUCT pData)
{	
	HRESULT nRetValue = S_OK;  //戻り値
	DWORD dwFlags = 0;         //イベント立ち上げフラグ

	//+++ イベントハンドラの取得 +++
	char* ptr = (char*)pData->lpData;
	memcpy(&hRemoteEvent, ptr, sizeof(HANDLE));
	ptr += sizeof(HANDLE);
	memcpy(&dwFlags, ptr, sizeof(DWORD));
	
	//+++ 状態監視フラグのセット +++
	if( (dwFlags & 0x00000001) != 0)
	{
		g_IsWatchObject  = TRUE;
	}
	if( (dwFlags & 0x00000010) != 0)
	{
		g_IsWatchPnm     = TRUE;
	}	
EXIT:
	return nRetValue;
}
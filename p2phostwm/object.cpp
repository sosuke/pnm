/************************************************************/
/* [PNM/Object要求処理]                                     */
/*  + API不足のための強引な対応                             */
/*   + EnumPeopleNearMe                                     */
/*   + SetObject                                            */
/*   + DeleteObject                                         */
/*   + EnumObjects                                          */
/*   + EnumMyObjects                                        */
/*   + EnumPnmObjects                                       */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

extern std::list<Object>    MyObjectList;
extern std::list<ObjectMap> MasterPeerObjects;
extern std::list<PeerMap>   MasterPeerMap;
extern CRITICAL_SECTION     cs_MasterPeerMap;
extern CRITICAL_SECTION     cs_MyObjectList;
extern CRITICAL_SECTION     cs_MasterPeerObjects;

////////////////////////////////////////////
//
// EnumPeopleNearMe
//
////////////////////////////////////////////
LRESULT EnumPeopleNearMe(void)
{
	//-- 送信データフォーマット
	//| count | GUID(DeviceId) | cbSize | pData(UserName) | GUID(DeviceId) | cbSize | pData(UserName) | ....

	LRESULT nRetValue = S_OK;       //戻り値
	DWORD dwListCount = 0;          //ユニット数
	char* ptr = NULL;               //送信パケット用メモリポインタ
	char* tmpPtr = NULL;            //送信パケット用メモリポインタ（作業用）
	LPVOID	pAddr = NULL;           //共有メモリアドレス
	HANDLE hMap = NULL;             //共有メモリハンドル
	DWORD dwCurrentMallocSize = 0;  //送信パケットサイズ
	DWORD strSize = 0;              //ユーザ名文字列長
 
	::EnterCriticalSection(&cs_MasterPeerMap);
	std::list<PeerMap>::iterator it = MasterPeerMap.begin();
	ptr = (char*)::malloc(sizeof(DWORD));
	dwListCount = MasterPeerMap.size();
	::memcpy(ptr, &dwListCount, sizeof(DWORD));
	dwCurrentMallocSize += sizeof(DWORD);
	while( it != MasterPeerMap.end() )
	{	
		strSize = (::wcslen(it->userName)+1)*sizeof(WCHAR);
		dwCurrentMallocSize += sizeof(GUID) + sizeof(DWORD) + strSize;
		ptr = (char*)::realloc(ptr, dwCurrentMallocSize);
		tmpPtr = ptr + dwCurrentMallocSize - (sizeof(GUID) + sizeof(DWORD) + strSize);
		::memcpy(tmpPtr, &it->deviceId, sizeof(GUID));
		tmpPtr += sizeof(GUID);
		::memcpy(tmpPtr, &strSize, sizeof(DWORD));
		tmpPtr += sizeof(DWORD);		
		::memcpy(tmpPtr, it->userName, strSize);

		it++;
	}
	::LeaveCriticalSection(&cs_MasterPeerMap);

	//-- 取得PNMデータの共有メモリ展開
	hMap = ::CreateFileMapping(
		(HANDLE)INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE|SEC_COMMIT,
		0, 
		0x1000, 
		FM_ENUMPEOPLENEARME );
	if(hMap == NULL)
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	pAddr = ::MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0x1000 );
	if(pAddr == NULL)
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	::memcpy(pAddr, ptr, dwCurrentMallocSize);

EXIT:
	if(nRetValue != S_OK)
	{
		if(hMap != NULL)
		{
			::CloseHandle(hMap);
		}
	}
	if(ptr != NULL)
	{
		::free(ptr);
	}

	return nRetValue;
}


////////////////////////////////////////////
//
// SetObject
//
////////////////////////////////////////////
LRESULT SetObject(PCOPYDATASTRUCT pData)
{
	Object obj;                  //オブジェクトユニット
	BOOL isExist = FALSE;        //ADD or UPDATE
	GUID guidObj;                //オブジェクトID
	DWORD dwRecvDataSize = 0;    //オブジェクトデータサイズ
	PVOID lpRecvData = NULL;     //オブジェクトデータ 
	LRESULT nRetValue = S_OK;    //戻り値
	std::list<Object>::iterator  it;
	
	//+++ オブジェクトデータの取得 +++
	char* ptr = (char*)pData->lpData;
	memcpy(&guidObj, ptr, sizeof(GUID));
	ptr += sizeof(GUID);
	memcpy(&dwRecvDataSize, ptr, sizeof(DWORD));
	ptr += sizeof(DWORD);
	lpRecvData = ::malloc(dwRecvDataSize);
	if(lpRecvData == NULL)
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}
	memcpy(lpRecvData, ptr, dwRecvDataSize);

	//+++ オブジェクトデータのセット +++
	::ZeroMemory(&obj, sizeof(obj));
	obj.objectId = guidObj;
	obj.cbData   = dwRecvDataSize;
	obj.pbData   = lpRecvData;

	//+++ オブジェクトデータのマスタ登録 +++
	::EnterCriticalSection(&cs_MyObjectList);
	it = MyObjectList.begin();
	while( it != MyObjectList.end() )
	{
		if(it->objectId == guidObj)
		{
			//+++ オブジェクトのアップデート +++
			isExist = TRUE;
			if(it->pbData != NULL)
			{
				::free(it->pbData);
			}			
			it->objectSeq++;
			it->cbData = dwRecvDataSize;
			it->pbData = lpRecvData;
		}
		it++;
	}
	if(!isExist)
	{
		//+++ オブジェクトの新規追加 +++
		MyObjectList.push_back(obj);
	}			
	::LeaveCriticalSection(&cs_MyObjectList);

	//+++ HELLOメッセージで更新通知+++
	nRetValue = HelloSend();

EXIT:
	return nRetValue;
}


////////////////////////////////////////////
//
// DeleteObject
//
////////////////////////////////////////////
LRESULT DeleteObject(PCOPYDATASTRUCT pData)
{
	GUID guidObj;                 //削除対象オブジェクトID
	BOOL IsDeleted = FALSE;       //削除実績有無
	LRESULT nRetValue = S_OK;     //戻り値

	//-- 削除対象オブジェクトのGUIDを取得
	char* ptr = (char*)pData->lpData;
	memcpy(&guidObj, ptr, sizeof(GUID));
	
	//-- 対象オブジェクトを検索し削除する
	::EnterCriticalSection(&cs_MyObjectList);
	std::list<Object>::iterator it = MyObjectList.begin();
	while( it != MyObjectList.end() )
	{
		if(it->objectId == guidObj)
		{
			if(it->pbData != NULL)
			{
				::free(it->pbData);
			}			
			it = MyObjectList.erase(it);
			IsDeleted = TRUE;	
		} 
		else 
		{
			it++;
		}
	}
	::LeaveCriticalSection(&cs_MyObjectList);
	if(!IsDeleted)
	{		
		goto EXIT;
	}
	//-- HELLOメッセージで更新通知
	nRetValue = HelloSend();

EXIT:
	return nRetValue;
}

////////////////////////////////////////////
//
// EnumObjects
//
////////////////////////////////////////////
LRESULT EnumObjects(PCOPYDATASTRUCT pData)
{
	GUID guidObject;
	GUID guidDevice;
	char* ptr = NULL;

	switch(pData->dwData)
	{
	case FLG_OBJECT_ENUM1:
		return EnumMyObjects(NULL);
	case FLG_OBJECT_ENUM2:
		ptr = (char*)pData->lpData;
		memcpy(&guidObject, ptr, sizeof(GUID));
		return EnumMyObjects(&guidObject);
	case FLG_OBJECT_ENUM3:
		ptr = (char*)pData->lpData;
		memcpy(&guidDevice, ptr, sizeof(GUID));
		return EnumPnmObjects(&guidDevice, NULL);
	case FLG_OBJECT_ENUM4:
		ptr = (char*)pData->lpData;
		memcpy(&guidDevice, ptr, sizeof(GUID));
		ptr += sizeof(GUID);
		memcpy(&guidObject, ptr, sizeof(GUID));
		return EnumPnmObjects(&guidDevice, &guidObject);
	default:
		return PEER_S_OBJECT_NOTEXIST;		
	}
}

////////////////////////////////////////////
//
// EnumMyObjects
//
////////////////////////////////////////////
LRESULT EnumMyObjects(GUID* guidObject)
{
	//-- 送信データフォーマット
    //| count | GUID | cbSize | pData | GUID | cbSize | pData | ....

	char* ptr = NULL;                //送信パケット用メモリポインタ
	char* tmpPtr = NULL;             //送信パケット用メモリポインタ(作業用)
	LPVOID	pAddr;                   //共有メモリアドレス
	HANDLE hMap;                     //共有メモリハンドル
	DWORD dwCurrentMallocSize = 0;   //送信パケットサイズ
	DWORD dwListCount = 0;           //ユニット数
	LRESULT nRetValue = S_OK;        //戻り値

	//-- MyObjectデータの取得
	::EnterCriticalSection(&cs_MyObjectList);
	std::list<Object>::iterator it = MyObjectList.begin();
	ptr = (char*)::malloc(sizeof(DWORD));
	dwListCount = MyObjectList.size();
	::memcpy(ptr, &dwListCount, sizeof(DWORD));
	dwCurrentMallocSize += sizeof(DWORD);	 
	while( it != MyObjectList.end() )
	{
		if(guidObject == NULL ||  (guidObject != NULL && it->objectId == *guidObject))
		{
			dwCurrentMallocSize += sizeof(GUID) + sizeof(DWORD) + it->cbData;
			ptr = (char*)::realloc(ptr, dwCurrentMallocSize);
			tmpPtr = ptr + dwCurrentMallocSize - (sizeof(GUID) + sizeof(DWORD) + it->cbData);
			::memcpy(tmpPtr, &it->objectId, sizeof(GUID));
			tmpPtr += sizeof(GUID);
			::memcpy(tmpPtr, &it->cbData, sizeof(DWORD));
			tmpPtr += sizeof(DWORD);
			::memcpy(tmpPtr, it->pbData, it->cbData);
		}
		it++;
	}
	::LeaveCriticalSection(&cs_MyObjectList);

	//-- 取得オブジェクトデータの共有メモリ展開
	hMap = ::CreateFileMapping(
		(HANDLE)INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE|SEC_COMMIT,
		0, 
		0x1000, 
		FM_ENUMOBJECTS );
	if(hMap == NULL)
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	pAddr = ::MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0x1000 );
	if(pAddr == NULL)
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	::memcpy(pAddr, ptr, dwCurrentMallocSize);

EXIT:
	if(nRetValue != S_OK)
	{
		if(hMap != NULL)
		{
			::CloseHandle(hMap);
		}
	}
	if(ptr != NULL)
	{
		::free(ptr);
	}
	return nRetValue;
}


////////////////////////////////////////////
//
// EnumPnmObjects
//
////////////////////////////////////////////
LRESULT EnumPnmObjects(GUID* guidDevice, GUID* guidObject)
{
	//-- 送信データフォーマット
    //| count | GUID | cbSize | pData | GUID | cbSize | pData | ....
	LRESULT nRetValue = S_OK;              //戻り値
	LPVOID	pAddr = NULL;                  //共有メモリアドレス
	HANDLE hMap = NULL;                    //共有メモリハンドル
	BOOL IsExistDeviceId = FALSE;          //デバイスIDの存在有無
	BOOL IsExistObjectId = FALSE;          //オブジェクトIDの存在有無
	BOOL IsSelectObjectId = FALSE;         //オブジェクトIDの指定有無
	SOCKET conSock = INVALID_SOCKET;       //オブジェクト取得用TCPソケット
	struct sockaddr_in6 to;                //送信先アドレス
	struct sockaddr_in6 from;              //送信元アドレス
	int len = 0;                           //アドレス構造体サイズ
	char send[SIZEOF_WCHAR+SIZEOF_GUID+1]; //オブジェク取得依頼パケット
	char recv[MAX_RECV_OBJECTS];           //全オブジェクトデータ	
	DWORD dwSendBufSize = 0;               //送信データサイズ
	std::list<PeerMap>::iterator it;      
	std::list<ObjectMap>::iterator it2;

	//-- デバイスIDの指定は必須 
	if(IsEmptyGuid(guidDevice))
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}

	//++ デバイスの存在チェック +++
	::EnterCriticalSection(&cs_MasterPeerMap);
	it = MasterPeerMap.begin();
	while( it != MasterPeerMap.end() )
	{
		if(it->deviceId == *guidDevice)
		{
			IsExistDeviceId = TRUE;
			from = it->address;
		}
		it++;
	}
	::LeaveCriticalSection(&cs_MasterPeerMap);
	if(!IsExistDeviceId)
	{
		nRetValue = PEER_S_OBJECT_NOTEXIST;
		goto EXIT;
	}

	//++ オブジェクトの存在チェック +++
	if(!IsEmptyGuid(guidObject))
	{
		::EnterCriticalSection(&cs_MasterPeerObjects);
		it2 = MasterPeerObjects.begin();
		while( it2 != MasterPeerObjects.end() )
		{
			if(it2->objectId == *guidObject)
			{
				IsExistObjectId = TRUE;
			}
			it2++;
		}
		::LeaveCriticalSection(&cs_MasterPeerObjects);
		IsSelectObjectId = TRUE;
	}
	else
	{
		IsSelectObjectId = FALSE;
	}
	if(IsSelectObjectId && !IsExistObjectId)
	{
		nRetValue  = PEER_S_OBJECT_NOTEXIST;
		goto EXIT;
	}
				
	//+++ TCPソケット作成 +++
	if ((conSock = ::socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

	//++データ取得+++				
	::ZeroMemory((char *)&to, sizeof(to));
	::memcpy(&to, &from, sizeof(struct sockaddr_in6));
	to.sin6_port = htons(TCP_PORT);

	//接続
	if(::connect(conSock, (struct sockaddr *)&to, sizeof(struct sockaddr_in6)) == SOCKET_ERROR)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}
			
	//オブジェクト要求をSEND	
	::ZeroMemory(send, sizeof(send));
	if(IsSelectObjectId)
	{
		::memset(send, 'A', sizeof(char));
		::memcpy(send+sizeof(char), guidObject, sizeof(GUID));
		dwSendBufSize = sizeof(char)+sizeof(GUID);
	} 
	else 
	{
		::memset(send, 'B', sizeof(char));
		dwSendBufSize = sizeof(char);
	}		
	if(::send(conSock, send, dwSendBufSize, 0) == -1)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

	//-- オブジェクトをRECV
	::ZeroMemory(recv, sizeof(recv));
	if( (len = ::recv(conSock, recv, sizeof(recv), 0)) == -1)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

	//-- 取得オブジェクトデータの共有メモリ展開
	hMap = ::CreateFileMapping(
				(HANDLE)INVALID_HANDLE_VALUE,
				NULL,
				PAGE_READWRITE|SEC_COMMIT,
				0, 
				0x1000, 
				FM_ENUMOBJECTS );
	if(hMap == NULL)
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	pAddr = ::MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0x1000 );
	if(pAddr == NULL)
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}
	::memcpy(pAddr, recv, len);

EXIT:
	if(conSock != INVALID_SOCKET)
	{
		::closesocket(conSock);
	}
	if(nRetValue != S_OK)
	{
		if(hMap != NULL)
		{
			::CloseHandle(hMap);
		}
	}

	return nRetValue;
}

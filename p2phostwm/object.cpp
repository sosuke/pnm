/************************************************************/
/* [PNM/Object�v������]                                     */
/*  + API�s���̂��߂̋����ȑΉ�                             */
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
	//-- ���M�f�[�^�t�H�[�}�b�g
	//| count | GUID(DeviceId) | cbSize | pData(UserName) | GUID(DeviceId) | cbSize | pData(UserName) | ....

	LRESULT nRetValue = S_OK;       //�߂�l
	DWORD dwListCount = 0;          //���j�b�g��
	char* ptr = NULL;               //���M�p�P�b�g�p�������|�C���^
	char* tmpPtr = NULL;            //���M�p�P�b�g�p�������|�C���^�i��Ɨp�j
	LPVOID	pAddr = NULL;           //���L�������A�h���X
	HANDLE hMap = NULL;             //���L�������n���h��
	DWORD dwCurrentMallocSize = 0;  //���M�p�P�b�g�T�C�Y
	DWORD strSize = 0;              //���[�U��������
 
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

	//-- �擾PNM�f�[�^�̋��L�������W�J
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
	Object obj;                  //�I�u�W�F�N�g���j�b�g
	BOOL isExist = FALSE;        //ADD or UPDATE
	GUID guidObj;                //�I�u�W�F�N�gID
	DWORD dwRecvDataSize = 0;    //�I�u�W�F�N�g�f�[�^�T�C�Y
	PVOID lpRecvData = NULL;     //�I�u�W�F�N�g�f�[�^ 
	LRESULT nRetValue = S_OK;    //�߂�l
	std::list<Object>::iterator  it;
	
	//+++ �I�u�W�F�N�g�f�[�^�̎擾 +++
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

	//+++ �I�u�W�F�N�g�f�[�^�̃Z�b�g +++
	::ZeroMemory(&obj, sizeof(obj));
	obj.objectId = guidObj;
	obj.cbData   = dwRecvDataSize;
	obj.pbData   = lpRecvData;

	//+++ �I�u�W�F�N�g�f�[�^�̃}�X�^�o�^ +++
	::EnterCriticalSection(&cs_MyObjectList);
	it = MyObjectList.begin();
	while( it != MyObjectList.end() )
	{
		if(it->objectId == guidObj)
		{
			//+++ �I�u�W�F�N�g�̃A�b�v�f�[�g +++
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
		//+++ �I�u�W�F�N�g�̐V�K�ǉ� +++
		MyObjectList.push_back(obj);
	}			
	::LeaveCriticalSection(&cs_MyObjectList);

	//+++ HELLO���b�Z�[�W�ōX�V�ʒm+++
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
	GUID guidObj;                 //�폜�ΏۃI�u�W�F�N�gID
	BOOL IsDeleted = FALSE;       //�폜���їL��
	LRESULT nRetValue = S_OK;     //�߂�l

	//-- �폜�ΏۃI�u�W�F�N�g��GUID���擾
	char* ptr = (char*)pData->lpData;
	memcpy(&guidObj, ptr, sizeof(GUID));
	
	//-- �ΏۃI�u�W�F�N�g���������폜����
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
	//-- HELLO���b�Z�[�W�ōX�V�ʒm
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
	//-- ���M�f�[�^�t�H�[�}�b�g
    //| count | GUID | cbSize | pData | GUID | cbSize | pData | ....

	char* ptr = NULL;                //���M�p�P�b�g�p�������|�C���^
	char* tmpPtr = NULL;             //���M�p�P�b�g�p�������|�C���^(��Ɨp)
	LPVOID	pAddr;                   //���L�������A�h���X
	HANDLE hMap;                     //���L�������n���h��
	DWORD dwCurrentMallocSize = 0;   //���M�p�P�b�g�T�C�Y
	DWORD dwListCount = 0;           //���j�b�g��
	LRESULT nRetValue = S_OK;        //�߂�l

	//-- MyObject�f�[�^�̎擾
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

	//-- �擾�I�u�W�F�N�g�f�[�^�̋��L�������W�J
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
	//-- ���M�f�[�^�t�H�[�}�b�g
    //| count | GUID | cbSize | pData | GUID | cbSize | pData | ....
	LRESULT nRetValue = S_OK;              //�߂�l
	LPVOID	pAddr = NULL;                  //���L�������A�h���X
	HANDLE hMap = NULL;                    //���L�������n���h��
	BOOL IsExistDeviceId = FALSE;          //�f�o�C�XID�̑��ݗL��
	BOOL IsExistObjectId = FALSE;          //�I�u�W�F�N�gID�̑��ݗL��
	BOOL IsSelectObjectId = FALSE;         //�I�u�W�F�N�gID�̎w��L��
	SOCKET conSock = INVALID_SOCKET;       //�I�u�W�F�N�g�擾�pTCP�\�P�b�g
	struct sockaddr_in6 to;                //���M��A�h���X
	struct sockaddr_in6 from;              //���M���A�h���X
	int len = 0;                           //�A�h���X�\���̃T�C�Y
	char send[SIZEOF_WCHAR+SIZEOF_GUID+1]; //�I�u�W�F�N�擾�˗��p�P�b�g
	char recv[MAX_RECV_OBJECTS];           //�S�I�u�W�F�N�g�f�[�^	
	DWORD dwSendBufSize = 0;               //���M�f�[�^�T�C�Y
	std::list<PeerMap>::iterator it;      
	std::list<ObjectMap>::iterator it2;

	//-- �f�o�C�XID�̎w��͕K�{ 
	if(IsEmptyGuid(guidDevice))
	{
		nRetValue = E_INVALIDARG;
		goto EXIT;
	}

	//++ �f�o�C�X�̑��݃`�F�b�N +++
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

	//++ �I�u�W�F�N�g�̑��݃`�F�b�N +++
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
				
	//+++ TCP�\�P�b�g�쐬 +++
	if ((conSock = ::socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

	//++�f�[�^�擾+++				
	::ZeroMemory((char *)&to, sizeof(to));
	::memcpy(&to, &from, sizeof(struct sockaddr_in6));
	to.sin6_port = htons(TCP_PORT);

	//�ڑ�
	if(::connect(conSock, (struct sockaddr *)&to, sizeof(struct sockaddr_in6)) == SOCKET_ERROR)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}
			
	//�I�u�W�F�N�g�v����SEND	
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

	//-- �I�u�W�F�N�g��RECV
	::ZeroMemory(recv, sizeof(recv));
	if( (len = ::recv(conSock, recv, sizeof(recv), 0)) == -1)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

	//-- �擾�I�u�W�F�N�g�f�[�^�̋��L�������W�J
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

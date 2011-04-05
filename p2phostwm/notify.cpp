/************************************************************/
/* [�C�x���g�f�[�^�Ǘ�]                                     */
/*   + SetEventNotify                                       */
/*   + InitDeviceProfile                                    */
/*   + WndProc                                              */
/*   + WmSignInProc                                         */
/*   + WmSignOutProc                                        */
/************************************************************/
//
//�y�C�x���g�f�[�^�������\���z
//   | EventType | EventAction | PEER_ID | count | GUID | cbSize | pData | GUID | cbSize | pData | ...
//                                       ���������炪���ۂ̃C�x���g�f�[�^�i��{�I�Ƀf�[�^�͂P�j
//
#include "stdafx.h"
#include "p2phostwm.h"

//--�@�O���[�o���ϐ�
BOOL g_IsWatchObject = FALSE;   //Object�C�x���g�Ď��t���O
BOOL g_IsWatchPnm = FALSE;      //PNM�C�x���g�Ď��t���O
HANDLE hRemoteEvent;            //�v���Z�X�ԃC�x���g

////////////////////////////////////////////
//
// SetEventNotify
//
////////////////////////////////////////////
HRESULT SetEventNotify(DWORD dwEventType, DWORD dwEventAction, GUID* deviceId, PVOID lpEventData, DWORD dwDataSize)
{	
	DBGOUTEX2("[INFO0001]SetEventNotify Started.\n");

	HRESULT nRetValue = S_OK;    //�߂�l
	HANDLE hMap = NULL;          //���L�������n���h��
	LPVOID pAddr = NULL;         //���L�������A�h���X
	char* ptr = NULL;            //��Ɨp�|�C���^

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
	//���L�������n���h���͍���g�p���邽�߂ɂ����ŊJ�����Ȃ�
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

	HRESULT nRetValue = S_OK;    //�߂�l
	DWORD dwUserNameSize = 0;    //���[�U����
	DWORD dwDataSize = 0;        //PNM�C�x���g�f�[�^��
	char* lpData = NULL;         //PNM�C�x���g�f�[�^
	char* ptr = NULL;            //��Ɨp�|�C���^
	DWORD count = 1;             //PNM�C�x���g�̏ꍇ���j�b�g����1�ł��邱�Ƃ��O��

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
	// + DELETE�̏ꍇ
	//   {�C�x���g�f�[�^} = �I�u�W�F�N�g�₢���킹���v���C
	// + Add/Update�̏ꍇ **
	//   | count | OBJECT_GUID | cbSize(OBJECT_SIZE) | pData(OBJECT_DATA) | 

	HRESULT nRetValue = S_OK;              //�߂�l
	struct sockaddr_in6 to;                //OBJECT�₢���킹��A�h���X
	SOCKET conSock = INVALID_SOCKET;       //OBJECT�v���p�\�P�b�g(TCP)
	DWORD dwEmptyObjectSize = 0;           //���f�[�^�T�C�Y(0)
	DWORD count = 1;                       //OBJECT_DEL�C�x���g�̏ꍇ���j�b�g����1�ł��邱�Ƃ��O��
	char send[SIZEOF_WCHAR+SIZEOF_GUID+1]; //�I�u�W�F�N�擾�˗��p�P�b�g
	char* lpData = NULL;                   //�C�x���g�f�[�^�|�C���^
	DWORD dwDataSize = 0;                  //�C�x���g�f�[�^�T�C�Y
	char* ptr = NULL;                      //��Ɨp�|�C���^
	char recv[MAX_RECV_OBJECTS];           //�S�I�u�W�F�N�g�f�[�^		
	int len = 0;                           //�p�P�b�g��M�f�[�^�T�C�Y

	if(g_IsWatchObject)
	{
		//-- ������
		::ZeroMemory(&to, sizeof(to));
		::ZeroMemory(send, sizeof(send));
		::ZeroMemory(recv, sizeof(recv));
			
		switch(actionType)
		{
		case PEER_CHANGE_ADDED:
		case PEER_CHANGE_UPDATED:
			GetPeerAddress(*deviceId, &to);
			to.sin6_port = htons(TCP_PORT);
			
			//+++ TCP�\�P�b�g�쐬 +++
			if ((conSock = ::socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
				nRetValue = PEER_S_NO_CONNECTIVITY;
				goto EXIT;
			}
			//+++ �ڑ� +++
			if(::connect(conSock, (struct sockaddr *)&to, sizeof(struct sockaddr_in6)) == SOCKET_ERROR)
			{
				DBGOUTEX2("[EROR1002]connect failed\n");
				nRetValue = PEER_S_NO_CONNECTIVITY;
				goto EXIT;
			}
			//+++ �I�u�W�F�N�g�v����SEND +++
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
	HRESULT nRetValue = S_OK;  //�߂�l
	DWORD dwFlags = 0;         //�C�x���g�����グ�t���O

	//+++ �C�x���g�n���h���̎擾 +++
	char* ptr = (char*)pData->lpData;
	memcpy(&hRemoteEvent, ptr, sizeof(HANDLE));
	ptr += sizeof(HANDLE);
	memcpy(&dwFlags, ptr, sizeof(DWORD));
	
	//+++ ��ԊĎ��t���O�̃Z�b�g +++
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
/************************************************************/
/* [Object��M�܂��]                                       */
/*   + accept_func                                          */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

//-- �O���Q��
extern SOCKET              tcp_sd;
extern BOOL                g_bIsSignIn;
extern BOOL                g_bIsEnd_accept;
extern std::list<Object>   MyObjectList;
extern CRITICAL_SECTION    cs_g_bIsSignIn;
extern CRITICAL_SECTION    cs_g_bIsEnd_accept;
extern CRITICAL_SECTION    cs_MyObjectList;

////////////////////////////////////////////
//
// accept_func
//
////////////////////////////////////////////
DWORD WINAPI accept_func(LPVOID pParam)
{
	struct sockaddr_in6  from;         //OBJECT�擾�v����
	int len = 0;                       //sockaddr_in6�\���̃T�C�Y
	char recv[MAX_RECVBUF_SIZE];       //��M�f�[�^	
	char chDis = '\0';                 //OBJECT�p�P�b�g���ʎq
	BOOL IsRequestAllObjects = FALSE;  //�S�I�u�W�F�N�g�擾�v���p�P�b�g���ۂ�
	GUID reqObjectId;                  //�擾�v���I�u�W�F�N�g��GUID
	char* ptr = NULL;                  //���M�p�P�b�g�p�������|�C���^
	char* tmpPtr = NULL;               //���M�p�P�b�g�p�������|�C���^�i��Ɨp�j
	DWORD dwCurrentMallocSize = 0;     //���M�p�P�b�g�T�C�Y
	fd_set org_accept_fd, accept_fd;   //SELECT�p�f�B�X�N���v�^
	struct timeval waitval;	           //�^�C���A�E�g�p
	SOCKET remoteSocket;               //�ڑ��\�P�b�g(TCP)
	int nRet = 0;                      //�߂�l�i��Ɨp�j
	std::list<Object>::iterator it;    //iterator

	//-- ���񎞏�����
	remoteSocket = INVALID_SOCKET;
	FD_ZERO(&org_accept_fd);
	FD_SET(tcp_sd, &org_accept_fd);
	::ZeroMemory(&waitval, sizeof(waitval));
	waitval.tv_sec  = SELECT_WAIT_TIME_SEC;
	waitval.tv_usec = SELECT_WAIT_TIME_USEC;

	while(1)
	{
		//-- �X���b�h�I���v��������ꍇ�̓��[�v�𔲂���
		if(::TryEnterCriticalSection(&cs_g_bIsEnd_accept))
		{
			if(g_bIsEnd_accept)
			{
				::LeaveCriticalSection(&cs_g_bIsEnd_accept);
				break;
			}
			else
			{
				::LeaveCriticalSection(&cs_g_bIsEnd_accept);
			}
		}

		//-- SignIn���ĂȂ��ꍇ�͉������������[�v��
		if(::TryEnterCriticalSection(&cs_g_bIsSignIn))
		{
			if(!g_bIsSignIn)
			{
				::LeaveCriticalSection(&cs_g_bIsSignIn);
				::Sleep(NEXT_ACCEPT_WAIT);
				goto ROOP_EXIT;
			}
			else
			{
				::LeaveCriticalSection(&cs_g_bIsSignIn);
			}
		}

		//-- ���[�v���ɏ����� 
		dwCurrentMallocSize = 0;
		::ZeroMemory(recv, sizeof(recv));
		::ZeroMemory(&reqObjectId, sizeof(reqObjectId));
		::memcpy(&accept_fd, &org_accept_fd, sizeof(org_accept_fd));

		//-- connection�L���[�`�F�b�N�iSELECT�j
		nRet = ::select(FD_SETSIZE, &accept_fd, NULL, NULL, &waitval);
		if(nRet == 0)
		{
			//�^�C���A�E�g�̏ꍇ�͎����[�v��
			goto ROOP_EXIT;
		}

		//-- TCP�R�l�N�g�ڑ��iACCEPT�j
		len = sizeof(from);
		remoteSocket = ::accept(tcp_sd, (struct sockaddr *)&from ,&len);
		if(remoteSocket == INVALID_SOCKET)
		{
			//accept�G���[���͎����[�v��
			goto ROOP_EXIT;
		}

		//-- RCP�\�P�b�g��M�irecv�j --
		if( (len = ::recv(remoteSocket, (char*)recv, sizeof(recv), 0)) < sizeof(char))
		{
			//recv�G���[���̓\�P�b�g���N���[�Y������
			goto ROOP_EXIT;
		}
		//recv[len/sizeof(WCHAR)]=L'\0'; //���̏����͏����������S�ɂ�邱�Ƃŕs�v�Ƃ���
		
		//-- OBJECT�p�P�b�g��ʂ̔���
		::memcpy(&chDis, recv, sizeof(chDis));
		if(chDis == 'A') 
		{
			//'A'�Ȃ��OBJECT_ID�̎w�肠��
			IsRequestAllObjects = FALSE;
		}
		else if(chDis == 'B') 
		{
			//'B'�Ȃ��OBJECT_ID�̎w��Ȃ��A�SObject�̗v��
			IsRequestAllObjects = TRUE;
		}
		else
		{
			//���ʎq��'A' or 'B'�łȂ���΃G���[�Ƃ��Ď����[�v��
			goto ROOP_EXIT;
		}
		if(!IsRequestAllObjects)
		{
			::memcpy(&reqObjectId, recv+sizeof(char), sizeof(GUID));
		}
		
		//-- ���M�p�p�P�b�g�f�[�^�𐶐�
		::EnterCriticalSection(&cs_MyObjectList);
		it = MyObjectList.begin();
		ptr = (char*)::malloc(sizeof(DWORD));
		DWORD dwListCount = MyObjectList.size();
		::memcpy(ptr, &dwListCount, sizeof(DWORD));
		dwCurrentMallocSize += sizeof(DWORD);
		while( it != MyObjectList.end() )
		{
			if(!IsRequestAllObjects)
			{
				//�w�肳�ꂽID�̃I�u�W�F�N�g�f�[�^��ԐM����i�����̍l���������͈Ⴄ�̂����B�B�B�j
				if(it->objectId == reqObjectId)
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
			}
			else
			{
				//�ێ����Ă���S�I�u�W�F�N�g�f�[�^��ԐM����
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

		//-- �I�u�W�F�N�g�f�[�^�ԐM
		if(::send(remoteSocket, ptr, dwCurrentMallocSize,0) == -1)
		{
			//�G���[�̏ꍇ�ł����̂܂ܑ��s�i�ȍ~���������Ȃ����߁j
		}		

ROOP_EXIT:
		//-- �㏈��
		if(remoteSocket != INVALID_SOCKET)
		{
			::closesocket(remoteSocket);
		}		
		if(ptr != NULL)
		{
			::free(ptr);
			ptr = NULL;
		}	
	}
	return 0;
}
//���O�o�͂�����Ό��\�֗��B���̍ۂɂ̓��b�N�����Y��Ȃ��B
/************************************************************/
/* [���Probe�z�M]                                          */
/*   + frame_func                                           */
/*   + RefreshPeerMap                                       */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

//-- �O���Q��
extern SOCKET               udp_sd;
extern BOOL                 g_bIsSignIn;
extern BOOL                 g_bIsEnd_frame;
extern std::list<PeerMap>   MasterPeerMap;
extern std::list<ObjectMap> MasterPeerObjects;
extern CRITICAL_SECTION     cs_g_bIsSignIn;
extern CRITICAL_SECTION     cs_MasterPeerMap;
extern CRITICAL_SECTION     cs_MasterPeerObjects;
extern CRITICAL_SECTION     cs_g_bIsEnd_frame;

////////////////////////////////////////////
//
// frame_func
//
////////////////////////////////////////////
DWORD WINAPI frame_func(LPVOID pParam)
{
	LARGE_INTEGER nFreq, nBefore, nAfter;
	DWORD dwTime;
	int count = 0;

	//+++ ����I���M�̃`�F�b�N +++
	QueryPerformanceFrequency((LARGE_INTEGER *)&nFreq);
	while(1){
		//-- �X���b�h�I���v��������ꍇ�̓��[�v�𔲂���
		if(::TryEnterCriticalSection(&cs_g_bIsEnd_frame))
		{
			if(g_bIsEnd_frame)
			{
				::LeaveCriticalSection(&cs_g_bIsEnd_frame);
				break;
			}
			else
			{
				::LeaveCriticalSection(&cs_g_bIsEnd_frame);
			}			
		}

		//-- SignIn���ĂȂ��ꍇ�͉������������[�v��
		if(::TryEnterCriticalSection(&cs_g_bIsSignIn))
		{
			if(!g_bIsSignIn)
			{
				::LeaveCriticalSection(&cs_g_bIsSignIn);
				::Sleep(NEXT_SEND_WAIT);
				continue;
			}
			else
			{
				::LeaveCriticalSection(&cs_g_bIsSignIn);
			}
		}

		QueryPerformanceCounter((LARGE_INTEGER *)&nBefore);

		//-- PROBE���b�Z�[�W�̑��M
		ProbeSend();

		if( count == CHECK_FRAME_CYCLE )
		{
			//�����C���N�������g������萔�𒴂����ꍇ�͂��̃G���g���̓h���b�v������
			RefreshPeerMap();
			count = 0;
		}

		QueryPerformanceCounter((LARGE_INTEGER *)&nAfter);
		dwTime = (DWORD)((nAfter.QuadPart - nBefore.QuadPart) * 1000 / nFreq.QuadPart);
		if(FRAME_TIME-dwTime > 0 )
		{
			::Sleep(FRAME_TIME-dwTime);
		}
		count++;
	}
	return 0;
}


////////////////////////////////////////////
//
// RefreshPeerMap
//
////////////////////////////////////////////
void RefreshPeerMap(void)
{
	GUID guidDevice;
	GUID guidObject;
	WCHAR userId[MAX_USERNAME_SIZE+1];

	//+++ TTL�����̃f�o�C�X�G���g�����폜 +++
	::EnterCriticalSection(&cs_MasterPeerMap);
	std::list<PeerMap>::iterator it = MasterPeerMap.begin();
	while( it != MasterPeerMap.end() )  
	{
		it->ttl++;
		if(it->ttl > MAX_REFRESH_TTL)
		{
			::ZeroMemory(userId, sizeof(userId));
			guidDevice = it->deviceId;
			::wcscpy(userId, it->userName);
			it = MasterPeerMap.erase(it);

			//+++ PNM DELETED�C�x���g�̔��s +++
			PnmEventNotify(PEER_CHANGE_DELETED, &guidDevice, userId);
		}
		else
		{
			it++;
		}
	}
	::LeaveCriticalSection(&cs_MasterPeerMap);

	//+++ �Y������f�o�C�X�̃I�u�W�F�N�g�}�b�v���폜 +++
	::EnterCriticalSection(&cs_MasterPeerObjects);
	std::list<ObjectMap>::iterator it2 = MasterPeerObjects.begin();
	while( it2 != MasterPeerObjects.end() ) 
	{
		if(it2->deviceId == guidDevice)
		{
			guidObject = it2->objectId;
			it2 = MasterPeerObjects.erase(it2);

			//+++ OBJECT DELETE�C�x���g�̔��s +++
			ObjectEventNotify(PEER_CHANGE_DELETED, &guidObject, &guidDevice);
		} 
		else 
		{
			it2++;
		}		
	}
	::LeaveCriticalSection(&cs_MasterPeerObjects);
}

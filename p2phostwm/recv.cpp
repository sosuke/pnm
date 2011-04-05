//(��recv_func�̃��\�[�X�Ǘ����܂��Â��BguidDevice�Ń����^�C���G���[����)
/************************************************************/
/* [�p�P�b�g��M�X���b�h����]                               */
/*   + recv_func                                            */
/*   + RenewObjectMap                                       */
/*   + InitializeDeleteFlagOnObjectMap                      */
/*   + CheckDeleteFlagOnObjectMap                           */
/*   + HelloProbeMatchRecv                                  */
/*   + ByeRecv                                              */
/*   + ProbeRecv                                            */
/************************************************************/

#include "stdafx.h"
#include "p2phostwm.h"

//-- �O���Q��
extern SOCKET               udp_sd;
extern BOOL                 g_bIsSignIn;
extern BOOL                 g_bIsEnd_recv;
extern std::list<PeerMap>   MasterPeerMap;
extern std::list<ObjectMap> MasterPeerObjects;
extern CRITICAL_SECTION     cs_g_bIsSignIn;  
extern CRITICAL_SECTION     cs_g_bIsEnd_recv;  
extern CRITICAL_SECTION     cs_MasterPeerMap;  
extern CRITICAL_SECTION     cs_MasterPeerObjects;  

////////////////////////////////////////////
//
// recv_func 
//
////////////////////////////////////////////
DWORD WINAPI recv_func(LPVOID pParam)
{
    struct sockaddr_in6 from;      //��M�A�h���X�\����
	int len = 0;                   //��M���b�Z�[�W�T�C�Y
	int fromlen = 0;               //�A�h���X�\���̃T�C�Y
	WCHAR recv[MAX_RECVBUF_SIZE];  //��M�o�b�t�@
	
	GUID guidObject;               //�I�u�W�F�N�gID
	GUID guidDevice;               //�f�o�C�XID
	DWORD dwSeqNo = 0;             //�I�u�W�F�N�g����ԍ�
	DWORD dwObjectNum = 0;         //�I�u�W�F�N�g��
	fd_set org_recv_fd, recv_fd;   //SELECT�p�f�B�X�N���v�^�p
	struct timeval waitval;        //�^�C���A�E�g�p
	int nRet = 0;                  //�߂�l�i��Ɨp�j
	BSTR bstrNodeName;             //�p�P�b�g���ʃm�[�h��(HELLO/BYE/PROBE/PROBEMATCH)
	BSTR bstrDeviceId;             //�f�o�C�XID������
	BSTR bstrUserName;             //���[�U��������
	BSTR bstrAttObjectId;          //�I�u�W�F�N�gID������
	BSTR bstrAttObjectSeq;         //�I�u�W�F�N�g����ԍ�������

	//+++ ���[�v�O������ +++
	FD_ZERO(&org_recv_fd);
	FD_SET(udp_sd, &org_recv_fd);	
	::ZeroMemory(&guidDevice, sizeof(guidDevice));
	::ZeroMemory(&guidObject, sizeof(guidObject));
	::ZeroMemory(&waitval, sizeof(waitval));
	waitval.tv_sec  = SELECT_WAIT_TIME_SEC;
	waitval.tv_usec = SELECT_WAIT_TIME_USEC;
	msxml::IXMLDOMDocumentPtr pDoc("MSXML.DOMDocument");
	pDoc->put_async(VARIANT_FALSE);
	pDoc->validateOnParse = VARIANT_FALSE;

	//+++ ��M���[�v(Blocking) +++
	while(1){
		//-- �X���b�h�I���v��������ꍇ�̓��[�v�𔲂���
		if(::TryEnterCriticalSection(&cs_g_bIsEnd_recv))
		{
			if(g_bIsEnd_recv)
			{
				::LeaveCriticalSection(&cs_g_bIsEnd_recv);
				break;
			}
			else
			{
				::LeaveCriticalSection(&cs_g_bIsEnd_recv);
			}
		}

		//-- SignIn���ĂȂ��ꍇ�͉������������[�v��
		if(::TryEnterCriticalSection(&cs_g_bIsSignIn))
		{
			if(!g_bIsSignIn)
			{
				::LeaveCriticalSection(&cs_g_bIsSignIn);
				::Sleep(NEXT_RECV_WAIT);
				continue;
			}
			else
			{
				::LeaveCriticalSection(&cs_g_bIsSignIn);
			}
		}

		//-- ���[�v��������
		::ZeroMemory(recv, sizeof(recv));
		::memcpy(&recv_fd, &org_recv_fd, sizeof(org_recv_fd));

		//-- connection�L���[�`�F�b�N(SELCT)
		nRet = ::select(FD_SETSIZE, &recv_fd, NULL, NULL, &waitval);
		if(nRet == 0)
		{
			//�^�C���A�E�g�̏ꍇ�͎����[�v��
			continue;
		}

		//-- ���b�Z�[�W��M(recvfrom)
		fromlen=sizeof(from);
		len=recvfrom(udp_sd, (char *)recv, sizeof(recv), 0, (struct sockaddr *)&from, &fromlen);
		switch(len){
		case 0:
		case SOCKET_ERROR:
			break;
		default:
			pDoc->loadXML(recv);

			msxml::IXMLDOMNodePtr pBodyNode = pDoc->selectSingleNode(_bstr_t("Envelope/Body"));
			msxml::IXMLDOMNodePtr pTypeNode = pBodyNode->GetfirstChild();	
			pTypeNode->get_nodeName(&bstrNodeName);	

			if(::wcscmp(bstrNodeName, L"Probe")==0)
			{
				DBGOUTEX2("[INFO0016] PROBE RECV\n");
				ProbeRecv(from);
			} 
			else if(::wcscmp(bstrNodeName, L"Bye")==0)
			{
				DBGOUTEX2("[INFO0015] BYE RECV\n");
				//DeviceId�̎擾
				msxml::IXMLDOMNodePtr pNodeDeviceId = pTypeNode->selectSingleNode(_bstr_t("DeviceId"));
				pNodeDeviceId->get_text(&bstrDeviceId);
				GuidFromString(&guidDevice, bstrDeviceId);
				ByeRecv(guidDevice);
			} 
			else if(::wcscmp(bstrNodeName, L"Hello")==0 || ::wcscmp(bstrNodeName, L"ProbeMatch")==0)
			{	
				DBGOUTEX2("[INFO0014] HELO/PROBEMATCH RECV\n");

				//DeviceId�̎擾
				msxml::IXMLDOMNodePtr pNodeDeviceId = pTypeNode->selectSingleNode(_bstr_t("DeviceId"));
				pNodeDeviceId->get_text(&bstrDeviceId);
				GuidFromString(&guidDevice, bstrDeviceId); 

				//UserName�̎擾
				msxml::IXMLDOMNodePtr pNodeUserName = pTypeNode->selectSingleNode(_bstr_t("UserName"));
				pNodeUserName->get_text(&bstrUserName);
	
				//Object�Q�̎擾
				msxml::IXMLDOMNodeListPtr pNodeListObjects = pTypeNode->selectNodes(_bstr_t("Object"));
				dwObjectNum = pNodeListObjects->Getlength();

				if(dwObjectNum > 0)
				{
					//������
					InitializeDeleteFlagOnObjectMap(guidDevice);
					for(int i= 0; i<dwObjectNum; i++)
					{
						msxml::IXMLDOMNodePtr pObjectNode = pNodeListObjects->Getitem(i);
	
						//����id�̎擾
						msxml::IXMLDOMNodePtr pIdAttNode = pObjectNode->selectSingleNode(_bstr_t("@id"));
						pIdAttNode->get_text(&bstrAttObjectId);
						GuidFromString(&guidObject, bstrAttObjectId);
		
						//����seq�̎擾
						msxml::IXMLDOMNodePtr pSeqAttNode = pObjectNode->selectSingleNode(_bstr_t("@seq"));
						pSeqAttNode->get_text(&bstrAttObjectSeq);
						dwSeqNo = ::_wtoi(bstrAttObjectSeq);

						//-- ObjectMap���X�V
						RenewPeerObjectMap(guidObject, guidDevice, dwSeqNo);
					}
					//Delete�`�F�b�N
					CheckDeleteFlagOnObjectMap(guidDevice);
				}
				else if(dwObjectNum == 0)
				{
					//Delete�`�F�b�N(OBJECT����0�̏ꍇ)
					CheckDeleteOnObjectMapWhenEmpty(guidDevice);
				}


				//+++ PeerMap���X�V +++
				HelloProbeMatchRecv(guidDevice, bstrUserName, from);
			} 
			else 
			{
				continue;
			}


		}
	}
	return 0;
}

////////////////////////////////////////////
//
// InitializeDeleteFlagOnObjectMap
//
////////////////////////////////////////////
void InitializeDeleteFlagOnObjectMap(GUID deviceID)
{
	::EnterCriticalSection(&cs_MasterPeerObjects);
	std::list<ObjectMap>::iterator it = MasterPeerObjects.begin(); 
	while( it != MasterPeerObjects.end() ) 
	{
		if(it->deviceId == deviceID)
		{
			it->flgDel = FALSE;
		}
		it++;
	}
	::LeaveCriticalSection(&cs_MasterPeerObjects);
}

////////////////////////////////////////////
//
// CheckDeleteFlagOnObjectMap
//
////////////////////////////////////////////
void CheckDeleteFlagOnObjectMap(GUID deviceID)
{
	::EnterCriticalSection(&cs_MasterPeerObjects);
	std::list<ObjectMap>::iterator it = MasterPeerObjects.begin(); 
	while( it != MasterPeerObjects.end() ) 
	{
		if(it->deviceId == deviceID && it->flgDel == FALSE)
		{
			GUID guidObject = it->objectId;
			it = MasterPeerObjects.erase(it);

			DBGOUTEX2("[INFO0010] Object Deleted.\n");
			ObjectEventNotify(PEER_CHANGE_DELETED, &guidObject, &deviceID);
		} 
		else
		{
			it++;
		}		
	}
	::LeaveCriticalSection(&cs_MasterPeerObjects);
}

////////////////////////////////////////////
//
// CheckDeleteOnObjectMapWhenEmpty
//
////////////////////////////////////////////
void CheckDeleteOnObjectMapWhenEmpty(GUID deviceID)
{
	::EnterCriticalSection(&cs_MasterPeerObjects);
	std::list<ObjectMap>::iterator it = MasterPeerObjects.begin(); 
	while( it != MasterPeerObjects.end() ) 
	{	
		if(it->deviceId == deviceID)
		{
			GUID guidObject = it->objectId;
			it = MasterPeerObjects.erase(it);
			DBGOUTEX2("[INFO0010] Object Deleted.\n");
			ObjectEventNotify(PEER_CHANGE_DELETED, &guidObject, &deviceID);
		}
		else
		{
			it++;
		}			
	}
	::LeaveCriticalSection(&cs_MasterPeerObjects);
}


////////////////////////////////////////////
//
// RenewObjectMap
//
////////////////////////////////////////////
BOOL RenewPeerObjectMap(GUID objectID, GUID deviceID, DWORD dwSeqNo)
{
	ObjectMap addObject;
	BOOL bRetValue = TRUE;
	BOOL bIsExist = FALSE;
	
	//������
	::ZeroMemory(&addObject, sizeof(addObject));

	std::list<ObjectMap>::iterator it = MasterPeerObjects.begin(); 
	while( it != MasterPeerObjects.end() ) 
	{
		if(it->objectId == objectID && it->deviceId == deviceID)
		{
			if(it->objectSeq < dwSeqNo )
			{
				it->objectSeq = dwSeqNo;
				DBGOUTEX2("[INFO0009] Object Updated.\n");
				ObjectEventNotify(PEER_CHANGE_UPDATED, &objectID, &deviceID);
			}
			it->flgDel = TRUE;
			bIsExist = TRUE;
		}
		it++;
	}
	if(!bIsExist) 
	{
		addObject.deviceId = deviceID;
		addObject.objectId = objectID;
		addObject.objectSeq = dwSeqNo;
		addObject.flgDel = TRUE;
		MasterPeerObjects.push_back(addObject);

		DBGOUTEX2("[INFO0008] Object Added.\n");
		ObjectEventNotify(PEER_CHANGE_ADDED, &objectID, &deviceID);
	}
EXIT:
	return bRetValue;
}

////////////////////////////////////////////
//
// HelloRecv
//
////////////////////////////////////////////
BOOL HelloProbeMatchRecv(GUID deviceID, WCHAR *userName, struct sockaddr_in6 from)
{
	PeerMap addPeer;
	BOOL bIsExist = FALSE;
	WCHAR szGuid[GUID_STR_SIZE+1];
	BOOL bRetValue = TRUE;

	//+++ �������@+++
	::ZeroMemory(&addPeer, sizeof(addPeer));
	::ZeroMemory(szGuid, sizeof(szGuid));

	//+++ �Y���f�o�C�X�G���g���̑��݃`�F�b�N +++
	::EnterCriticalSection(&cs_MasterPeerMap);
	std::list<PeerMap>::iterator it = MasterPeerMap.begin();
	while( it != MasterPeerMap.end() ) 
	{
		if( deviceID == it->deviceId)
		{
			if(::wcscmp(it->userName,  userName)!=0)
			{
				//-- ���[�U���X�V
				::wcscpy(it->userName, userName); 
				//-- PNM UPDATE�C�x���g�̔��s
				DBGOUTEX2("[INFO0012] Peer Updated.\n");
				PnmEventNotify(PEER_CHANGE_ADDED, &deviceID, userName);
			}		
			bIsExist = TRUE;
			it->ttl = 0;			
			break;
		}
		it++;
	}
	::LeaveCriticalSection(&cs_MasterPeerMap);

	if(!bIsExist) 
	{		
		addPeer.deviceId = deviceID;
		addPeer.address = from;
		addPeer.ttl = 0;
		::wcscpy(addPeer.userName, userName);
		
		//-- MasterPeerMap�ɒǉ�����
		::EnterCriticalSection(&cs_MasterPeerMap);
		MasterPeerMap.push_back(addPeer);
		::LeaveCriticalSection(&cs_MasterPeerMap);

		//-- PNM ADD�C�x���g�̔��s 
		DBGOUTEX2("[INFO0011] Peer Added.\n");
		PnmEventNotify(PEER_CHANGE_ADDED, &deviceID, userName);
	}

EXIT:
	return bRetValue;
}

////////////////////////////////////////////
//
// ByeRecv
//
////////////////////////////////////////////
BOOL ByeRecv(GUID deviceId)
{
	GUID guidObject;
	WCHAR userId[MAX_USERNAME_SIZE+1];
	BOOL bRetValue = TRUE;

	//+++ ������ +++
	::ZeroMemory(userId, sizeof(userId));

	//+++ �Y���f�o�C�X�G���g���̑��݃`�F�b�N +++
	::EnterCriticalSection(&cs_MasterPeerMap);
	std::list<PeerMap>::iterator it = MasterPeerMap.begin();
	while( it != MasterPeerMap.end() ) 
	{
		if( it->deviceId == deviceId)
		{
			::wcscpy(userId, it->userName);
			it = MasterPeerMap.erase(it);

			//+++ PNM DELETE�C�x���g�̔��s +++
			DBGOUTEX2("[INFO0013] Peer Deleted.\n");
			PnmEventNotify(PEER_CHANGE_DELETED, &deviceId, userId);
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
		if(it2->deviceId == deviceId)
		{
			guidObject = it2->objectId;
			it2 = MasterPeerObjects.erase(it2);			

			//+++ OBJECT DELETE�C�x���g�̔��s +++
			DBGOUTEX2("[INFO0010] Object Deleted.\n");
			ObjectEventNotify(PEER_CHANGE_DELETED, &guidObject, &deviceId);
		} 
		else 
		{
			it2++;
		}		
	}
	::LeaveCriticalSection(&cs_MasterPeerObjects);

EXIT:
	return bRetValue;
}

////////////////////////////////////////////
//
// ProbeRecv
//
////////////////////////////////////////////
BOOL ProbeRecv(struct sockaddr_in6 from)
{
	HRESULT hRet   = S_OK;
	BOOL bRetValue = TRUE;

	//+++ Prove��M������ProbeMatch��ԓ����� +++
	hRet = ProbeMatchSend(from);
	if(hRet != S_OK)
	{
		bRetValue = FALSE;
		goto EXIT;
	}
EXIT:
	return bRetValue;
}


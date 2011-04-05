/************************************************************/
/* [p2phostwm.exe �p�P�b�g���M����]                         */
/*   + MakeHelloMsg                                         */
/*   + HelloSend                                            */
/*   + MakeByteMsg                                          */
/*   + ByeSend                                              */
/*   + MakeProbeMsg                                         */
/*   + ProbeSend                                            */
/*   + MakeProbeMatchMsg                                    */
/*   + ProbeMatchSend                                       */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

//-- �O���Q��
extern SOCKET		       udp_sd;
extern DeviceProfile       g_MyDeviceProfile;
extern std::list<Object>   MyObjectList;
extern CRITICAL_SECTION     cs_MyObjectList;
extern CRITICAL_SECTION     cs_g_MyDeviceProfile;

////////////////////////////////////////////
//
// MakeHelloMsg
//
////////////////////////////////////////////
HRESULT MakeHelloMsg(WCHAR* buf, DWORD dwBufSize)
{
	HRESULT nRetValue = S_OK;
	WCHAR guid[GUID_STR_SIZE+1];
	WCHAR wszDeviceIdElement[DEVICEID_ELEMENT_SIZE+GUID_STR_SIZE+1];  
	WCHAR wszUserNameElement[USERNAME_ELEMENT_SIZE+MAX_USERNAME_SIZE+1];
	WCHAR wszObjectIdElement[(OBJECT_ELEMENT_SIZE+GUID_STR_SIZE+MAX_OBJECTSEQ_DIGIT)*MAX_OBJECT_STORE_NUM+1];	
	std::list<Object>::iterator it;
	int nRet = 0;
	DWORD dwSeqNo = 0;

	//+++ ������ +++
	::ZeroMemory(guid, sizeof(guid));
	::ZeroMemory(wszDeviceIdElement, sizeof(wszDeviceIdElement));
	::ZeroMemory(wszUserNameElement, sizeof(wszUserNameElement));
	::ZeroMemory(wszObjectIdElement, sizeof(wszObjectIdElement));

	//+++ �f�o�C�X�v���t�@�C���̃Z�b�g+++
	::EnterCriticalSection(&cs_g_MyDeviceProfile);
	GuidToString(g_MyDeviceProfile.deviceId, guid);
	::LeaveCriticalSection(&cs_g_MyDeviceProfile);
	nRet = ::_snwprintf(wszDeviceIdElement, sizeof(wszDeviceIdElement)-1, L"<DeviceId>%s</DeviceId>", guid);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}
	::EnterCriticalSection(&cs_g_MyDeviceProfile);
	nRet = ::_snwprintf(wszUserNameElement, sizeof(wszUserNameElement)-1, L"<UserName>%s</UserName>", g_MyDeviceProfile.userName);
	::LeaveCriticalSection(&cs_g_MyDeviceProfile);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}

	//+++ �I�u�W�F�N�g�ꗗ�̃Z�b�g +++
	::EnterCriticalSection(&cs_MyObjectList);
	it = MyObjectList.begin();
	while( it != MyObjectList.end() )
	{
		GuidToString(it->objectId, guid);
		dwSeqNo = it->objectSeq;
		nRet = ::swprintf(wszObjectIdElement, L"%s<Object id=\"%s\" seq=\"%d\"/>", wszObjectIdElement, guid, dwSeqNo);		
		if(nRet < 0 )
		{
			::LeaveCriticalSection(&cs_MyObjectList);
			nRetValue = E_OUTOFMEMORY;
			goto EXIT;
		}
		it++;
	}
	::LeaveCriticalSection(&cs_MyObjectList);

	//+++ ���b�Z�[�W������ +++
	nRet = ::_snwprintf(
		buf,
		dwBufSize-1, 
		L"%s%s%s%s%s", 
		HELLO_MSGSTR_HEAD, 
		wszDeviceIdElement, 
		wszUserNameElement,
		wszObjectIdElement,
		HELLO_MSGSTR_TRAIL
		);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}

////////////////////////////////////////////
//
// HelloSend
//
////////////////////////////////////////////
HRESULT HelloSend(void)
{
    struct sockaddr_in6 to;
	INT fromlen = 0;
	HRESULT nRetValue = S_OK;
	HRESULT nRet = S_OK;
	DWORD retVal = 0;
	WCHAR sendbuf[MAX_HELLO_SENDBUF_SIZE];
	DWORD dwStrLen = 0;

	//+++ ���M��̐ݒ� +++
	::ZeroMemory(&to, sizeof(to));
    fromlen=sizeof(to);
    ::WSAStringToAddress(IPV6_MULTICAST_ADDRESS, AF_INET6, NULL, (LPSOCKADDR)&to, &fromlen);
    to.sin6_port = htons(UDP_PORT);
	
	///+++ HELLO���b�Z�[�W�\�z +++
	::ZeroMemory(sendbuf, sizeof(sendbuf));
	nRet = MakeHelloMsg(sendbuf, sizeof(sendbuf));
	if(nRet != S_OK)
	{
		nRetValue = nRet;
		goto EXIT;
	}

	///+++ HELLO���b�Z�[�W���M +++
	dwStrLen = ::wcslen(sendbuf)*sizeof(WCHAR);	
	retVal = ::sendto(udp_sd, (char *)sendbuf, dwStrLen, 0, (struct sockaddr *)&to, fromlen);
	if(retVal == SOCKET_ERROR)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}

////////////////////////////////////////////
//
// MakeProbeMsg
//
////////////////////////////////////////////
HRESULT MakeProbeMsg(WCHAR* buf, DWORD dwBufSize)
{
	HRESULT nRetValue = S_OK;
	int nRet = 0;

	nRet = ::_snwprintf(buf, dwBufSize-1, L"%s%s", PROBE_MSGSTR_HEAD, PROBE_MSGSTR_TRAIL);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}

////////////////////////////////////////////
//
// ProbeSend
//
////////////////////////////////////////////6
HRESULT ProbeSend(void)
{
	struct sockaddr_in6 to;
	INT fromlen = 0;
	WCHAR sendbuf[MAX_PROBE_SENDBUF_SIZE];
	DWORD dwStrLen = 0;
	DWORD retVal = 0;
	HRESULT nRetValue = S_OK;	
	HRESULT nRet = S_OK;

	//+++ ���M��̐ݒ� +++    
	::ZeroMemory(&to, sizeof(to));
    fromlen=sizeof(to);
    ::WSAStringToAddress(IPV6_MULTICAST_ADDRESS, AF_INET6, NULL, (LPSOCKADDR)&to, &fromlen);
    to.sin6_port = htons(UDP_PORT);

	///+++ PROBE���b�Z�[�W�\�z +++
	::ZeroMemory(sendbuf, sizeof(sendbuf));
	nRet = MakeProbeMsg(sendbuf, sizeof(sendbuf));
	if(nRet != S_OK)
	{
		nRetValue = nRet;
		goto EXIT;
	}
	
	///+++ PROBE���b�Z�[�W���M +++
	dwStrLen = ::wcslen(sendbuf)*sizeof(WCHAR);	
	retVal = sendto(udp_sd, (char *)sendbuf, dwStrLen, 0, (struct sockaddr *)&to, fromlen);
	if(retVal == SOCKET_ERROR)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}


////////////////////////////////////////////
//
// MakeByeMsg
//
////////////////////////////////////////////
HRESULT MakeByeMsg(WCHAR* buf, DWORD dwBufSize)
{
	HRESULT nRetValue = S_OK;
	int nRet = 0;
	WCHAR guid[GUID_STR_SIZE+1];
	WCHAR wszDeviceIdElement[DEVICEID_ELEMENT_SIZE+GUID_STR_SIZE+1];

	//+++ ������ +++
	::ZeroMemory(guid, sizeof(guid));
	::ZeroMemory(wszDeviceIdElement, sizeof(wszDeviceIdElement));

	::EnterCriticalSection(&cs_g_MyDeviceProfile);
	GuidToString(g_MyDeviceProfile.deviceId, guid);
	::LeaveCriticalSection(&cs_g_MyDeviceProfile);
	nRet = ::_snwprintf(wszDeviceIdElement, sizeof(wszDeviceIdElement)-1, L"<DeviceId>%s</DeviceId>", guid);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}

	nRet = ::_snwprintf(buf, dwBufSize-1, L"%s%s%s", BYE_MSGSTR_HEAD, wszDeviceIdElement, BYE_MSGSTR_TRAIL);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}

////////////////////////////////////////////
//
// ByeSend
//
////////////////////////////////////////////
HRESULT ByeSend(void)
{
    struct sockaddr_in6 to;
	INT fromlen = 0;
	WCHAR sendbuf[MAX_BYE_SENDBUF_SIZE];
	DWORD retVal = 0;
	DWORD dwStrLen = 0;
	HRESULT nRetValue = S_OK;	
	HRESULT nRet = S_OK;
	
	//+++ ���M��̐ݒ� +++
	::ZeroMemory(&to, sizeof(to));
    fromlen=sizeof(to);
    ::WSAStringToAddress(IPV6_MULTICAST_ADDRESS, AF_INET6, NULL, (LPSOCKADDR)&to, &fromlen);
    to.sin6_port = htons(UDP_PORT);

	///+++ BYE���b�Z�[�W�\�z +++
	::ZeroMemory(sendbuf, sizeof(sendbuf));
	nRet = MakeByeMsg(sendbuf, sizeof(sendbuf));
	if(nRet != S_OK)
	{
		nRetValue = nRet;
		goto EXIT;
	}

	///+++ BYE���b�Z�[�W���M +++
	dwStrLen = ::wcslen(sendbuf)*sizeof(WCHAR);	
	retVal = sendto(udp_sd, (char *)sendbuf, dwStrLen, 0, (struct sockaddr *)&to, fromlen);
	if(retVal == SOCKET_ERROR)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}


////////////////////////////////////////////
//
// MakeProbeMatchMsg
//
////////////////////////////////////////////
HRESULT MakeProbeMatchMsg(WCHAR* buf, DWORD dwBufSize)
{
	HRESULT nRetValue = S_OK;
	WCHAR guid[GUID_STR_SIZE+1];
	WCHAR wszDeviceIdElement[DEVICEID_ELEMENT_SIZE+GUID_STR_SIZE+1];
	WCHAR wszUserNameElement[USERNAME_ELEMENT_SIZE+MAX_USERNAME_SIZE+1];
	WCHAR wszObjectIdElement[(OBJECT_ELEMENT_SIZE+GUID_STR_SIZE+MAX_OBJECTSEQ_DIGIT)*MAX_OBJECT_STORE_NUM+1];
	std::list<Object>::iterator it;
	int nRet = 0;
	DWORD dwSeqNo = 0;

	//+++ ������ +++
	::ZeroMemory(guid, sizeof(guid));
	::ZeroMemory(wszDeviceIdElement, sizeof(wszDeviceIdElement));
	::ZeroMemory(wszUserNameElement, sizeof(wszUserNameElement));
	::ZeroMemory(wszObjectIdElement, sizeof(wszObjectIdElement));

	//+++ �f�o�C�X�v���t�@�C���̃Z�b�g+++
	::EnterCriticalSection(&cs_g_MyDeviceProfile);
	GuidToString(g_MyDeviceProfile.deviceId, guid);
	::LeaveCriticalSection(&cs_g_MyDeviceProfile);
	nRet = ::_snwprintf(wszDeviceIdElement, sizeof(wszDeviceIdElement)-1, L"<DeviceId>%s</DeviceId>", guid);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}
	::EnterCriticalSection(&cs_g_MyDeviceProfile);
	nRet = ::_snwprintf(wszUserNameElement, sizeof(wszUserNameElement)-1, L"<UserName>%s</UserName>", g_MyDeviceProfile.userName);
	::LeaveCriticalSection(&cs_g_MyDeviceProfile);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}

	//+++ �I�u�W�F�N�g�ꗗ�̃Z�b�g +++
	::EnterCriticalSection(&cs_MyObjectList);
	it = MyObjectList.begin();
	while( it != MyObjectList.end() )
	{
		GuidToString(it->objectId, guid);
		dwSeqNo = it->objectSeq;
		nRet = ::swprintf(wszObjectIdElement, L"%s<Object id=\"%s\" seq=\"%d\"/>", wszObjectIdElement, guid, dwSeqNo);		
		if(nRet < 0 )
		{
			::LeaveCriticalSection(&cs_MyObjectList);
			nRetValue = E_OUTOFMEMORY;
			goto EXIT;
		}
		it++;
	}
	::LeaveCriticalSection(&cs_MyObjectList);

	//+++ ���b�Z�[�W������ +++
	nRet = ::_snwprintf(
		buf, 
		dwBufSize-1, 
		L"%s%s%s%s%s", 
		PROBEMATCH_MSGSTR_HEAD, 
		wszDeviceIdElement, 
		wszUserNameElement,
		wszObjectIdElement,
		PROBEMATCH_MSGSTR_TRAIL
		);
	if(nRet < 0 )
	{
		nRetValue = E_OUTOFMEMORY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}

////////////////////////////////////////////
//
// ProbeMatchSend
//
////////////////////////////////////////////
HRESULT ProbeMatchSend(struct sockaddr_in6 from)
{
	WCHAR sendbuf[MAX_PROBEMATCH_SENDBUF_SIZE];
	INT fromlen=sizeof(from);
	HRESULT nRetValue = S_OK;
	HRESULT nRet = S_OK;
	DWORD retVal = 0;
	DWORD dwStrLen = 0;

	///+++ PROBEMATCH���b�Z�[�W�\�z +++
	::ZeroMemory(sendbuf, sizeof(sendbuf));
	nRet = MakeProbeMatchMsg(sendbuf, sizeof(sendbuf));
	if(nRet != S_OK)
	{
		nRetValue = nRet;
		goto EXIT;
	}

	///+++ PROBEMATCH���b�Z�[�W���M +++
	dwStrLen = ::wcslen(sendbuf)*sizeof(WCHAR);	
	retVal = sendto(udp_sd, (char *)sendbuf, dwStrLen, 0, (struct sockaddr *)&from, fromlen);
	if(retVal == SOCKET_ERROR)
	{
		nRetValue = PEER_S_NO_CONNECTIVITY;
		goto EXIT;
	}

EXIT:
	return nRetValue;
}


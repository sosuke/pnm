/************************************************************/
/* [Socket�܂��̊�{����]                                 */
/*   + InitConnection                                       */
/************************************************************/

//[memo]
// setsockopt �֐��� bind �֐��̑O�ɌĂ΂��Ȃ�΁A bind ���N����܂�TCP/IP �I�v�V������ TCP/IP �Əƍ�����܂���B
// ���̏ꍇ�A setsockopt �֐��Ăяo���́A��ɐ������܂��B
// �������Abind �֐��Ăяo���́A���s���Ă��鏉���� setsockopt �̂��ߎ��s���邱�Ƃ����肦�܂��B

#include "stdafx.h"
#include "p2phostwm.h"

SOCKET		udp_sd;     //����M�ėp
SOCKET		tcp_sd;     //��M��p�A���M��Accept�߂�l�𗘗p

////////////////////////////////////////////
//
// InitConnection
//
////////////////////////////////////////////
BOOL InitConnection()
{
	BOOL bRet = TRUE;
	WSADATA		wsaData;

	DBGOUTEX2("[INFO0001]InitConnection Function Starts.\n");

	//+++ �\�P�b�g���� +++
	udp_sd = INVALID_SOCKET;
	tcp_sd = INVALID_SOCKET;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		DBGOUTEX2("[EROR1002]WSAStartup failed.\n");		
		bRet = FALSE;
		goto EXIT;
	}

	////////////////////////////////////////////
	// 1. UDP�\�P�b�g�쐬&���� 
	////////////////////////////////////////////
	//+++ Create Socket(UDP) +++
	if ((udp_sd = ::socket(AF_INET6, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		DBGOUTEX2("[EROR1002] Socket(Udp) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}
	//+++ Binding(UDP) +++
	struct sockaddr_in6 udpaddr;
	struct in6_addr anyaddr = IN6ADDR_ANY_INIT;
	memset((char *)&udpaddr, 0, sizeof(udpaddr));
	udpaddr.sin6_family = AF_INET6;
	udpaddr.sin6_port=htons(UDP_PORT);
	udpaddr.sin6_flowinfo = 0;
	udpaddr.sin6_addr = anyaddr;
	if(::bind(udp_sd,(struct sockaddr *)&udpaddr, sizeof(udpaddr))==-1)
	{
		DBGOUTEX2("[EROR1002] Bind(udp) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}
	//+++ Sockopt(UDP): Buffer�ݒ� +++
	int	buf_size = MAX_SOCKBUF, buf_minsize = MAX_SOCKBUF / 2;
	if (::setsockopt(udp_sd, SOL_SOCKET, SO_SNDBUF, (char *)&buf_size, sizeof(int)) != 0
	&&	::setsockopt(udp_sd, SOL_SOCKET, SO_SNDBUF, (char *)&buf_minsize, sizeof(int)) != 0)
	{
		DBGOUTEX2("[EROR1002] setsockopt(sendbuf) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}
	buf_size = MAX_SOCKBUF, buf_minsize = MAX_SOCKBUF / 2;
	if (::setsockopt(udp_sd, SOL_SOCKET, SO_RCVBUF, (char *)&buf_size, sizeof(int)) != 0
	&&	::setsockopt(udp_sd, SOL_SOCKET, SO_RCVBUF, (char *)&buf_minsize, sizeof(int)) != 0)
	{
		DBGOUTEX2("[EROR1002] setsockopt(recvbuf) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

	//+++ Sockopt(UDP): Multicast Join +++
	struct ipv6_mreq maddr;
	struct sockaddr_in6 v6addr;
	memset(&v6addr, 0x00, sizeof(sockaddr_in6));
	int size = sizeof(struct sockaddr_in6);
	::WSAStringToAddress(L"ff02::c", AF_INET6, NULL, (LPSOCKADDR)&v6addr, &size);
	memcpy(&maddr.ipv6mr_multiaddr, &v6addr.sin6_addr, sizeof(struct in6_addr));
	maddr.ipv6mr_interface = 0;
	if ( ::setsockopt(udp_sd, IPPROTO_IPV6 , IPV6_JOIN_GROUP, (char *)&maddr, sizeof(struct ipv6_mreq)) != 0)
	{
		DBGOUTEX2("[EROR1002] setsockopt(multicast) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}
	//+++ Sockopt(UDP): Multicast LOOPBACK +++
	u_int on = 0;/* 0 = disable, 1 = enable; default = 1 */
	if ( ::setsockopt(udp_sd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *)&on, sizeof(on)) < 0)
	{
		DBGOUTEX2("[EROR1002] setsockopt(multicast loopback) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}


	////////////////////////////////////////////
	// 2. TCP�\�P�b�g�쐬&���� 
	////////////////////////////////////////////
	//+++ Create Socket(TCP) +++
	if ((tcp_sd = ::socket(AF_INET6, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		DBGOUTEX2("[EROR1002] Socket(tcp) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}
	
	//+++ Sockopt(TCP) +++
	BOOL flg = TRUE;
	if (setsockopt(tcp_sd, SOL_SOCKET, SO_REUSEADDR, (char *)&flg, sizeof(flg)) != 0)
	{
		DBGOUTEX2("[EROR1002] setsockopt(tcp-SO_REUSEADDR) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

	//+++ Binding(TCP) +++
	struct sockaddr_in6 tcpaddr;
	memset((char *)&tcpaddr, 0, sizeof(tcpaddr));
	tcpaddr.sin6_family = AF_INET6;
	tcpaddr.sin6_port=htons(TCP_PORT);
	tcpaddr.sin6_flowinfo = 0;
	tcpaddr.sin6_addr = anyaddr;
	if(::bind(tcp_sd,(struct sockaddr *)&tcpaddr, sizeof(tcpaddr))==-1)
	{
		DBGOUTEX2("[EROR1002] bind(tcp) failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

	//+++ Listen(TCP) +++
	if (::listen(tcp_sd, 5) != 0)
	{
		DBGOUTEX2("[EROR1002] listen failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

EXIT:
	if(!bRet)
	{
		//�G���[���͍쐬�ς�socket���N���[�Y
		if(udp_sd != INVALID_SOCKET)
		{
			::closesocket(udp_sd);
		}
		if(tcp_sd != INVALID_SOCKET)
		{
			::closesocket(tcp_sd);
		}
	}

	DBGOUTEX2("[INFO0002]InitConnection Function Ends.\n");

	return	bRet;
}






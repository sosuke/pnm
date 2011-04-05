/************************************************************/
/* [Object受信まわり]                                       */
/*   + accept_func                                          */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

//-- 外部参照
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
	struct sockaddr_in6  from;         //OBJECT取得要求元
	int len = 0;                       //sockaddr_in6構造体サイズ
	char recv[MAX_RECVBUF_SIZE];       //受信データ	
	char chDis = '\0';                 //OBJECTパケット識別子
	BOOL IsRequestAllObjects = FALSE;  //全オブジェクト取得要求パケットか否か
	GUID reqObjectId;                  //取得要求オブジェクトのGUID
	char* ptr = NULL;                  //送信パケット用メモリポインタ
	char* tmpPtr = NULL;               //送信パケット用メモリポインタ（作業用）
	DWORD dwCurrentMallocSize = 0;     //送信パケットサイズ
	fd_set org_accept_fd, accept_fd;   //SELECT用ディスクリプタ
	struct timeval waitval;	           //タイムアウト用
	SOCKET remoteSocket;               //接続ソケット(TCP)
	int nRet = 0;                      //戻り値（作業用）
	std::list<Object>::iterator it;    //iterator

	//-- 初回時初期化
	remoteSocket = INVALID_SOCKET;
	FD_ZERO(&org_accept_fd);
	FD_SET(tcp_sd, &org_accept_fd);
	::ZeroMemory(&waitval, sizeof(waitval));
	waitval.tv_sec  = SELECT_WAIT_TIME_SEC;
	waitval.tv_usec = SELECT_WAIT_TIME_USEC;

	while(1)
	{
		//-- スレッド終了要求がある場合はループを抜ける
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

		//-- SignInしてない場合は何もせず次ループへ
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

		//-- ループ毎に初期化 
		dwCurrentMallocSize = 0;
		::ZeroMemory(recv, sizeof(recv));
		::ZeroMemory(&reqObjectId, sizeof(reqObjectId));
		::memcpy(&accept_fd, &org_accept_fd, sizeof(org_accept_fd));

		//-- connectionキューチェック（SELECT）
		nRet = ::select(FD_SETSIZE, &accept_fd, NULL, NULL, &waitval);
		if(nRet == 0)
		{
			//タイムアウトの場合は次ループへ
			goto ROOP_EXIT;
		}

		//-- TCPコネクト接続（ACCEPT）
		len = sizeof(from);
		remoteSocket = ::accept(tcp_sd, (struct sockaddr *)&from ,&len);
		if(remoteSocket == INVALID_SOCKET)
		{
			//acceptエラー時は次ループへ
			goto ROOP_EXIT;
		}

		//-- RCPソケット受信（recv） --
		if( (len = ::recv(remoteSocket, (char*)recv, sizeof(recv), 0)) < sizeof(char))
		{
			//recvエラー時はソケットをクローズした後
			goto ROOP_EXIT;
		}
		//recv[len/sizeof(WCHAR)]=L'\0'; //この処理は初期化を完全にやることで不要とする
		
		//-- OBJECTパケット種別の判別
		::memcpy(&chDis, recv, sizeof(chDis));
		if(chDis == 'A') 
		{
			//'A'ならばOBJECT_IDの指定あり
			IsRequestAllObjects = FALSE;
		}
		else if(chDis == 'B') 
		{
			//'B'ならばOBJECT_IDの指定なし、全Objectの要求
			IsRequestAllObjects = TRUE;
		}
		else
		{
			//識別子が'A' or 'B'でなければエラーとして次ループへ
			goto ROOP_EXIT;
		}
		if(!IsRequestAllObjects)
		{
			::memcpy(&reqObjectId, recv+sizeof(char), sizeof(GUID));
		}
		
		//-- 送信用パケットデータを生成
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
				//指定されたIDのオブジェクトデータを返信する（ここの考え方が実は違うのかも。。。）
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
				//保持している全オブジェクトデータを返信する
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

		//-- オブジェクトデータ返信
		if(::send(remoteSocket, ptr, dwCurrentMallocSize,0) == -1)
		{
			//エラーの場合でもそのまま続行（以降実処理がないため）
		}		

ROOP_EXIT:
		//-- 後処理
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
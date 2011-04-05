/************************************************************/
/* [p2phostwm基本ヘッダ]                                    */
/************************************************************/

//-- DebugOutputStringマクロ
#ifdef _DEBUG
#define DBGFILE "debug.txt"
#endif
#ifdef DBGFILE
#define DBGFILENAME DBGFILE
inline void debug(char* format, ...)
{
	va_list va;
	FILE* fp = fopen(DBGFILENAME, "a");
	if(fp)
	{
		va_start(va, format);
		vfprintf(fp, format, va);
		va_end(va);
		fclose(fp);
	}
}
#define DBGOUT ::debug
#define DBGOUTEX ::debug("%s (%d):", __FILE__, __LINE__), ::debug
#define THREADID__ GetCurrentThreadId()
#define DBGOUTEX2 ::debug("%s (%d): %x# ",__FILE__,__LINE__,THREADID__),::debug
#else
inline void debug(char * format,...){}
#define DBGOUT              1 ? (void)0 : ::debug
#define DBGOUTEX DBGOUT
#define DBGOUTEX2 DBGOUT
#endif

//-- GET_?_LPARAMマクロ
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam) ((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam) ((int)(short)HIWORD(lParam))
#endif

//+++ XMLデータ +++
#define PROBE_MSGSTR_HEAD L"<?xml version=\"1.0\" encoding=\"utf-8\" ?><Envelope><Body><Probe><Types>a4c1fbe4-6d30-46c9-8bba-b8663d615706</Types><Version>1.0.0.0</Version>"
#define PROBE_MSGSTR_TRAIL  L"</Probe></Body></Envelope>"
#define HELLO_MSGSTR_HEAD L"<?xml version=\"1.0\" encoding=\"utf-8\" ?><Envelope><Body><Hello><Types>a4c1fbe4-6d30-46c9-8bba-b8663d615706</Types><Version>1.0.0.0</Version>"
#define HELLO_MSGSTR_TRAIL L"</Hello></Body></Envelope>"
#define PROBEMATCH_MSGSTR_HEAD L"<?xml version=\"1.0\" encoding=\"utf-8\" ?><Envelope><Body><ProbeMatch><Types>a4c1fbe4-6d30-46c9-8bba-b8663d615706</Types><Version>1.0.0.0</Version>"
#define PROBEMATCH_MSGSTR_TRAIL L"</ProbeMatch></Body></Envelope>"
#define BYE_MSGSTR_HEAD L"<?xml version=\"1.0\" encoding=\"utf-8\" ?><Envelope><Body><Bye><Types>a4c1fbe4-6d30-46c9-8bba-b8663d615706</Types><Version>1.0.0.0</Version>"
#define BYE_MSGSTR_TRAIL L"</Bye></Body></Envelope>"

//+++ 各種パラメータ +++
#define MAX_SOCKBUF		             65536
#define UDP_PORT                     10000
#define TCP_PORT                     10001
#define FRAME_TIME                   10000    //[msec]
#define MAX_OBJECT_STORE_NUM         5
#define MAX_REFRESH_TTL              10
#define CHECK_FRAME_CYCLE            3
#define MAX_LOADSTRING               100
#define NEXT_RECV_WAIT               1000     //[msec]
#define NEXT_SEND_WAIT               1000     //[msec]
#define NEXT_ACCEPT_WAIT             1000     //[msec]
#define MAX_USERNAME_SIZE            63
#define GUID_STR_SIZE                36    
#define DEVICEID_ELEMENT_SIZE        21
#define USERNAME_ELEMENT_SIZE        21
#define OBJECT_ELEMENT_SIZE          22
#define MAX_OBJECTSEQ_DIGIT          8        //Object Sequence Noの最大桁数
#define MAX_RECVBUF_SIZE             1024     //メッセージ受信用バッファサイズ（送信用バッファ中最大サイズ）[WCHAR]
#define MAX_HELLO_SENDBUF_SIZE       1024     //HELLOメッセージ送信用バッファサイズ
#define MAX_BYE_SENDBUF_SIZE         512      //BYEメッセージ送信用バッファサイズ
#define MAX_PROBE_SENDBUF_SIZE       512      //PROBEメッセージ送信用バッファサイズ
#define MAX_PROBEMATCH_SENDBUF_SIZE  1024     //BYEメッセージ送信用バッファサイズ
#define DEFAULT_COMPUTER_NAME        "anonymous"
#define REGKEY_PROFILE               L"Software\\p2phostwm\\Profile"
#define REGKEY_PROFILE_USER          L"UserName"
#define REGKEY_PROFILE_GUID          L"GUID"
#define DEVICE_TYPE_NONE             0
#define DEVICE_TYPE_POCKETPC         1
#define DEVICE_TYPE_SMARTPHONE       2
#define PLATFORM_TYPE_NONE           0
#define PLATFORM_TYPE_WM2003         1
#define PLATFORM_TYPE_WM2003SE       2
#define PLATFORM_TYPE_WM50           3
#define PLATFORM_TYPE_WM60           4
#define IPV6_MULTICAST_ADDRESS       L"ff02::c"
#define SIZEOF_GUID                  16
#define SIZEOF_WCHAR                 2
#define SIZEOF_DWORD                 4
#define MAX_OBJECT_SIZE              2048     //１オブジェクトデータの最大サイズ
#define MAX_STORE_OBJECT_NUM         20       //１ピアが最大保持できるオブジェクト数
#define MAX_RECV_OBJECTS             41360    //オブジェクトデータ受信最大バイト数
                                              //(SIZEOF_GUID + SIZEOF_DWORD+MAX_OBJECT_SIZE)*MAX_STORE_OBJECT_NUM
#define MAX_TCP_CONNECT_NUM          10       //
#define SELECT_WAIT_TIME_SEC         2        //SELECTタイムアウト値[sec]
#define SELECT_WAIT_TIME_USEC        500      //SELECTタイムアウト値[usec]


//+++ イベントID　+++
#define WM_TRAY_NOTIFY		(WM_USER + 100)

//+++ 構造体定義　+++
typedef struct _WindowData
{
	HINSTANCE		hI;
	HWND			hMainWnd;
	HWND			hAboutDlg;
	HWND			hSuspendDlg;

	const WCHAR		*suspendStr;
	int				suspendCnt;
	int				timerMin;
	UINT			timerID;
	HANDLE			hNotify;
	NOTIFYICONDATA	nid;

	UINT			options;
	UINT			status;
} WindowData;

typedef struct _Object
{
	GUID  objectId;
	DWORD objectSeq;
	DWORD cbData;
	PVOID pbData;
} Object;

typedef struct _DeviceProfile
{
	GUID deviceId;
	WCHAR userName[MAX_USERNAME_SIZE+1];
} DeviceProfile;

typedef struct _PeerMap
{
	GUID deviceId; //[*key]
	WCHAR userName[MAX_USERNAME_SIZE+1];
	DWORD ttl;    
	struct sockaddr_in6 address;
} PeerMap;

typedef struct _ObjectMap
{
	GUID objectId;      //ObjectID [*key]
	GUID deviceId;      //DeviceID
	DWORD objectSeq;    //Object Sequence No
	BOOL flgDel;        //Deleteチェック用
} ObjectMap;


//+++ プロトタイプ宣言 +++
//-- p2phostwm.cpp
BOOL Initialize(HINSTANCE hI, LPTSTR cmdLine);
BOOL InitMainWindow(HINSTANCE hI);
BOOL InitDeviceProfile(void);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT WmSignInProc(void);
LRESULT WmSignOutProc(void);
void Terminate(void);
//-- socket.cpp
BOOL InitConnection(void);
//-- utility.cpp
void GetPeerAddress(GUID deviceId, struct sockaddr_in6* addr);
BOOL IsEmptyGuid(GUID* guid);
BOOL GuidToString(GUID guid, WCHAR* szString);
BOOL GuidFromString(GUID* guid, WCHAR* szString);
//-- frame.cpp
void RefreshPeerMap(void);
DWORD WINAPI frame_func(LPVOID pParam);
//-- send.cpp
HRESULT HelloSend(void);
HRESULT ProbeSend(void);
HRESULT ByeSend(void);
HRESULT ProbeMatchSend(struct sockaddr_in6 from);
HRESULT MakeHelloMsg(WCHAR* buf, DWORD dwBufSize);
HRESULT MakeProbeMsg(WCHAR* buf, DWORD dwBufSize);
HRESULT MakeByeMsg(WCHAR* buf, DWORD dwBufSize);
HRESULT MakeProbeMatchMsg(WCHAR* buf, DWORD dwBufSize);
//-- notify.cpp
HRESULT SetEventNotify(DWORD dwEventType, DWORD dwEventAction, PVOID lpEventData, DWORD dwDataSize);
HRESULT PnmEventNotify(PEER_CHANGE_TYPE actionType, GUID* deviceId, WCHAR* userId);
HRESULT ObjectEventNotify(PEER_CHANGE_TYPE actionType, GUID* objectId, GUID* deviceId);
LRESULT RegisterEvent(PCOPYDATASTRUCT pData);
//-- recv.cpp
DWORD WINAPI recv_func(LPVOID pParam);
BOOL RenewPeerObjectMap(GUID objectID, GUID deviceID, DWORD dwSeqNo);
void InitializeDeleteFlagOnObjectMap(GUID deviceID);
void CheckDeleteFlagOnObjectMap(GUID deviceID);
void CheckDeleteOnObjectMapWhenEmpty(GUID deviceID);
BOOL HelloProbeMatchRecv(GUID deviceID, WCHAR *userName, struct sockaddr_in6 from);
BOOL ByeRecv(GUID deviceId);
BOOL ProbeRecv(struct sockaddr_in6 from);
//-- accept.cpp
DWORD WINAPI accept_func(LPVOID pParam);
//-- object.cpp
LRESULT SetObject(PCOPYDATASTRUCT pData);
LRESULT DeleteObject(PCOPYDATASTRUCT pData);
LRESULT EnumPeopleNearMe(void);
LRESULT EnumObjects(PCOPYDATASTRUCT pData);
LRESULT EnumMyObjects(GUID* guidObject);
LRESULT EnumPnmObjects(GUID* guidDevice, GUID* guidObject);
#ifdef P2PHOSTWM
//-- version.cpp
DWORD GetDeviceType(void);
DWORD GetPlatformInfo(void);
#endif


//+++ メッセージID +++
#define CM_STARTUP           WM_USER+21
#define CM_SHUTDOWN          WM_USER+22
#define CM_SIGNIN            WM_USER+23
#define CM_SIGNOUT           WM_USER+24
#define CM_ENUM_PEOPLENEARME WM_USER+25
#define CM_ENUM_MYOBJECTS    WM_USER+26
#define CM_ENUM_OBJECTS      WM_USER+27
#define CM_UNREGISTER_EVENT  WM_USER+28

//+++ 戻り値 +++
#define PEER_E_UNSUPPORTED_VERSION  101
#define PEER_E_INITIALIZE_EXECUTE   102
#define PEER_E_INITIALIZE_REGQUERY  103
#define PEER_E_NOT_INITIALIZED      104
#define PEER_SHUTDOWN_FAILED        105
#define PEER_SIGNIN_FAILED          106
#define PEER_SIGNOUT_FAILED         107
#define PEER_OBJECTSEND_FAILED      108
#define PEER_ENUMPNM_FAILED         109
#define PEER_ENUMOBJ_FAILED         110

#define PEER_S_NO_CONNECTIVITY      201
#define PEER_S_OBJECT_UPDATED       202
#define PEER_S_OBJECT_ADDED         203
#define PEER_S_OBJECT_DELETED       204
#define PEER_S_OBJECT_NOTEXIST      205

//+++ COPYDATASTRUCT機能コード +++
#define FLG_OBJECT_SET              0
#define FLG_OBJECT_DEL              1
#define FLG_OBJECT_ENUM1            2 // [Device-Object] = [NULL, NULL]
#define FLG_OBJECT_ENUM2            3 // [Device-Object] = [NULL, GUID]
#define FLG_OBJECT_ENUM3            4 // [Device-Object] = [GUID, NULL]
#define FLG_OBJECT_ENUM4            5 // [Device-Object] = [GUID, GUID]
#define FLG_EVENT_REG               6

//+++ レジストリキー
#define REGKEY_P2PSERVICE           L"Software\\p2phostwm"
#define REGKEY_P2PSERVICE_PATH      L"Path"

//+++ ファイルマッピング名 +++
#define FM_ENUMPEOPLENEARME L"ENUMPEOPLENEARME"
#define FM_ENUMOBJECTS      L"ENUMOBJECTS"
#define FM_EVENTDATA        L"EVENTDATA"

//+++ データ構造定義 +++
typedef enum 
{
  PEER_SIGNIN_NONE,
  PEER_SIGNIN_NEAR_ME,
  PEER_SIGNIN_INTERNET,
  PEER_SIGNIN_ALL
} WMPEER_SIGNIN_FLAGS;

typedef struct peer_data_tag {
  ULONG cbData;
  PBYTE pbData;
} PEER_DATA, *PPEER_DATA;

typedef struct {
  GUID id;
  PEER_DATA data;
  DWORD dwPublicationScope;
} PEER_OBJECT, *PPEER_OBJECT;
typedef const PEER_OBJECT *PCPEER_OBJECT;

typedef struct _DataUnit
{
	GUID  dataId;
	DWORD cbData;
	PVOID pbData;
} DATA_UNIT, *PDATA_UNIT; 

typedef enum 
{
  PEER_EVENT_ENDPOINT_OBJECT_CHANGED,
  PEER_EVENT_PEOPLE_NEAR_ME_CHANGED
} PEER_COLLAB_EVENT_TYPE, *PPEER_COLLAB_EVENT_TYPE;

typedef enum 
{
  PEER_CHANGE_ADDED,
  PEER_CHANGE_DELETED,
  PEER_CHANGE_UPDATED
}PEER_CHANGE_TYPE;

typedef struct {
  PEER_COLLAB_EVENT_TYPE eventType;
  PEER_CHANGE_TYPE actionType;
  GUID peerId;
  DWORD dwItemCount;
  PVOID* ppItems;
} PEER_COLLAB_EVENT_DATA, *PPEER_COLLAB_EVENT_DATA;

typedef struct {
  PEER_COLLAB_EVENT_TYPE eventType;
  GUID* pInstance;
} PEER_COLLAB_EVENT_REGISTRATION, *PPEER_COLLAB_EVENT_REGISTRATION;
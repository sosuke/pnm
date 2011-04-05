/************************************************************/
/* [p2phostwm.exeメイン制御]                                */
/*   + Initialize                                           */
/*   + InitMainWindow                                       */
/*   + InitDeviceProfile                                    */
/*   + WndProc                                              */
/*   + WmSignInProc                                         */
/*   + WmSignOutProc                                        */
/*   + Terminate                                            */
/************************************************************/
#include "stdafx.h"
#include "p2phostwm.h"

//-- グローバル変数
WindowData           g_WndData;
DeviceProfile        g_MyDeviceProfile;
std::list<Object>    MyObjectList;
std::list<PeerMap>   MasterPeerMap;
std::list<ObjectMap> MasterPeerObjects;
HANDLE hThread[3];
DWORD dwThreadId[3];
BOOL g_bIsSignIn;
BOOL g_bIsEnd_frame;
BOOL g_bIsEnd_recv;
BOOL g_bIsEnd_accept;

//-- クリティカルセクション（粒度は細かく）
CRITICAL_SECTION cs_g_bIsSignIn;  
CRITICAL_SECTION cs_g_MyDeviceProfile;
CRITICAL_SECTION cs_MasterPeerMap;
CRITICAL_SECTION cs_MasterPeerObjects;
CRITICAL_SECTION cs_MyObjectList;
CRITICAL_SECTION cs_g_bIsEnd_frame;
CRITICAL_SECTION cs_g_bIsEnd_recv;
CRITICAL_SECTION cs_g_bIsEnd_accept;


//-- 外部参照
extern SOCKET		udp_sd; 
extern SOCKET		tcp_sd; 

////////////////////////////////////////////
//
// InitMainWindow
//
////////////////////////////////////////////
BOOL InitMainWindow(HINSTANCE hI)
{    
	BOOL bRet = TRUE;
	HWND hWnd = NULL;
	WCHAR szTitle[MAX_LOADSTRING];		    // タイトル バー テキスト
	WCHAR szWindowClass[MAX_LOADSTRING];	// メイン ウィンドウ クラス名
	
	DBGOUTEX2("[INFO0001]InitMainWindow Function Starts.\n");

	//+++初期化 +++
	::ZeroMemory(szTitle, sizeof(szTitle));
	::ZeroMemory(szWindowClass, sizeof(szWindowClass));

	LoadString(hI, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hI, IDC_P2PHOSTWM, szWindowClass, MAX_LOADSTRING);

	//+++ 複数起動防止 +++
    hWnd = FindWindow(szWindowClass, szTitle);	
    if (hWnd) 
    {
		DBGOUTEX2("[INFO0004] Already launched.\n");
        SetForegroundWindow((HWND)((ULONG) hWnd | 0x00000001));
        goto EXIT;
    }

    //+++ ウィンドウクラス登録 +++
	WNDCLASS	wc;
	::ZeroMemory(&wc, sizeof(wc));
    wc.style			= 0;
    wc.lpfnWndProc		= (WNDPROC) WndProc;
    wc.hInstance		= g_WndData.hI;
    wc.hIcon			= ::LoadIcon(g_WndData.hI, MAKEINTRESOURCE(IDI_MAINICON));
    wc.lpszClassName	= szWindowClass;
	if (!::RegisterClass(&wc))
	{
		DBGOUTEX2("[EROR1002]RegisterClass failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

    //++ メインウィンドウ作成 +++
	if ((g_WndData.hMainWnd = ::CreateWindow(szWindowClass, szTitle, 0, 0, 0, 0, 0, 0, NULL, g_WndData.hI, 0)) == NULL)
	{
		DBGOUTEX2("[EROR1003]CreateWindow failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

EXIT:
	DBGOUTEX2("[INFO0002]InitMainWindow Function Ends.\n");
	return	bRet;
}


////////////////////////////////////////////
//
// InitDeviceProfile
//
////////////////////////////////////////////
BOOL InitDeviceProfile(void)
{ 
    BOOL bRet = TRUE;                            // 戻り値
	GUID gid;                                    // デバイス識別子
	HKEY hkResult = NULL;	                   	 // キーのハンドル
	DWORD dwDisposition = 0;	                 // 処理結果を受け取る
	LONG lResult = 0;	    	                 // 関数の戻り値を格納する
	DWORD dwType = 0;	    	                 // 値の種類を受け取る
	DWORD dwSize = 0;		                     // データのサイズを受け取る
	DWORD dwComputerNameSize = 0;                // コンピュータ名文字列長
	DWORD nRet = 0;                              // 作業用関数戻り値
	char szComputerName[MAX_USERNAME_SIZE+1];    // コンピュータ名
	WCHAR wszComputerName[MAX_USERNAME_SIZE+1];  // コンピュータ名(W)
	WCHAR wszDeviceGuid[GUID_STR_SIZE+1];        // デバイスGUID(W)

	DBGOUTEX2("[INFO0001]InitDeviceProfile Function Starts.\n");

	//+++ 初期化 +++
	::ZeroMemory(&gid, sizeof(gid));

	//+++ プロファイル格納用レジストリキーを取得 +++
	lResult = ::RegCreateKeyExW(
		HKEY_CURRENT_USER, 
		REGKEY_PROFILE, 
		0, 
		L"", 
		REG_OPTION_NON_VOLATILE, 
		KEY_ALL_ACCESS, 
		NULL, 
		&hkResult, 
		&dwDisposition);
	if( lResult != ERROR_SUCCESS) 
	{
		bRet = FALSE;
		goto EXIT;
	}

	//+++ デバイスIDの設定 +++
	//レジストリ格納済みのデバイスIDを取得
	::ZeroMemory(wszDeviceGuid, sizeof(wszDeviceGuid));
	::RegQueryValueExW(hkResult, REGKEY_PROFILE_GUID, NULL, &dwType, NULL, &dwSize);
	if(dwSize == 0)
	{
		//新規起動時はGUIDを生成する
		::CoCreateGuid(&gid);
		GuidToString(gid, wszDeviceGuid);
		::RegSetValueExW(hkResult, REGKEY_PROFILE_GUID, 0, REG_SZ, (CONST BYTE*)wszDeviceGuid, sizeof(wszDeviceGuid));

		DBGOUTEX2("[INFO0006] Profile set to DeviceID.\n");
	}
	else
	{
		::RegQueryValueExW(hkResult, REGKEY_PROFILE_GUID, NULL, &dwType, (LPBYTE)wszDeviceGuid, &dwSize);
		GuidFromString(&gid, wszDeviceGuid);
		DBGOUTEX2("[INFO0005] Profile set from Registry.\n");
	}
	//::CoCreateGuid(&gid);
	memcpy(&g_MyDeviceProfile.deviceId, &gid, sizeof(GUID));

	//+++ ユーザ名の設定 +++
	//レジストリから設定済みのユーザ名を取得
	dwSize = dwType = 0;
	::ZeroMemory(wszComputerName, sizeof(wszComputerName));
	//2008/2/18 不具合&エラーロジックとなるため削除
	//::RegQueryValueExW(hkResult, REGKEY_PROFILE_USER, NULL, &dwType, NULL, &dwSize);
	if(dwSize == 0)
	{
		::ZeroMemory(szComputerName, sizeof(szComputerName));
#ifdef P2PHOSTWM
		nRet = gethostname(szComputerName, sizeof(szComputerName));
		if(nRet == -1)
#else		
		dwComputerNameSize = sizeof(szComputerName);
		nRet = ::GetComputerNameA(szComputerName, &dwComputerNameSize);
		if(nRet == 0)
#endif
		{
			::strcpy(szComputerName, DEFAULT_COMPUTER_NAME);			
		}
		::MultiByteToWideChar(CP_ACP, 0,szComputerName, strlen(szComputerName), wszComputerName, sizeof(wszComputerName));
		::RegSetValueExW(hkResult, REGKEY_PROFILE_USER, 0, REG_SZ, (CONST BYTE*)wszComputerName, sizeof(wszComputerName));

		DBGOUTEX2("[INFO0006] Profile set to DeviceName.\n");
	} 
	else 
	{
		::RegQueryValueExW(hkResult, REGKEY_PROFILE_USER, NULL, &dwType, (LPBYTE)wszComputerName, &dwSize);
		DBGOUTEX2("[INFO0005] Profile set from Registry.\n");
	}
	memcpy(g_MyDeviceProfile.userName, wszComputerName, sizeof(g_MyDeviceProfile.userName));

EXIT:
	if(hkResult != NULL)
	{
		::RegCloseKey(hkResult);
	}
	DBGOUTEX2("[INFO0002]InitDeviceProfile Function Ends.\n");
	return TRUE;
}

////////////////////////////////////////////
//
// Initialize
//
////////////////////////////////////////////
BOOL Initialize(HINSTANCE hI, LPTSTR cmdLine)
{
	BOOL bRet = TRUE;

	DBGOUTEX2("[INFO0001]Initialize Function Starts.\n");

	//+++ バージョンチェック +++
#ifdef P2PHOSTWM
	if(GetPlatformInfo() != PLATFORM_TYPE_WM50 || GetDeviceType() != DEVICE_TYPE_POCKETPC)
	{
		DBGOUTEX2("[INFO0007] This platform is not support.\n");
		bRet = FALSE;
		goto EXIT;
	}
	else 
	{
		::SHInitExtraControls();
	}
#endif

	//+++ アプリケーションハンドルの退避 +++
	g_WndData.hI = hI;

	//+++ メインウィンドウ生成 +++
	if(!InitMainWindow(hI))
	{
		DBGOUTEX2("[EROR1004]Initialize Window failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

    //+++ タスクトレイ登録 +++
	::ZeroMemory(&g_WndData.nid, sizeof(NOTIFYICONDATA));
	g_WndData.nid.cbSize = sizeof(NOTIFYICONDATA);
	g_WndData.nid.hWnd = g_WndData.hMainWnd;
	g_WndData.nid.uID = WM_TRAY_NOTIFY;
	g_WndData.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	g_WndData.nid.uCallbackMessage = WM_TRAY_NOTIFY;
	g_WndData.nid.hIcon = (HICON)::LoadImage(g_WndData.hI, MAKEINTRESOURCE(IDI_MAINICON), IMAGE_ICON, 16, 16, 0);
	if (!::Shell_NotifyIcon(NIM_ADD, &g_WndData.nid))
	{
		DBGOUTEX2("[EROR1005]Registry tasktray icon failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

	//+++ デバイスプロファイルの設定 +++
	if(!InitDeviceProfile())
	{
		DBGOUTEX2("[EROR1006]DeviceProfile Initialize failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

	//+++ 通信初期化 +++
	if(!InitConnection())
	{
		DBGOUTEX2("[EROR1007]Connection Initialize failed.\n");
		bRet = FALSE;
		goto EXIT;
	}

	//+++ サインイン状態の復帰 +++
	g_bIsSignIn = FALSE;

	//++ COM初期化 +++
	::CoInitializeEx(NULL, COINIT_MULTITHREADED);

	//+++ スレッド開始 +++
	g_bIsEnd_frame  = FALSE;
	g_bIsEnd_recv   = FALSE;
	g_bIsEnd_accept = FALSE;
	::InitializeCriticalSection(&cs_g_bIsSignIn);
	::InitializeCriticalSection(&cs_g_bIsEnd_frame);
	::InitializeCriticalSection(&cs_g_bIsEnd_recv);
	::InitializeCriticalSection(&cs_g_bIsEnd_accept);
	::InitializeCriticalSection(&cs_MasterPeerMap);
	::InitializeCriticalSection(&cs_MasterPeerObjects);
	::InitializeCriticalSection(&cs_MyObjectList);
	::InitializeCriticalSection(&cs_g_MyDeviceProfile);
	
	hThread[0]=(HANDLE)CreateThread(NULL, 0, frame_func, NULL, 0, &dwThreadId[0]);
	hThread[1]=(HANDLE)CreateThread(NULL, 0, recv_func,  NULL, 0, &dwThreadId[1]);
	hThread[2]=(HANDLE)CreateThread(NULL, 0, accept_func,NULL, 0, &dwThreadId[2]);

EXIT:
	DBGOUTEX2("[INFO0002]Initialize Function Ends.\n");
	return bRet;
}

////////////////////////////////////////////
//
// Terminate
//
////////////////////////////////////////////
void Terminate()
{
	DBGOUTEX2("[INFO0001]Terminate Function Starts.\n");

	//+++ スレッドの終了処理 +++
	::EnterCriticalSection(&cs_g_bIsEnd_frame);
	g_bIsEnd_frame  = TRUE;
	::LeaveCriticalSection(&cs_g_bIsEnd_frame);

	::EnterCriticalSection(&cs_g_bIsEnd_recv);
	g_bIsEnd_recv   = TRUE;
	::LeaveCriticalSection(&cs_g_bIsEnd_recv);

	::EnterCriticalSection(&cs_g_bIsEnd_accept);
	g_bIsEnd_accept = TRUE;
	::LeaveCriticalSection(&cs_g_bIsEnd_accept);

	::WaitForMultipleObjects(3, hThread, TRUE, INFINITE);

	//+++ 各種マップ情報のリリース +++
	std::list<Object>::iterator it = MyObjectList.begin();
	while( it != MyObjectList.end() )
	{
		if(it->pbData != NULL)
		{
			::free(it->pbData);
		}
		it++;
	}	
	MyObjectList.clear();
	MasterPeerMap.clear();
	MasterPeerObjects.clear();

	//+++ クリティカルセクションオブジェクトの開放 +++
	::DeleteCriticalSection(&cs_g_bIsSignIn);
	::DeleteCriticalSection(&cs_g_bIsEnd_frame);
	::DeleteCriticalSection(&cs_g_bIsEnd_recv);
	::DeleteCriticalSection(&cs_g_bIsEnd_accept);
	::DeleteCriticalSection(&cs_MasterPeerMap);
	::DeleteCriticalSection(&cs_MasterPeerObjects);
	::DeleteCriticalSection(&cs_MyObjectList);
	::DeleteCriticalSection(&cs_g_MyDeviceProfile);

	//+++ ソケットのクローズ +++
	if(udp_sd != INVALID_SOCKET)
	{
		::closesocket(udp_sd);
	}
	if(tcp_sd != INVALID_SOCKET)
	{
		::closesocket(udp_sd);
	}

	//+++ タスクトレイウィンドウ破棄 +++
	::Shell_NotifyIcon(NIM_DELETE, &g_WndData.nid);

	//+++ COM終了処理 +++
	::CoUninitialize();	

	DBGOUTEX2("[INFO000]Terminate Function Ends.\n");
}

////////////////////////////////////////////
//
// WmTrayNotifyProc
//  
////////////////////////////////////////////
LRESULT WmTrayNotifyProc(HWND hwnd,DWORD fwKeys,WORD xPos,WORD yPos)
{
    HMENU   hMenu;
    HMENU   hSubMenu;
    POINT   p;

	DBGOUTEX2("[INFO0001]WmTrayNotifyProc Function Starts.\n");

	//+++ ポップアップ用メニューをロード +++
	hMenu= ::LoadMenu(g_WndData.hI, MAKEINTRESOURCE(IDR_MAINMENU));
	hSubMenu= ::GetSubMenu(hMenu,0);

	//+++ メニュー項目のアクティブ／非アクティブ設定 +++
	::EnterCriticalSection(&cs_g_bIsSignIn);  //ここでブロックしとるぞ！！
	if(g_bIsSignIn)
	{
		::EnableMenuItem(hMenu, ID_MENU_SIGNIN,  MF_GRAYED);
		::EnableMenuItem(hMenu, ID_MENU_SIGNOUT, MF_ENABLED);
	}
	else
	{
		::EnableMenuItem(hMenu, ID_MENU_SIGNIN,  MF_ENABLED);
		::EnableMenuItem(hMenu, ID_MENU_SIGNOUT, MF_GRAYED);
	}
	::LeaveCriticalSection(&cs_g_bIsSignIn);
	::DrawMenuBar(hwnd);	

	//+++ ポップアップメニュー表示座標の設定 +++
	//+++ (※スクリーン座標の右下とする)     +++
#ifdef P2PHOSTWM
	p.x = ::GetSystemMetrics(SM_CXSCREEN); 
	p.y = ::GetSystemMetrics(SM_CYSCREEN);
#else
	::GetCursorPos(&p);	
#endif

	::SetForegroundWindow(hwnd);
	::SetFocus(hwnd);

	TrackPopupMenu(hSubMenu,TPM_RIGHTALIGN|TPM_BOTTOMALIGN, p.x, p.y, 0, hwnd, NULL);
	::DestroyMenu(hMenu);

	DBGOUTEX2("[INFO0002]WmTrayNotifyProc Function Ends.\n");

    return  S_OK;
}


////////////////////////////////////////////
//
// AboutProc
//
////////////////////////////////////////////
LRESULT CALLBACK AboutProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			::EndDialog(hDlg, LOWORD(wParam)); 
			break;
		}
		break;
	}
    return S_OK;
}

////////////////////////////////////////////
//
// PropertyProc
//
////////////////////////////////////////////
LRESULT CALLBACK PropertyProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			::EndDialog(hDlg, LOWORD(wParam)); 
			break;
		}
		break;
	}
    return S_OK;
}



////////////////////////////////////////////
//
// WmCommandProc
//
////////////////////////////////////////////
LRESULT WmCommandProc(HWND hwnd,WORD wNotifyCode,WORD wID,HWND hwndCtl)
{
    switch(wID)
    {
        case ID_MENU_SIGNIN:
            return WmSignInProc();
        case ID_MENU_SIGNOUT:
            return WmSignOutProc();
		case ID_MENU_PROPERTY:
			::DialogBox(g_WndData.hI, (LPCTSTR)IDD_PROPERTY_DIALOG, NULL, (DLGPROC)PropertyProc);
			return S_OK;
        case ID_MENU_ABOUT:
			::DialogBox(g_WndData.hI, (LPCTSTR)IDD_ABOT_DIALOG, NULL, (DLGPROC)AboutProc);
            return S_OK;
        case ID_MENU_EXIT:
			WmSignOutProc();
			::PostQuitMessage(0);
			return S_OK;
		default:
			return S_OK;
    }
}



////////////////////////////////////////////
//
// WmSignInProc
//
////////////////////////////////////////////
LRESULT WmSignInProc(void)
{
	LRESULT nRetValue = S_OK;

	DBGOUTEX2("[INFO0001]WmSignInProc Function Starts.\n");

	::EnterCriticalSection(&cs_g_bIsSignIn);
	if(!g_bIsSignIn)
	{
		::LeaveCriticalSection(&cs_g_bIsSignIn);
		nRetValue = HelloSend();
		if(nRetValue != S_OK)
		{
			goto EXIT;
		}
		::EnterCriticalSection(&cs_g_bIsSignIn);
		g_bIsSignIn = TRUE;
	}
	::LeaveCriticalSection(&cs_g_bIsSignIn);

EXIT:
	DBGOUTEX2("[INFO0002]WmSignInProc Function Ends.\n");
	return nRetValue;
}

////////////////////////////////////////////
//
// WmSignOutProc
//
////////////////////////////////////////////
LRESULT WmSignOutProc(void)
{
	LRESULT nRetValue = S_OK;

	DBGOUTEX2("[INFO0001]WmSignOutProc Function Starts.\n");

	::EnterCriticalSection(&cs_g_bIsSignIn);
	if(g_bIsSignIn)
	{
		::LeaveCriticalSection(&cs_g_bIsSignIn);
		nRetValue = ByeSend();
		if(nRetValue != S_OK)
		{
			goto EXIT;
		}	
		::EnterCriticalSection(&cs_g_bIsSignIn);
		g_bIsSignIn = FALSE;
	}
	::LeaveCriticalSection(&cs_g_bIsSignIn);

EXIT:
	DBGOUTEX2("[INFO0002]WmSignOutProc Function Ends.\n");
	return nRetValue;
}


////////////////////////////////////////////
//
// WndProc
//
////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PCOPYDATASTRUCT pData;

	switch(uMsg)
	{
	case WM_TRAY_NOTIFY: 
		if(lParam == WM_LBUTTONUP)
		{
			return WmTrayNotifyProc(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
		}
		return S_OK;

	case WM_COMMAND:
		return WmCommandProc(hWnd, HIWORD(wParam), LOWORD(wParam), (HWND)lParam);

	case WM_COPYDATA:
		pData = (PCOPYDATASTRUCT)lParam;		
		switch(pData->dwData)
		{
		case FLG_OBJECT_SET:
			return SetObject(pData);
		case FLG_OBJECT_DEL:
			return DeleteObject(pData);
		case FLG_OBJECT_ENUM1:
		case FLG_OBJECT_ENUM2:
		case FLG_OBJECT_ENUM3:
		case FLG_OBJECT_ENUM4:
			return EnumObjects(pData);
		case FLG_EVENT_REG:
			return RegisterEvent(pData);
		}

	case CM_UNREGISTER_EVENT:
		return 1;

	case CM_ENUM_PEOPLENEARME:
		return EnumPeopleNearMe();

	case CM_SIGNIN:
		return WmSignInProc();

	case CM_SHUTDOWN:
		WmSignOutProc();
		::PostQuitMessage(0);
		return S_OK;

	case CM_SIGNOUT:
		return WmSignOutProc();

	default:
		return ::DefWindowProc(hWnd, uMsg, wParam, lParam);	
	}
}

////////////////////////////////////////////
//
// WinMain
//
////////////////////////////////////////////
#ifdef P2PHOSTWM
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#else
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
#endif
{
	MSG msg;
	DBGOUTEX2("[INFO0001]WinMain Function Starts.\n");

	//+++ 開始処理 +++
	if (!Initialize(hInstance, lpCmdLine))
	{
		DBGOUTEX2("[EROR1001]Initialize Error.\n");
		return	0;
	}

	//+++ メインメッセージループ +++
	while (::GetMessage(&msg, NULL, 0, 0)) 
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	//+++ 終了処理 +++
	Terminate();

	DBGOUTEX2("[INFO0002]WinMain Function Ends.\n");

	return msg.wParam;
}

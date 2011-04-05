#pragma once
#include "resourceppc.h"

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM		    	MyRegisterClass(HINSTANCE, LPTSTR);
BOOL			    InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

BOOL                InitP2PLibrary(void);
void                terminate(void);
DWORD WINAPI EventHandler(LPVOID lpv);
void SignInProc(void);
void SignOutProc(void);
void StartupProc(void);
void ShutdownProc(void);
void SetObjectProc(void);
void DeleteObjectProc(void);
void EnumPeopleNearMe(void);
void EnumObjects(void);
void RegiesterEvent(void);

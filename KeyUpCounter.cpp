// KeyUpCounter.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "KeyUpCounter.h"

#include <shellapi.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

class __declspec(uuid("9D0B8B92-4E1C-488e-A1E1-2331AFCE2CB5")) PrinterIcon;
UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;
UINT const WMAPP_HIDEFLYOUT = WM_APP + 2;

#define MAX_LOADSTRING 100
#define PCOUNTER_SIZE 1024
#define FILE_PATH_MAX 256
// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWnd;                                      // the main windows handle
HHOOK hKeyboardHook = 0;                        // the keyboard low-level hook handle
UINT pCounter[PCOUNTER_SIZE] = {0};                  // counter
WCHAR szStaticFile[MAX_LOADSTRING];

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


VOID SaveDataToFile(UINT* uCounters, int uCounterSize, WCHAR* szFileName);
VOID ReadDataFromFile(UINT* uCounters, int uCounterSize, WCHAR* szFileName);
LRESULT CALLBACK KeyboardHooker(int nCode, WPARAM wParam, LPARAM lParam);
BOOL AddNotificationIcon(HWND hwnd);
BOOL DeleteNotificationIcon();
VOID ShowContextMenu(HWND hwnd, HMENU hMenu, POINT pt);
VOID SetBootMenuChecked(BOOL bChecked);
BOOL IsOpenOnBootValid();
VOID SetOpenOnBoot();
VOID ClearOpenOnBoot();

VOID SaveDataToFile(UINT* uCounters, int uCounterSize, WCHAR* szFileName) {
    HANDLE hFile = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    WriteFile(hFile, uCounters, uCounterSize * sizeof(UINT), NULL, NULL);
    CloseHandle(hFile);
}

VOID ReadDataFromFile(UINT* uCounters, int uCounterSize, WCHAR* szFileName) {
    HANDLE hFile = CreateFile(szFileName, GENERIC_READ | GENERIC_WRITE, NULL, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    ReadFile(hFile, uCounters, uCounterSize * sizeof(UINT), NULL, NULL);
    CloseHandle(hFile);
}

LRESULT CALLBACK KeyboardHooker(int nCode, WPARAM wParam, LPARAM lParam) {
    KBDLLHOOKSTRUCT* ks = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
    if (WM_KEYUP == wParam || WM_SYSKEYUP == wParam) {
        pCounter[ks->vkCode]++;
        InvalidateRect(hWnd, NULL, false);
    }
    return CallNextHookEx(0, nCode, wParam, lParam);
}

BOOL AddNotificationIcon(HWND hwnd)
{
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.hWnd = hwnd;
    // add the icon, setting the icon, tooltip, and callback message.
    // the icon will be identified with the GUID
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP | NIF_GUID;
    nid.guidItem = __uuidof(PrinterIcon);
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_KEYUPCOUNTER));
    lstrcpynW(nid.szTip, szTitle, MAX_LOADSTRING);
    Shell_NotifyIcon(NIM_ADD, &nid);
    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL DeleteNotificationIcon()
{
    NOTIFYICONDATA nid = { sizeof(nid) };
    nid.uFlags = NIF_GUID;
    nid.guidItem = __uuidof(PrinterIcon);
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

VOID ShowContextMenu(HWND hwnd,HMENU hMenu, POINT pt)
{
    if (hMenu)
    {
        HMENU hSubMenu = GetSubMenu(hMenu, 0);
        if (hSubMenu)
        {
            // our window must be foreground before calling TrackPopupMenu or the menu will not disappear when the user clicks away
            SetForegroundWindow(hwnd);

            // respect menu drop alignment
            UINT uFlags = TPM_RIGHTBUTTON;
            if (GetSystemMetrics(SM_MENUDROPALIGNMENT) != 0)
            {
                uFlags |= TPM_RIGHTALIGN;
            }
            else
            {
                uFlags |= TPM_LEFTALIGN;
            }

            TrackPopupMenuEx(hSubMenu, uFlags, pt.x, pt.y, hwnd, NULL);
        }
    }
}

VOID SetBootMenuChecked(BOOL bChecked) {
    CheckMenuItem(GetMenu(hWnd), ID_OPTION_BOOTSTART, bChecked ? MF_CHECKED : MF_UNCHECKED);
}

BOOL IsOpenOnBootValid() {
    HKEY hKey;
    BYTE szBootPath[FILE_PATH_MAX] = { 0 };
    DWORD uBootPathLen = FILE_PATH_MAX;
    RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), NULL, KEY_READ, &hKey);
    RegQueryValueEx(hKey, szTitle, NULL, NULL, szBootPath, &uBootPathLen);
    TCHAR szSelfPath[FILE_PATH_MAX] = { 0 };
    GetModuleFileName(NULL, szSelfPath, FILE_PATH_MAX);
    RegCloseKey(hKey);
    return !lstrcmpW(szSelfPath, reinterpret_cast<PTCHAR>(szBootPath));
}

VOID SetOpenOnBoot() {
    HKEY hKey;
    BYTE szBootPath[FILE_PATH_MAX] = { 0 };
    DWORD uBootPathLen = FILE_PATH_MAX;
    RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), NULL, KEY_SET_VALUE, &hKey);
    TCHAR szSelfPath[FILE_PATH_MAX] = { 0 };
    GetModuleFileName(NULL, szSelfPath, FILE_PATH_MAX);
    RegSetValueEx(hKey, szTitle, NULL, REG_SZ, reinterpret_cast<BYTE*>(szSelfPath), lstrlenW(szSelfPath) * sizeof(TCHAR));
    RegCloseKey(hKey);
}

VOID ClearOpenOnBoot() {
    HKEY hKey;
    BYTE szBootPath[FILE_PATH_MAX] = { 0 };
    DWORD uBootPathLen = FILE_PATH_MAX;
    RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), NULL, KEY_SET_VALUE, &hKey);
    RegDeleteValue(hKey, szTitle);
    RegCloseKey(hKey);
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_KEYUPCOUNTER, szWindowClass, MAX_LOADSTRING);
    LoadStringW(hInstance, IDS_STATIC_FILE, szStaticFile, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_KEYUPCOUNTER));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_KEYUPCOUNTER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_KEYUPCOUNTER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, SW_HIDE);
   UpdateWindow(hWnd);

   return TRUE;
}

VOID SaveFileTimerProc(
    HWND unnamedParam1,
    UINT unnamedParam2,
    UINT_PTR unnamedParam3,
    DWORD unnamedParam4
)
{
    SaveDataToFile(pCounter, PCOUNTER_SIZE, szStaticFile);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        AddNotificationIcon(hWnd);
        ReadDataFromFile(pCounter, PCOUNTER_SIZE, szStaticFile);
        hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHooker, GetModuleHandle(0), 0);
        SetTimer(hWnd, 0, 1000, SaveFileTimerProc);
        break;

    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case ID_OPTION_BOOTSTART:
                if (IsOpenOnBootValid()) {
                    ClearOpenOnBoot();
                }
                else {
                    SetOpenOnBoot();
                }
                SetBootMenuChecked(IsOpenOnBootValid());
                break;
            case ID_FILE_SAVETOFILE:
                SaveDataToFile(pCounter, PCOUNTER_SIZE, szStaticFile);
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            for (int i = 0; i < 255; i++) {
                char out[64] = { 0 };
                char count[8] = { 0 };
                _itoa_s(i, out, 8, 10);
                _itoa_s(pCounter[i], count, 8, 10);
                strcat_s(out, ":");
                strcat_s(out, count);
                TextOutA(hdc, (i/20)*80, (i%20)* 20, out, static_cast<int>(strlen(out)));
            }
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        DeleteNotificationIcon();
        UnhookWindowsHookEx(hKeyboardHook);
        PostQuitMessage(0);
        break;

    case WMAPP_NOTIFYCALLBACK:
        switch (LOWORD(lParam))
        {
            case WM_CONTEXTMENU:
            {
                SetBootMenuChecked(IsOpenOnBootValid());
                POINT pt;
                GetCursorPos(&pt);
                //POINT const pt = { 0,0 };
                ShowContextMenu(hWnd, GetMenu(hWnd), pt);
            }
            break;
            case WM_LBUTTONUP:
                ShowWindow(hWnd, SW_SHOW);
                break;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

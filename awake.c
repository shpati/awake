#include <windows.h>
#include <stdio.h>

// Manual definition for NOTIFYICONDATA since shellapi.h may not be available in TCC
typedef struct _NOTIFYICONDATA {
    DWORD cbSize;
    HWND hWnd;
    UINT uID;
    UINT uFlags;
    UINT uCallbackMessage;
    HICON hIcon;
    char szTip[128];
} NOTIFYICONDATA, *PNOTIFYICONDATA;

#define NIM_ADD 0x00000000
#define NIM_MODIFY 0x00000001
#define NIM_DELETE 0x00000002
#define NIF_MESSAGE 0x00000001
#define NIF_ICON 0x00000002
#define NIF_TIP 0x00000004

// Shell_NotifyIcon function declaration
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATA lpData);
#define Shell_NotifyIcon Shell_NotifyIconA

HICON CreateCustomIcon(COLORREF color) {
    HDC hdc = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 16, 16);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
    
    HBRUSH hBrush = CreateSolidBrush(color);
    RECT rect = {0, 0, 16, 16};
    FillRect(hdcMem, &rect, hBrush);
    DeleteObject(hBrush);
    
    HBRUSH hBlackBrush = CreateSolidBrush(RGB(0, 0, 0));
    FrameRect(hdcMem, &rect, hBlackBrush);
    DeleteObject(hBlackBrush);
    
    SelectObject(hdcMem, hOldBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);
    
    ICONINFO iconInfo;
    iconInfo.fIcon = TRUE;
    iconInfo.xHotspot = 0;
    iconInfo.yHotspot = 0;
    iconInfo.hbmMask = hBitmap;
    iconInfo.hbmColor = hBitmap;
    
    HICON hIcon = CreateIconIndirect(&iconInfo);
    DeleteObject(hBitmap);
    
    return hIcon;
}

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_APP_ICON 1001
#define ID_TRAY_EXIT 1002
#define ID_TRAY_ABOUT 1003
#define ID_TRAY_SHOW_DIALOG 1004
#define ID_TRAY_TOGGLE 1005

#define ID_ACTIVE_15MIN 1010
#define ID_ACTIVE_30MIN 1011
#define ID_ACTIVE_45MIN 1012
#define ID_ACTIVE_1HR 1013
#define ID_ACTIVE_2HR 1014
#define ID_ACTIVE_4HR 1015
#define ID_ACTIVE_6HR 1016
#define ID_ACTIVE_8HR 1017
#define ID_ACTIVE_12HR 1018
#define ID_ACTIVE_16HR 1019
#define ID_ACTIVE_20HR 1020
#define ID_ACTIVE_24HR 1021
#define ID_ACTIVE_CUSTOM 1022

#define ID_INACTIVE_15MIN 1030
#define ID_INACTIVE_30MIN 1031
#define ID_INACTIVE_45MIN 1032
#define ID_INACTIVE_1HR 1033
#define ID_INACTIVE_2HR 1034
#define ID_INACTIVE_4HR 1035
#define ID_INACTIVE_6HR 1036
#define ID_INACTIVE_8HR 1037
#define ID_INACTIVE_12HR 1038
#define ID_INACTIVE_16HR 1039
#define ID_INACTIVE_20HR 1040
#define ID_INACTIVE_24HR 1041
#define ID_INACTIVE_CUSTOM 1042

#define TIMER_KEEPAWAKE 1
#define TIMER_COUNTDOWN 2
#define TIMER_INACTIVE 3

NOTIFYICONDATA nid;
HMENU hPopMenu;
HMENU hActiveSubmenu;
HMENU hInactiveSubmenu;
BOOL isActive = FALSE;
int remainingMinutes = 0;
BOOL timerMode = FALSE;
BOOL inactiveTimerMode = FALSE;
HICON hIconActive;
HICON hIconInactive;

void SendF15Key() {
    INPUT input;
    memset(&input, 0, sizeof(INPUT));
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = VK_F15;
    input.ki.dwFlags = 0;
    SendInput(1, &input, sizeof(INPUT));
    
    input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void UpdateTrayIcon(HWND hwnd) {
    if (inactiveTimerMode) {
        sprintf(nid.szTip, "Awake - Inactive for %d min", remainingMinutes);
        nid.hIcon = hIconInactive;
    } else if (isActive) {
        if (timerMode && remainingMinutes > 0) {
            sprintf(nid.szTip, "Awake - Active (%d min)", remainingMinutes);
        } else {
            strcpy(nid.szTip, "Awake - Active");
        }
        nid.hIcon = hIconActive;
    } else {
        strcpy(nid.szTip, "Awake - Inactive");
        nid.hIcon = hIconInactive;
    }
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void ToggleActive(HWND hwnd) {
    isActive = !isActive;
    if (isActive) {
        SetTimer(hwnd, TIMER_KEEPAWAKE, 60000, NULL);
        KillTimer(hwnd, TIMER_INACTIVE);
        inactiveTimerMode = FALSE;
    } else {
        KillTimer(hwnd, TIMER_KEEPAWAKE);
        KillTimer(hwnd, TIMER_COUNTDOWN);
        timerMode = FALSE;
        remainingMinutes = 0;
    }
    UpdateTrayIcon(hwnd);
}

void SetActiveTimer(HWND hwnd, int minutes) {
    if (!isActive) {
        isActive = TRUE;
        SetTimer(hwnd, TIMER_KEEPAWAKE, 60000, NULL);
    }
    
    KillTimer(hwnd, TIMER_INACTIVE);
    inactiveTimerMode = FALSE;
    
    timerMode = TRUE;
    remainingMinutes = minutes;
    SetTimer(hwnd, TIMER_COUNTDOWN, 60000, NULL);
    UpdateTrayIcon(hwnd);
}

void SetInactiveTimer(HWND hwnd, int minutes) {
    if (isActive) {
        isActive = FALSE;
        KillTimer(hwnd, TIMER_KEEPAWAKE);
    }
    
    KillTimer(hwnd, TIMER_COUNTDOWN);
    timerMode = FALSE;
    
    inactiveTimerMode = TRUE;
    remainingMinutes = minutes;
    SetTimer(hwnd, TIMER_INACTIVE, 60000, NULL);
    UpdateTrayIcon(hwnd);
}

char customInputBuffer[20];
HWND hCustomEditBox = NULL;
BOOL dialogResultOK = FALSE;

LRESULT CALLBACK InputDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            CreateWindowEx(0, "STATIC", "Enter hours (1 to 9000):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                15, 15, 250, 20,
                hwnd, NULL, GetModuleHandle(NULL), NULL);
            
            hCustomEditBox = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP,
                15, 40, 250, 25,
                hwnd, (HMENU)100, GetModuleHandle(NULL), NULL);
            
            CreateWindowEx(0, "BUTTON", "OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                65, 80, 70, 28,
                hwnd, (HMENU)IDOK, GetModuleHandle(NULL), NULL);
            
            CreateWindowEx(0, "BUTTON", "Cancel",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                145, 80, 70, 28,
                hwnd, (HMENU)IDCANCEL, GetModuleHandle(NULL), NULL);
            
            SetFocus(hCustomEditBox);
            return TRUE;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK) {
                GetWindowText(hCustomEditBox, customInputBuffer, sizeof(customInputBuffer));
                dialogResultOK = TRUE;
                DestroyWindow(hwnd);
                return TRUE;
            } else if (LOWORD(wParam) == IDCANCEL) {
                customInputBuffer[0] = '\0';
                dialogResultOK = FALSE;
                DestroyWindow(hwnd);
                return TRUE;
            }
            break;
            
        case WM_CLOSE:
            customInputBuffer[0] = '\0';
            dialogResultOK = FALSE;
            DestroyWindow(hwnd);
            return TRUE;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return TRUE;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int GetCustomTime(HWND hwndParent, const char* title) {
    int hours;
    WNDCLASSEX wcInput;
    
    while (1) {
        customInputBuffer[0] = '\0';
        dialogResultOK = FALSE;
        
        memset(&wcInput, 0, sizeof(WNDCLASSEX));
        wcInput.cbSize = sizeof(WNDCLASSEX);
        wcInput.lpfnWndProc = InputDialogProc;
        wcInput.hInstance = GetModuleHandle(NULL);
        wcInput.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wcInput.lpszClassName = "AwakeInputDialog";
        wcInput.hCursor = LoadCursor(NULL, IDC_ARROW);
        
        if (!RegisterClassEx(&wcInput)) {
            UnregisterClass("AwakeInputDialog", GetModuleHandle(NULL));
            RegisterClassEx(&wcInput);
        }
        
        HWND hDialog = CreateWindowEx(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            "AwakeInputDialog",
            title,
            WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,
            (GetSystemMetrics(SM_CXSCREEN) - 300) / 2,
            (GetSystemMetrics(SM_CYSCREEN) - 150) / 2,
            300, 150,
            hwndParent, NULL, GetModuleHandle(NULL), NULL);
        
        if (!hDialog) {
            UnregisterClass("AwakeInputDialog", GetModuleHandle(NULL));
            return -1;
        }
        
        ShowWindow(hDialog, SW_SHOW);
        UpdateWindow(hDialog);
        SendMessage(hDialog, WM_INITDIALOG, 0, 0);
        
        EnableWindow(hwndParent, FALSE);
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (!IsWindow(hDialog)) {
                break;
            }
            
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
                SendMessage(hDialog, WM_COMMAND, MAKEWPARAM(IDOK, BN_CLICKED), 0);
                continue;
            }
            if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
                SendMessage(hDialog, WM_COMMAND, MAKEWPARAM(IDCANCEL, BN_CLICKED), 0);
                continue;
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        EnableWindow(hwndParent, TRUE);
        SetForegroundWindow(hwndParent);
        
        UnregisterClass("AwakeInputDialog", GetModuleHandle(NULL));
        
        if (!dialogResultOK || strlen(customInputBuffer) == 0) {
            return -1;
        }
        
        hours = atoi(customInputBuffer);
        
        if (hours >= 1 && hours <= 9000) {
            return hours * 60;
        }
        
        MessageBox(hwndParent, "Please enter a number between 1 and 9000.", "Invalid Input", MB_OK | MB_ICONWARNING);
    }
}

void ShowContextMenu(HWND hwnd) {
    POINT pt;
    GetCursorPos(&pt);
    
    SetForegroundWindow(hwnd);
    
    UINT uFlags = MF_BYCOMMAND | MF_STRING;
    
    ModifyMenu(hPopMenu, ID_TRAY_TOGGLE, uFlags | (isActive ? MF_CHECKED : MF_UNCHECKED), 
               ID_TRAY_TOGGLE, "Active");
    
    TrackPopupMenu(hPopMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            memset(&nid, 0, sizeof(NOTIFYICONDATA));
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = ID_TRAY_APP_ICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_TRAYICON;
            
            hIconActive = CreateCustomIcon(RGB(0, 255, 0));
            hIconInactive = CreateCustomIcon(RGB(128, 128, 128));
            nid.hIcon = hIconInactive;
            
            strcpy(nid.szTip, "Awake - Inactive");
            Shell_NotifyIcon(NIM_ADD, &nid);
            
            hPopMenu = CreatePopupMenu();
            AppendMenu(hPopMenu, MF_STRING, ID_TRAY_ABOUT, "About");
            AppendMenu(hPopMenu, MF_SEPARATOR, 0, NULL);
            
            hActiveSubmenu = CreatePopupMenu();
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_15MIN, "15 minutes");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_30MIN, "30 minutes");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_45MIN, "45 minutes");
            AppendMenu(hActiveSubmenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_1HR, "1 hour");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_2HR, "2 hours");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_4HR, "4 hours");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_6HR, "6 hours");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_8HR, "8 hours");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_12HR, "12 hours");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_16HR, "16 hours");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_20HR, "20 hours");
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_24HR, "24 hours");
            AppendMenu(hActiveSubmenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hActiveSubmenu, MF_STRING, ID_ACTIVE_CUSTOM, "Custom...");
            AppendMenu(hPopMenu, MF_STRING | MF_POPUP, (UINT_PTR)hActiveSubmenu, "Active for");
            
            hInactiveSubmenu = CreatePopupMenu();
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_15MIN, "15 minutes");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_30MIN, "30 minutes");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_45MIN, "45 minutes");
            AppendMenu(hInactiveSubmenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_1HR, "1 hour");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_2HR, "2 hours");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_4HR, "4 hours");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_6HR, "6 hours");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_8HR, "8 hours");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_12HR, "12 hours");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_16HR, "16 hours");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_20HR, "20 hours");
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_24HR, "24 hours");
            AppendMenu(hInactiveSubmenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hInactiveSubmenu, MF_STRING, ID_INACTIVE_CUSTOM, "Custom...");
            AppendMenu(hPopMenu, MF_STRING | MF_POPUP, (UINT_PTR)hInactiveSubmenu, "Inactive for");
            
            AppendMenu(hPopMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hPopMenu, MF_STRING, ID_TRAY_SHOW_DIALOG, "Show Dialog");
            AppendMenu(hPopMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hPopMenu, MF_STRING, ID_TRAY_TOGGLE, "Active");
            AppendMenu(hPopMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hPopMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
            break;
            
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowContextMenu(hwnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ToggleActive(hwnd);
            }
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                    DestroyWindow(hwnd);
                    break;
                case ID_TRAY_TOGGLE:
                    ToggleActive(hwnd);
                    break;
                case ID_TRAY_ABOUT:
                    MessageBox(hwnd, 
                        "Awake v1.0\n\n"
                        "Keeps your PC awake by simulating keyboard activity.\n\n"
                        "Double-click the tray icon to toggle active/inactive.\n"
                        "Right-click for options.",
                        "About Awake", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_TRAY_SHOW_DIALOG:
                    MessageBox(hwnd, 
                        isActive ? "Status: Active" : "Status: Inactive",
                        "Awake Status", MB_OK | MB_ICONINFORMATION);
                    break;
                case ID_ACTIVE_15MIN:
                    SetActiveTimer(hwnd, 15);
                    break;
                case ID_ACTIVE_30MIN:
                    SetActiveTimer(hwnd, 30);
                    break;
                case ID_ACTIVE_45MIN:
                    SetActiveTimer(hwnd, 45);
                    break;
                case ID_ACTIVE_1HR:
                    SetActiveTimer(hwnd, 60);
                    break;
                case ID_ACTIVE_2HR:
                    SetActiveTimer(hwnd, 120);
                    break;
                case ID_ACTIVE_4HR:
                    SetActiveTimer(hwnd, 240);
                    break;
                case ID_ACTIVE_6HR:
                    SetActiveTimer(hwnd, 360);
                    break;
                case ID_ACTIVE_8HR:
                    SetActiveTimer(hwnd, 480);
                    break;
                case ID_ACTIVE_12HR:
                    SetActiveTimer(hwnd, 720);
                    break;
                case ID_ACTIVE_16HR:
                    SetActiveTimer(hwnd, 960);
                    break;
                case ID_ACTIVE_20HR:
                    SetActiveTimer(hwnd, 1200);
                    break;
                case ID_ACTIVE_24HR:
                    SetActiveTimer(hwnd, 1440);
                    break;
                case ID_ACTIVE_CUSTOM:
                    {
                        int minutes = GetCustomTime(hwnd, "Active");
                        if (minutes > 0) {
                            SetActiveTimer(hwnd, minutes);
                        }
                    }
                    break;
                case ID_INACTIVE_15MIN:
                    SetInactiveTimer(hwnd, 15);
                    break;
                case ID_INACTIVE_30MIN:
                    SetInactiveTimer(hwnd, 30);
                    break;
                case ID_INACTIVE_45MIN:
                    SetInactiveTimer(hwnd, 45);
                    break;
                case ID_INACTIVE_1HR:
                    SetInactiveTimer(hwnd, 60);
                    break;
                case ID_INACTIVE_2HR:
                    SetInactiveTimer(hwnd, 120);
                    break;
                case ID_INACTIVE_4HR:
                    SetInactiveTimer(hwnd, 240);
                    break;
                case ID_INACTIVE_6HR:
                    SetInactiveTimer(hwnd, 360);
                    break;
                case ID_INACTIVE_8HR:
                    SetInactiveTimer(hwnd, 480);
                    break;
                case ID_INACTIVE_12HR:
                    SetInactiveTimer(hwnd, 720);
                    break;
                case ID_INACTIVE_16HR:
                    SetInactiveTimer(hwnd, 960);
                    break;
                case ID_INACTIVE_20HR:
                    SetInactiveTimer(hwnd, 1200);
                    break;
                case ID_INACTIVE_24HR:
                    SetInactiveTimer(hwnd, 1440);
                    break;
                case ID_INACTIVE_CUSTOM:
                    {
                        int minutes = GetCustomTime(hwnd, "Inactive");
                        if (minutes > 0) {
                            SetInactiveTimer(hwnd, minutes);
                        }
                    }
                    break;
            }
            break;
            
        case WM_TIMER:
            if (wParam == TIMER_KEEPAWAKE) {
                SendF15Key();
            } else if (wParam == TIMER_COUNTDOWN) {
                remainingMinutes--;
                if (remainingMinutes <= 0) {
                    KillTimer(hwnd, TIMER_COUNTDOWN);
                    timerMode = FALSE;
                    ToggleActive(hwnd);
                } else {
                    UpdateTrayIcon(hwnd);
                }
            } else if (wParam == TIMER_INACTIVE) {
                remainingMinutes--;
                if (remainingMinutes <= 0) {
                    KillTimer(hwnd, TIMER_INACTIVE);
                    inactiveTimerMode = FALSE;
                    ToggleActive(hwnd);
                } else {
                    UpdateTrayIcon(hwnd);
                }
            }
            break;
            
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            if (hIconActive) DestroyIcon(hIconActive);
            if (hIconInactive) DestroyIcon(hIconInactive);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG msg;
    
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "AwakeClass";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        "AwakeClass",
        "Awake",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 240, 120,
        NULL, NULL, hInstance, NULL);
    
    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }
    
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}

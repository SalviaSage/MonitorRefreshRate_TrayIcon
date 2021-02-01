// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.


#include "resource.h"
#include <windows.h>
#include <tchar.h>

HINSTANCE g_hInst = NULL;

DWORD varHz = NULL;

UINT const WMAPP_NOTIFYCALLBACK = WM_APP + 1;

wchar_t const szWindowClass[] = L"MonitorRefreshRate_TrayIcon";

// Forward declarations of functions included in this code module:
void                RegisterWindowClass(PCWSTR pszClassName, WNDPROC lpfnWndProc);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void                ShowContextMenu(HWND hwnd, POINT pt);
DWORD               UpdateIcon(HWND hwnd, DWORD txtHz);
DWORD               CurrentHz();
BOOL                AddNotificationIcon(HWND hwnd);
BOOL                DeleteNotificationIcon(HWND hwnd);
HCURSOR             CreateAlphaCursor(LPCTSTR txt);


int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR /*lpCmdLine*/, int nCmdShow)
{
    g_hInst = hInstance;
    RegisterWindowClass(szWindowClass, WndProc);

    // Create the main window. This could be a hidden window if you don't need
    // any UI other than the notification icon.
    WCHAR sTitle[100];
    LoadString(hInstance, APP_TITLE, sTitle, ARRAYSIZE(sTitle));
    HWND hwnd = CreateWindow(szWindowClass, sTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, 250, 200, NULL, NULL, g_hInst, NULL);
    if (hwnd)
    {
        ShowWindow(hwnd, nCmdShow = SW_HIDE);

        // Main message loop:
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}

void RegisterWindowClass(PCWSTR pszClassName, WNDPROC lpfnWndProc)
{
    WNDCLASSEX wcex = {sizeof(wcex)};
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = lpfnWndProc;
    wcex.hInstance      = g_hInst;
    wcex.lpszClassName  = pszClassName;
    RegisterClassEx(&wcex);
}

DWORD UpdateIcon(HWND hwnd, DWORD txtHz) {
    WCHAR Hz[4];
    swprintf_s(Hz, 4, L"%d", txtHz);
    NOTIFYICONDATA nid;
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.hIcon = CreateAlphaCursor(Hz);
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    nid.uID = 1;
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    LoadString(g_hInst, TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    return txtHz;
}

DWORD CurrentHz() {
    DEVMODE dm;
    dm.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
    return dm.dmDisplayFrequency;
}

HCURSOR CreateAlphaCursor(LPCTSTR txt)
{
    HDC hMemDC;
    DWORD dwWidth, dwHeight;
    BITMAPV5HEADER bi;
    HBITMAP hBitmap, hOldBitmap;
    void* lpBits;
    DWORD x, y;
    HCURSOR hAlphaCursor = NULL;
    HFONT hFont;

    dwWidth = 16;  // width of cursor
    dwHeight = 16;  // height of cursor

    ZeroMemory(&bi, sizeof(BITMAPV5HEADER));
    bi.bV5Size = sizeof(BITMAPV5HEADER);
    bi.bV5Width = dwWidth;
    bi.bV5Height = dwHeight;
    bi.bV5Planes = 1;
    bi.bV5BitCount = 32;
    bi.bV5Compression = BI_BITFIELDS;
    // The following mask specification specifies a supported 32 BPP
    // alpha format for Windows XP.
    bi.bV5RedMask = 0x00FF0000;
    bi.bV5GreenMask = 0x0000FF00;
    bi.bV5BlueMask = 0x000000FF;
    bi.bV5AlphaMask = 0xFF000000;

    HDC hdc;
    hdc = GetDC(NULL);

    // Create the DIB section with an alpha channel.
    hBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bi, DIB_RGB_COLORS,
        (void**)&lpBits, NULL, (DWORD)0);

    hMemDC = CreateCompatibleDC(hdc);
    ReleaseDC(NULL, hdc);

    // Draw something on the DIB section.
    hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap);
    PatBlt(hMemDC, 0, 0, dwWidth, dwHeight, WHITENESS);
    SetTextColor(hMemDC, RGB(75, 20, 10));
    SetBkMode(hMemDC, TRANSPARENT);
    hFont = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Impact"));
    hFont = (HFONT)SelectObject(hMemDC, hFont);
    TextOut(hMemDC, -1, 0, txt, lstrlen(txt));
    SelectObject(hMemDC, hOldBitmap);
    DeleteDC(hMemDC);

    // Create an empty mask bitmap.
    HBITMAP hMonoBitmap = CreateBitmap(dwWidth, dwHeight, 1, 1, NULL);

    // Set the alpha values for each pixel in the cursor so that
    // the complete cursor is semi-transparent.
    DWORD* lpdwPixel;
    lpdwPixel = (DWORD*)lpBits;
    for (x = 0;x < dwWidth;x++)
        for (y = 0;y < dwHeight;y++)
        {
            // Clear the alpha bits
            *lpdwPixel &= 0x00FFFFFF;
            // Set the alpha bits to 0x9F (semi-transparent)
            *lpdwPixel |= 0x9F000000;
            lpdwPixel++;
        }

    ICONINFO ii;
    ii.fIcon = TRUE;  // Change fIcon to TRUE to create an alpha icon
    ii.xHotspot = 0;
    ii.yHotspot = 0;
    ii.hbmMask = hMonoBitmap;
    ii.hbmColor = hBitmap;

    // Create the alpha cursor with the alpha DIB section.
    hAlphaCursor = CreateIconIndirect(&ii);

    DeleteObject(hBitmap);
    DeleteObject(hMonoBitmap);

    return hAlphaCursor;
}

BOOL AddNotificationIcon(HWND hwnd)
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.hWnd = hwnd;
    // add the icon, setting the icon, tooltip, and callback message.
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE | NIF_SHOWTIP;
    nid.uID = 1;
    nid.uCallbackMessage = WMAPP_NOTIFYCALLBACK;
    LoadString(g_hInst, TOOLTIP, nid.szTip, ARRAYSIZE(nid.szTip));
    Shell_NotifyIcon(NIM_ADD, &nid);

    varHz = UpdateIcon(hwnd ,CurrentHz());
    SetTimer(hwnd, TIMER1, 5000, NULL);
    // NOTIFYICON_VERSION_4 is prefered
    nid.uVersion = NOTIFYICON_VERSION_4;
    return Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

BOOL DeleteNotificationIcon(HWND hwnd)
{
    NOTIFYICONDATA nid = {sizeof(nid)};
    nid.hWnd = hwnd;
    nid.uID = 1;
    return Shell_NotifyIcon(NIM_DELETE, &nid);
}

int queryUserNotificationState() {
    QUERY_USER_NOTIFICATION_STATE notificationState;
    HRESULT hr = SHQueryUserNotificationState(&notificationState);
    int returnValue = -1;

    if (SUCCEEDED(hr))
    {
        switch (notificationState) {
        case QUNS_NOT_PRESENT:
            returnValue = 1;
            break;
        case QUNS_BUSY:
            returnValue = 2;
            break;
        case QUNS_RUNNING_D3D_FULL_SCREEN:
            returnValue = 3;
            break;
        case QUNS_PRESENTATION_MODE:
            returnValue = 4;
            break;
        case QUNS_ACCEPTS_NOTIFICATIONS:
            returnValue = 5;
            break;
        case QUNS_QUIET_TIME:
            returnValue = 6;
            break;
        case QUNS_APP:
            returnValue = 7;
            break;
        }
    }
    return returnValue;
}


void ShowContextMenu(HWND hwnd, POINT pt)
{
    HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDC_CONTEXTMENU));
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
        DestroyMenu(hMenu);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        // add the notification icon
        if (!AddNotificationIcon(hwnd))
        {
            MessageBox(hwnd,
                L"Please read the ReadMe.txt file for troubleshooting",
                L"Error adding icon", MB_OK);
            return -1;
        }
        break;
    case WM_COMMAND:
        {
            int const wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hwnd);
                break;
            default:
                return DefWindowProc(hwnd, message, wParam, lParam);
            }
        }
        break;
    case WMAPP_NOTIFYCALLBACK:
        switch (LOWORD(lParam))
        {
        case WM_CONTEXTMENU:
            {
                POINT const pt = { LOWORD(wParam), HIWORD(wParam) };
                ShowContextMenu(hwnd, pt);
            }
            break;
        }
        break;
    case WM_TIMER:
        if (wParam == TIMER1)
        {
            if (queryUserNotificationState() == 5 && varHz != CurrentHz())
            {
                varHz = UpdateIcon(hwnd, CurrentHz());
            }
        }
        break;
    case WM_DESTROY:
        DeleteNotificationIcon(hwnd);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}


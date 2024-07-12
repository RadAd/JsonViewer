#include "EditInPlace.h"

#include <windowsx.h>
#include <CommCtrl.h>

namespace
{
    inline LRESULT SendParentParentNotify(HWND hWnd, WORD code)
    {
        return SendMessage(GetParent(GetParent(hWnd)), WM_COMMAND, MAKELONG(GetDlgCtrlID(hWnd), code), (LPARAM)hWnd);
    }

    LRESULT CALLBACK Edit_InPlaceProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        switch (uMsg)
        {
        case WM_CHAR:
        {
            TCHAR c = (TCHAR)wParam;
            if (c == TEXT('\x1B'))
            {
                SendParentParentNotify(hWnd, EN_CANCEL);
                DestroyWindow(hWnd);
            }
            else if (c == TEXT('\r'))
            {
                SendParentParentNotify(hWnd, EN_DONE);
                DestroyWindow(hWnd);
            }
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
        }

        case WM_KILLFOCUS:
            SendParentParentNotify(hWnd, EN_CANCEL);
            DestroyWindow(hWnd);
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);

        case WM_GETDLGCODE:
            return DefSubclassProc(hWnd, uMsg, wParam, lParam) | DLGC_WANTALLKEYS;

        default:
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
        }
    }
}

HWND EditInPlace(HWND hWnd, LPCTSTR text, LPARAM lParam, RECT rc)
{
    DWORD dwStyle = WS_CHILD | ES_AUTOHSCROLL;
    dwStyle |= ES_LEFT;

    HWND hEdit = CreateWindow(
        WC_EDIT,
        text,
        dwStyle,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hWnd,
        (HMENU)(INT_PTR)IDC_EDITINPLACE,
        NULL,
        NULL);

    SetWindowLongPtr(hEdit, GWLP_USERDATA, lParam);
    SetWindowSubclass(hEdit, Edit_InPlaceProc, 0, 0);
    SetWindowFont(hEdit, GetWindowFont(hWnd), TRUE);
    ShowWindow(hEdit, SW_SHOW);
    Edit_SetSel(hEdit, 0, -1);
    SetFocus(hEdit);

    return hEdit;
}

#pragma once
#include <Windows.h>

#define EN_CANCEL 1
#define EN_DONE 2

#define IDC_EDITINPLACE (1010)

HWND EditInPlace(HWND hWnd, LPCTSTR text, LPARAM lParam, RECT rect);
inline LPARAM EditInPlace_GetParam(HWND hWndEdit)
{
    return GetWindowLongPtr(hWndEdit, GWLP_USERDATA);
}

#include "FindDlgChain.h"

extern HWND g_hWndDlg;

const UINT WM_FINDSTRING = RegisterWindowMessage(FINDMSGSTRING);

UINT_PTR FRHookProc(HWND hWnd, UINT nCode, WPARAM wParam, LPARAM lParam)
{
    switch (nCode)
    {
    case WM_INITDIALOG:
        return TRUE;

    case WM_ACTIVATE:
        if (LOWORD(wParam))
            g_hWndDlg = hWnd;
        else if (g_hWndDlg == hWnd)
            g_hWndDlg = NULL;
        return TRUE;

    default:
        return 0;
    }
}

void FindDlgChain::Init(HWND hWnd)
{
    m_FindData.hwndOwner = hWnd;
    m_FindData.Flags = FR_DOWN;
    m_FindData.lpstrFindWhat = m_FindStr;
    m_FindData.wFindWhatLen = ARRAYSIZE(m_FindStr);
    m_FindData.Flags |= FR_ENABLEHOOK;
    m_FindData.lpfnHook = FRHookProc;
}

void FindDlgChain::Show()
{
    if (m_hFind)
        SetActiveWindow(m_hFind);
    else
        m_hFind = FindText(&m_FindData);
    _ASSERT(m_hFind);
}

void FindDlgChain::DisplayError(LPCTSTR e, LPCTSTR strTitle)
{
    MessageBox(m_hFind, e, strTitle, MB_ICONERROR | MB_OK);
}

LRESULT FindDlgChain::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;
    if (uMsg == WM_FINDSTRING)
    {
        LPFINDREPLACE pfr = (LPFINDREPLACE)lParam;
        if ((pfr->Flags & FR_DIALOGTERM) == FR_DIALOGTERM)
        {
            SetHandled(true);
            m_hFind = NULL;
        }
    }
    return ret;
}

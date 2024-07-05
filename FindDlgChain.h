#pragma once
#include "Rad/MessageHandler.h"
#include <commdlg.h>

extern const UINT WM_FINDSTRING;

class FindDlgChain : public MessageChain
{
public:
    void Init(HWND hWnd);

    void Show();

    void DisplayError(LPCTSTR  e, LPCTSTR strTitle);

protected:
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    TCHAR m_FindStr[1024] = TEXT("");
    FINDREPLACE m_FindData = { sizeof(FINDREPLACE) };
    HWND m_hFind = NULL;
};

#include "ValuesDlg.h"
#include "Rad/Windowxx.h"
#include "Rad/Convert.h"

#include "resource.h"

#include "EditInPlace.h"

INT_PTR ValuesDlg::DoModal(HWND hWndParent)
{
    return Dialog::DoModal(IDD_VALUES, hWndParent);
}

BOOL ValuesDlg::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
    HWND hWndList = GetList();
    for (const auto& s : m_values)
        ListBox_AddString(hWndList, s2t(s).c_str());

    return TRUE;
}

void ValuesDlg::OnCommand(int id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_LIST1:
        switch (codeNotify)
        {
        case LBN_DBLCLK:
        {
            int index = ListBox_GetCurSel(hWndCtl);
            if (index >= 0)
            {
                TCHAR text[1024];
                ListBox_GetText(hWndCtl, index, text);

                RECT rect = {};
                ListBox_GetItemRect(hWndCtl, index, &rect);

                EditInPlace(hWndCtl, text, index, rect);
            }
            break;
        }
        }
        break;

    case IDC_EDITINPLACE:
    {
        if (codeNotify == EN_DONE)
        {
            TCHAR text[1024];
            Edit_GetText(hWndCtl, text, ARRAYSIZE(text));

            HWND hWndList = GetList();
            int index = (int) EditInPlace_GetParam(hWndCtl);
            ListBox_DeleteString(hWndList, index);
            ListBox_InsertString(hWndList, index, text);
            ListBox_SetCurSel(hWndList, index);
        }
        break;
    }

    case IDC_ADD:
    {
        HWND hWndList = GetList();
        int index = ListBox_GetCurSel(hWndList);
        if (index >= 0)
            ++index;
        else
            index = ListBox_GetCount(hWndList);
        LPCTSTR text = TEXT("<new>");
        ListBox_InsertString(hWndList, index, text);
        ListBox_SetCurSel(hWndList, index);

        RECT rect = {};
        ListBox_GetItemRect(hWndList, index, &rect);

        EditInPlace(hWndList, text, index, rect);
        break;
    }

    case IDC_DEL:
    {
        HWND hWndList = GetList();
        int index = ListBox_GetCurSel(hWndList);
        if (index >= 0)
        {
            ListBox_DeleteString(hWndList, index);
            ListBox_SetCurSel(hWndList, index);
        }
        break;
    }

    case IDOK:
        if (codeNotify == BN_CLICKED)
        {
            m_values.clear();
            HWND hWndList = GetList();
            for (int i = 0; i < ListBox_GetCount(hWndList); ++i)
            {
                TCHAR buf[1024];
                ListBox_GetText(hWndList, i, buf);
                m_values.push_back(w2a(buf));
            }
            //[[fallthrough]];
        }

    case IDCANCEL:
    {
        if (codeNotify == BN_CLICKED)
            EndDialog(*this, id);
        break;
    }
    }
}

HWND ValuesDlg::GetList() const
{
    return GetDlgItem(*this, IDC_LIST1);
}

LRESULT ValuesDlg::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;
    switch (uMsg)
    {
        HANDLE_MSG(WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(WM_COMMAND, OnCommand);
    }

    return IsHandled() ? ret : Dialog::HandleMessage(uMsg, wParam, lParam);
}

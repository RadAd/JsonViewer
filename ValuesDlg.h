#pragma once
#include "Rad/Dialog.h"

#include <vector>
#include <string>

class ValuesDlg : public Dialog
{
public:
    std::vector<std::string> m_values;

    INT_PTR DoModal(HWND hWndParent);

private:
    BOOL OnInitDialog(HWND hwndFocus, LPARAM lParam);
    void OnCommand(int id, HWND hWndCtl, UINT codeNotify);

    HWND GetList() const;

protected:
    virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
};

#pragma once
#include "Rad/MessageHandler.h"

class ShowMenuShortcutChain : public MessageChain
{
public:
    void Init(HACCEL hAccelTable)
    {
        m_hAccel = hAccelTable;
    }

protected:
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    void OnInitMenuPopup(HMENU hMenu, UINT item, BOOL fSystemMenu);

    HACCEL m_hAccel = NULL;
};

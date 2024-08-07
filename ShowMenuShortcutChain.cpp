#include "ShowMenuShortcutChain.h"
#include "Rad/Windowxx.h"

#include <tchar.h>

#include <string>
#include <vector>

namespace
{
    std::vector<ACCEL> UnpackAccel(HACCEL hAccel)
    {
        std::vector<ACCEL> accels(CopyAcceleratorTable(hAccel, nullptr, 0));
        CopyAcceleratorTable(hAccel, accels.data(), int(accels.size()));
        return accels;
    }

    std::wstring AccelToString(const ACCEL accel)
    {
        std::wstring str;
        if (accel.fVirt & FALT)
            str += TEXT("Alt+");
        if (accel.fVirt & FCONTROL)
            str += TEXT("Ctrl+");
        if (accel.fVirt & FSHIFT)
            str += TEXT("Shift+");
        _ASSERTE(accel.fVirt & FVIRTKEY);
        {
            const bool extended = false;
            const UINT scanCode = MapVirtualKey(accel.key, MAPVK_VK_TO_VSC) | (extended ? KF_EXTENDED : 0);
            TCHAR name[100];
            const int result = GetKeyNameText(scanCode << 16, name, ARRAYSIZE(name));
            str += name;
        }
        return str;
    }
}

LRESULT ShowMenuShortcutChain::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = 0;
    switch (uMsg)
    {
        HANDLE_MSG(WM_INITMENUPOPUP, OnInitMenuPopup);
    }
    return ret;
}

void ShowMenuShortcutChain::OnInitMenuPopup(HMENU hMenu, UINT item, BOOL fSystemMenu)
{
    const std::vector<ACCEL> accels = UnpackAccel(m_hAccel);
    for (int i = 0; i < GetMenuItemCount(hMenu); ++i)
    {
        TCHAR str[100];
        MENUITEMINFO mii = {};
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_STRING;
        mii.dwTypeData = str;
        mii.cch = ARRAYSIZE(str);
        if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
        {
            const UINT ID = mii.wID;
            auto it = std::find_if(accels.begin(), accels.end(), [ID](const ACCEL& accel) { return accel.cmd == ID; });
            if (it != accels.end())
            {
                const TCHAR* tab = _tcschr(str, TEXT('\t'));
                if (tab == nullptr)
                {
                    _tcscat_s(str, TEXT("\t"));
                    _tcscat_s(str, AccelToString(*it).c_str());
                    SetMenuItemInfo(hMenu, i, TRUE, &mii);
                }
            }
        }
    }
}

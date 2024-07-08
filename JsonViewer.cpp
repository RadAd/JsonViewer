#include "Rad/Window.h"
#include "Rad/Dialog.h"
#include "Rad/Windowxx.h"
#include "Rad/Log.h"
#include "Rad/Format.h"
#include "Rad/TreeViewPlus.h"
#include "Rad/AboutDlg.h"
#include <tchar.h>
//#include <strsafe.h>

#include "resource.h"

#include <CommCtrl.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include "StrUtils.h"
#include "FindDlgChain.h"

#include <nlohmann/json.hpp>

using namespace nlohmann;

template <class T>
decltype(auto) unconst(T& v)
{
    return const_cast<std::remove_const_t<T>&>(v);
}

template <class T>
decltype(auto) unconst(T* v)
{
    return const_cast<std::remove_const_t<T>*>(v);
}

extern HINSTANCE g_hInstance;
extern HACCEL g_hAccelTable;
extern HWND g_hWndAccel;

HTREEITEM TreeView_FindItem(HWND hTreeCtrl, HTREEITEM hItem, LPFINDREPLACE pfr)
{
    StrCompareT comp = GeStrComparator((pfr->Flags & FR_MATCHCASE) == FR_MATCHCASE, (pfr->Flags & FR_WHOLEWORD) == FR_WHOLEWORD);
    if ((pfr->Flags & FR_DOWN) == FR_DOWN)
    {
        while (hItem = TreeView_GetNextDepthFirst(hTreeCtrl, hItem))
        {
            TCHAR strItem[1024] = TEXT("");
            TV_ITEM tvi = {};
            tvi.hItem = hItem;
            tvi.mask = TVIF_TEXT;
            tvi.pszText = strItem;
            tvi.cchTextMax = ARRAYSIZE(strItem);
            TreeView_GetItem(hTreeCtrl, &tvi);
            if (comp(tvi.pszText, pfr->lpstrFindWhat))
                return hItem;
        }
    }
    else
    {
        while (hItem = TreeView_GetPrevDepthFirst(hTreeCtrl, hItem))
        {
            TCHAR strItem[1024] = TEXT("");
            TV_ITEM tvi = {};
            tvi.hItem = hItem;
            tvi.mask = TVIF_TEXT;
            tvi.pszText = strItem;
            tvi.cchTextMax = ARRAYSIZE(strItem);
            TreeView_GetItem(hTreeCtrl, &tvi);
            if (comp(tvi.pszText, pfr->lpstrFindWhat))
                return hItem;
        }
    }
    return NULL;
}

void ImportJson(HWND hTree, HTREEITEM hParent, const ordered_json& j, const std::vector<std::string>& values)
{
    for (const auto& i : j.items())
    {
        std::stringstream s;
        s << i.key();
        if (i.value().is_structured())
        {
            if (i.value().is_array())
                s << " [" << i.value().size() << "]";
            else if (!values.empty())
            {
                s << " { ";
                for (const auto& vn : values)
                {
                    if (i.value().contains(vn))
                        s << vn << ": " << i.value()[vn] << " ";
                }
                s << "}";
            }
        }
        else
        {
            s << ": ";
            s << i.value();
        }

        const std::wstring ws = a2w(s.str());

        TVINSERTSTRUCT tvis = {};
        tvis.hParent = hParent;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT;
        tvis.item.pszText = unconst(ws.c_str());
        const HTREEITEM hNode = TreeView_InsertItem(hTree, &tvis);

        if (i.value().is_structured())
            ImportJson(hTree, hNode, i.value(), values);
    }
}

class RootWindow : public Window
{
    friend WindowManager<RootWindow>;
public:
    static ATOM Register() { return WindowManager<RootWindow>::Register(); }
    static RootWindow* Create() { return WindowManager<RootWindow>::Create(); }

    void Import(LPCTSTR lpFilename, const std::vector<std::string>& values);
    void Import(std::istream& f, const std::vector<std::string>& values)
    {
        m_json = ordered_json::parse(f);
        ImportJson(m_hTreeCtrl, TVI_ROOT, m_json, values);
    }

    void DisplayError(LPCSTR e)
    {
        MessageBoxA(*this, e, "Json Viewer", MB_ICONERROR | MB_OK);
    }
    void DisplayError(LPCWSTR e)
    {
        MessageBoxW(*this, e, L"Json Viewer", MB_ICONERROR | MB_OK);
    }

protected:
    static void GetCreateWindow(CREATESTRUCT& cs);
    static void GetWndClass(WNDCLASS& wc);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSize(UINT state, int cx, int cy);
    void OnSetFocus(HWND hwndOldFocus);
    void OnCommand(int id, HWND hWndCtl, UINT codeNotify);
    void OnActivate(UINT state, HWND hWndActDeact, BOOL fMinimized);

    static LPCTSTR ClassName() { return TEXT("JsonViewer"); }

    HWND m_hTreeCtrl = NULL;
    ordered_json m_json;

    FindDlgChain m_FindDlgChain;
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Window::GetCreateWindow(cs);
    cs.lpszName = TEXT("Json Viewer");
    cs.style = WS_OVERLAPPEDWINDOW;
    cs.hMenu = LoadMenu(cs.hInstance, MAKEINTRESOURCE(IDR_MENU1));
    cs.cx = 600;
    cs.cy = 800;
}

void RootWindow::GetWndClass(WNDCLASS& wc)
{
    Window::GetWndClass(wc);
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON1));
}

BOOL RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct)
{
    m_hTreeCtrl = TreeView_Create(*this, RECT(), WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT, 0);
    m_FindDlgChain.Init(*this);
    return TRUE;
}

void RootWindow::OnDestroy()
{
    PostQuitMessage(0);
}

void RootWindow::OnSize(const UINT state, const int cx, const int cy)
{
    if (m_hTreeCtrl)
    {
        SetWindowPos(m_hTreeCtrl, NULL, 0, 0,
            cx, cy,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

void RootWindow::OnSetFocus(const HWND hwndOldFocus)
{
    if (m_hTreeCtrl)
        SetFocus(m_hTreeCtrl);
}

void RootWindow::OnCommand(int id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_FILE_OPEN:
    {
        TCHAR strFilename[MAX_PATH] = TEXT("");
        OPENFILENAME ofn = { sizeof(OPENFILENAME) };
        ofn.hwndOwner = *this;
        ofn.lpstrFilter = TEXT("Json Files\0*.json\0Text Files\0*.txt\0All Files\0*.*\0");
        ofn.Flags = OFN_FILEMUSTEXIST;
        ofn.lpstrFile = strFilename;
        ofn.nMaxFile = ARRAYSIZE(strFilename);
        if (GetOpenFileName(&ofn))
        {
            HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            TreeView_DeleteAllItems(m_hTreeCtrl);
            Import(strFilename, std::vector<std::string>());
            SetCursor(hOldCursor);
        }
        break;
    }

    case ID_FILE_EXIT:
        SendMessage(*this, WM_CLOSE, 0, 0);
        break;

    case ID_EDIT_EXPANDALL:
    {
        const HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        SetWindowRedraw(m_hTreeCtrl, FALSE);
        TreeView_ExpandAll(m_hTreeCtrl, TVE_EXPAND);
        SetWindowRedraw(m_hTreeCtrl, TRUE);
        SetCursor(hOldCursor);
        break;
    }

    case ID_EDIT_COLLAPSEALL:
    {
        const HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
        SetWindowRedraw(m_hTreeCtrl, FALSE);
        TreeView_ExpandAll(m_hTreeCtrl, TVE_COLLAPSE);
        SetWindowRedraw(m_hTreeCtrl, TRUE);
        SetCursor(hOldCursor);
        break;
    }

    case ID_EDIT_FIND:
        m_FindDlgChain.Show();
        break;

    case ID_HELP_ABOUT:
        AboutDlg::DoModal(*this);
        break;
    }
}

LRESULT RootWindow::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    LRESULT ret = 0;
    switch (uMsg)
    {
        HANDLE_MSG(WM_CREATE, OnCreate);
        HANDLE_MSG(WM_DESTROY, OnDestroy);
        HANDLE_MSG(WM_SIZE, OnSize);
        HANDLE_MSG(WM_SETFOCUS, OnSetFocus);
        HANDLE_MSG(WM_COMMAND, OnCommand);
        HANDLE_MSG(WM_ACTIVATE, OnActivate);
    default:
        if (uMsg == WM_FINDSTRING)
        {
            LPFINDREPLACE pfr = (LPFINDREPLACE) lParam;
            if ((pfr->Flags & FR_FINDNEXT) == FR_FINDNEXT)
            {
                SetHandled(true);
                HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                HTREEITEM hItem = TreeView_GetSelection(m_hTreeCtrl);
                HTREEITEM hSearchItem = TreeView_FindItem(m_hTreeCtrl, hItem, pfr);
                SetCursor(hOldCursor);
                if (hSearchItem)
                    TreeView_Select(m_hTreeCtrl, hSearchItem, TVGN_CARET);
                else
                    m_FindDlgChain.DisplayError(Format(TEXT("Unable to find: %s"), pfr->lpstrFindWhat).c_str(), TEXT("Json Viewer"));
            }
        }
        break;
    }

    if (!IsHandled())
    {
        bool bHandled = false;
        ret = m_FindDlgChain.ProcessMessage(*this, uMsg, wParam, lParam, bHandled);
        if (bHandled)
            SetHandled(true);
    }
    if (!IsHandled())
        ret = Window::HandleMessage(uMsg, wParam, lParam);

    return ret;
}

void RootWindow::OnActivate(UINT state, HWND hWndActDeact, BOOL fMinimized)
{
    if (state == WA_INACTIVE)
    {
        g_hWndAccel = NULL;
        g_hAccelTable = NULL;
    }
    else
    {
        g_hWndAccel = *this;
        g_hAccelTable = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
    }
}

void RootWindow::Import(LPCTSTR lpFilename, const std::vector<std::string>& values)
try
{
    if (lpFilename == nullptr || wcscmp(lpFilename, TEXT("-")) == 0)
    {
        SetWindowText(*this, TEXT("Json Viewer - stdin"));
        Import(std::cin, values);
    }
    else
    {
        SetWindowText(*this, Format(TEXT("Json Viewer - %s"), PathFindFileName(lpFilename)).c_str());
        std::ifstream f(lpFilename);
        //f.exceptions(f.exceptions() | std::ifstream::failbit);
        if (f)
            Import(f, values);
        else
        {
            SetWindowText(*this, TEXT("Json Viewer"));
            throw std::system_error(errno, std::iostream_category(), w2a(lpFilename).c_str());
        }
    }
}
catch (const std::exception& e)
{
    DisplayError(e.what());
}

bool Run(_In_ const LPCTSTR lpCmdLine, _In_ const int nShowCmd)
{
    LPCTSTR lpFilename = nullptr;
    std::vector<std::string> values;

    RadLogInitWnd(NULL, "Json Viewer", TEXT("Json Viewer"));

    CHECK_LE_RET(RootWindow::Register(), false);

    RootWindow* prw = RootWindow::Create();
    CHECK_LE_RET(prw != nullptr, false);

    RadLogInitWnd(*prw, "Json Viewer", TEXT("Json Viewer"));
    ShowWindow(*prw, nShowCmd);

    for (int argi = 1; argi < __argc; ++argi)
    {
        LPCTSTR arg = __targv[argi];
        if (_tcsicmp(arg, TEXT("/v")) == 0)
            values.push_back(w2a(__targv[++argi]).c_str());
        else if (lpFilename == nullptr)
            lpFilename = arg;
        else
            prw->DisplayError(Format(TEXT("Unknown argument: %s"), arg).c_str());
    }

    if (lpFilename)
        prw->Import(lpFilename, values);

    return true;
}

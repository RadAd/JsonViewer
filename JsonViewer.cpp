#include "Rad/Window.h"

#include <shellapi.h>

#include "Rad/Dialog.h"
#include "Rad/Windowxx.h"
#include "Rad/Log.h"
#include "Rad/Format.h"
#include "Rad/TreeViewPlus.h"
#include "Rad/MemoryPlus.h"
#include "Rad/AboutDlg.h"
#include <tchar.h>
//#include <strsafe.h>

#include "resource.h"

#include <CommCtrl.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <fstream>
#include <sstream>
#include <iostream>

#include "StrUtils.h"
#include "FindDlgChain.h"
#include "ShowMenuShortcutChain.h"
#include "ValuesDlg.h"

#include <nlohmann/json.hpp>

inline BOOL IsFlagSet(const DWORD f, const DWORD t)
{
    return (f & t) == t;
}
inline BOOL IsFlagSet(const UINT f, const UINT t)
{
    return (f & t) == t;
}

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

inline BOOL OpenClipboardRetry(HWND hWndNewOwner)
{
    BOOL r;
    while ((r = OpenClipboard(hWndNewOwner)) == FALSE)
    {
        if (GetLastError() != ERROR_ACCESS_DENIED)
            break;
        Sleep(100);
    }
    return r;
}

extern HINSTANCE g_hInstance;
extern HACCEL g_hAccelTable;
extern HWND g_hWndAccel;

BOOL CALLBACK TreeView_CompareItemFindReplaceOld(HWND hTreeCtrl, HTREEITEM hItem, LPARAM lParamData)
{
    LPFINDREPLACE pfr = reinterpret_cast<LPFINDREPLACE>(lParamData);

    StrCompareT comp = GeStrComparator(IsFlagSet(pfr->Flags, FR_MATCHCASE), IsFlagSet(pfr->Flags, FR_WHOLEWORD));

    TCHAR strItem[1024] = TEXT("");
    TreeView_GetText(hTreeCtrl, hItem, strItem, ARRAYSIZE(strItem));
    return comp(strItem, pfr->lpstrFindWhat);
}

struct TVCompareItemData
{
    StrCompareT pCompare;
    LPCTSTR lpstrFindWhat;
};

BOOL CALLBACK TreeView_CompareItemFindReplace(HWND hTreeCtrl, HTREEITEM hItem, LPARAM lParamData)
{
    const TVCompareItemData* pcid = reinterpret_cast<TVCompareItemData*>(lParamData);

    TCHAR strItem[1024] = TEXT("");
    TreeView_GetText(hTreeCtrl, hItem, strItem, ARRAYSIZE(strItem));
    return pcid->pCompare(strItem, pcid->lpstrFindWhat);
}

void FormatJsonValue(std::ostream& s, const ordered_json& j, const std::vector<std::string>& values)
{
    if (j.is_structured())
    {
        if (j.is_array())
            s << "[" << j.size() << "]";
        else if (!values.empty())
        {
            s << "{ ";
            for (const auto& vn : values)
            {
                if (j.contains(vn))
                {
                    s << vn << ": ";
                    FormatJsonValue(s, j[vn], values);
                    s << " ";
                }
            }
            s << "}";
        }
    }
    else
    {
        s << j;
    }
}

std::string FormatJson(const std::string& key, const ordered_json& j, const std::vector<std::string>& values)
{
    std::stringstream s;
    s << key << ": ";
    FormatJsonValue(s, j, values);
    return s.str();
}

void ImportJson(HWND hTree, HTREEITEM hParent, const ordered_json& j, const std::vector<std::string>& values)
{
    for (const auto& i : j.items())
    {
        const std::tstring ws = s2t(FormatJson(i.key(), i.value(), values));

        TVINSERTSTRUCT tvis = {};
        tvis.hParent = hParent;
        tvis.hInsertAfter = TVI_LAST;
        tvis.item.mask = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
        tvis.item.pszText = unconst(ws.c_str());
        tvis.item.cChildren = i.value().is_structured() ? static_cast<int>(i.value().size()) : 0;
        const ordered_json* pv = &i.value();
        tvis.item.lParam = reinterpret_cast<LPARAM>(pv);
        const HTREEITEM hNode = TreeView_InsertItem(hTree, &tvis);

        //if (i.value().is_structured())
            //ImportJson(hTree, hNode, i.value(), values);
    }
}

#define ID_TREE (101)

class RootWindow : public Window
{
    friend WindowManager<RootWindow>;
public:
    static ATOM Register() { return WindowManager<RootWindow>::Register(); }
    static RootWindow* Create() { return WindowManager<RootWindow>::Create(); }

    void SetValues(const std::vector<std::string>& values) { m_values = values; }
    void Import(LPCTSTR lpFilename);
    void SaveAs(LPCTSTR lpFilename);
    void Import(std::istream& f)
    {
        m_json = ordered_json::parse(f);
        FillTree();
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

    void FillTree();

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSize(UINT state, int cx, int cy);
    void OnSetFocus(HWND hwndOldFocus);
    void OnCommand(int id, HWND hWndCtl, UINT codeNotify);
    void OnActivate(UINT state, HWND hWndActDeact, BOOL fMinimized);
    void OnDropFiles(HDROP hDrop);
    LRESULT OnNotify(DWORD dwID, LPNMHDR pNmHdr);

    static LPCTSTR ClassName() { return TEXT("JsonViewer"); }

    HWND m_hTreeCtrl = NULL;
    std::tstring m_filename;
    ordered_json m_json;
    std::vector<std::string> m_values;

    FindDlgChain m_FindDlgChain;
    ShowMenuShortcutChain m_ShowMenuShortcutChain;
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Window::GetCreateWindow(cs);
    cs.lpszName = TEXT("Json Viewer");
    cs.style = WS_OVERLAPPEDWINDOW;
    cs.dwExStyle = WS_EX_ACCEPTFILES;
    cs.hMenu = LoadMenu(cs.hInstance, MAKEINTRESOURCE(IDR_MENU1));
    cs.cx = 600;
    cs.cy = 800;
}

void RootWindow::GetWndClass(WNDCLASS& wc)
{
    Window::GetWndClass(wc);
    wc.hIcon = LoadIcon(wc.hInstance, MAKEINTRESOURCE(IDI_ICON2));
}

BOOL RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct)
{
    m_hTreeCtrl = TreeView_Create(*this, RECT(), WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_EDITLABELS | TVS_DISABLEDRAGDROP | TVS_SHOWSELALWAYS | TVS_FULLROWSELECT, ID_TREE);
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

LPCTSTR g_Filter = TEXT(
    "Json Files (*.json)\0*.json\0"
    "Text Files (*.txt)\0*.txt\0"
    "All Files (*.*)\0*.*\0");

void RootWindow::OnCommand(int id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
    case ID_FILE_OPEN:
    {
        TCHAR strFilename[MAX_PATH] = TEXT("");
        OPENFILENAME ofn = { sizeof(OPENFILENAME) };
        ofn.hwndOwner = *this;
        ofn.lpstrFilter = g_Filter;
        ofn.Flags = OFN_FILEMUSTEXIST;
        ofn.lpstrFile = strFilename;
        ofn.nMaxFile = ARRAYSIZE(strFilename);
        if (GetOpenFileName(&ofn))
            Import(strFilename);
        break;
    }

    case ID_FILE_SAVE:
        SaveAs(m_filename.c_str());
        break;

    case ID_FILE_SAVEAS:
    {
        TCHAR strFilename[MAX_PATH] = TEXT("");
        OPENFILENAME ofn = { sizeof(OPENFILENAME) };
        ofn.hwndOwner = *this;
        ofn.lpstrFilter = g_Filter;
        ofn.Flags = OFN_CREATEPROMPT;
        ofn.lpstrFile = strFilename;
        ofn.nMaxFile = ARRAYSIZE(strFilename);
        if (GetSaveFileName(&ofn))
        {
            if (ofn.nFileExtension == 0)
            {
                switch (ofn.nFilterIndex)
                {
                case 1: _tcscat_s(strFilename, TEXT(".json")); break;
                case 2: _tcscat_s(strFilename, TEXT(".txt")); break;
                }
            }
            SaveAs(strFilename);
        }
        break;
    }

    case ID_FILE_EXIT:
        SendMessage(*this, WM_CLOSE, 0, 0);
        break;

    case ID_EDIT_COPY:
    {
        if (OpenClipboardRetry(*this))
        {
            EmptyClipboard();
            const std::string json = m_json.dump(2);
            HGLOBAL hClip = GlobalAlloc(GMEM_MOVEABLE, json.size() + 1);
            if (hClip)
            {
                {
                    auto buffer = AutoGlobalLock<PSTR>(hClip);
                    strcpy_s(buffer.get(), json.size() + 1, json.c_str());
                }
                SetClipboardData(CF_TEXT, hClip);
            }
            CloseClipboard();
        }
        else
            DisplayError(Format(TEXT("Error OpenClipboard: %d\n"), GetLastError()).c_str());
        break;
    }

    case ID_EDIT_PASTE:
    {
        if (OpenClipboardRetry(*this))
        {
            HGLOBAL hClip = GetClipboardData(CF_TEXT);
            if (hClip)
            {
                auto buffer = AutoGlobalLock<PCSTR>(hClip);

                try
                {
                    SetWindowText(*this, TEXT("Json Viewer - clipboard"));
                    m_json = ordered_json::parse(buffer.get());
                    m_filename.clear();
                    FillTree();
                }
                catch (const std::exception& e)
                {
                    SetWindowText(*this, m_filename.empty() ? TEXT("Json Viewer") : Format(TEXT("Json Viewer - %s"), PathFindFileName(m_filename.c_str())).c_str());
                    DisplayError(e.what());
                }
            }
            else
                DisplayError(TEXT("Invalid clipbaord format\n"));
            CloseClipboard();
        }
        else
            DisplayError(Format(TEXT("Error OpenClipboard: %d\n"), GetLastError()).c_str());
        break;
    }

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

    case ID_EDIT_VALUES:
    {
        ValuesDlg vdlg;
        vdlg.m_values = m_values;
        if (vdlg.DoModal(*this) == IDOK)
        {
            m_values = vdlg.m_values;

#if 1
            FillTree();
#else
            // TODO Refresh view
            HTREEITEM hItem = TVI_ROOT;
            while (hItem = TreeView_GetNextDepthFirst(m_hTreeCtrl, hItem, FALSE))
            {
                TV_ITEM tvi = {};
                tvi.hItem = hItem;
                tvi.mask = TVIF_PARAM;
                TreeView_GetItem(m_hTreeCtrl, &tvi);

                const std::string key = "key";  // TODO How to get key?
                const ordered_json& j = *reinterpret_cast<const ordered_json*>(tvi.lParam);
                FormatJson(key, j, m_values);
            }
#endif
        }
        break;
    }

    case ID_EDIT_EDITVALUE:
    {
        HTREEITEM hItem = TreeView_GetSelection(m_hTreeCtrl);
        TreeView_EditLabel(m_hTreeCtrl, hItem);
        break;
    }

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
        HANDLE_MSG(WM_DROPFILES, OnDropFiles);
        HANDLE_MSG(WM_NOTIFY, OnNotify);
    default:
        if (uMsg == WM_FINDSTRING)
        {
            LPFINDREPLACE pfr = (LPFINDREPLACE) lParam;
            if (IsFlagSet(pfr->Flags, FR_FINDNEXT))
            {
                SetHandled(true);
                HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                HTREEITEM hItem = TreeView_GetSelection(m_hTreeCtrl);
                //HTREEITEM hSearchItem = TreeView_FindItem(m_hTreeCtrl, hItem, IsFlagSet(pfr->Flags, FR_DOWN), TreeView_CompareItemFindReplaceOld, reinterpret_cast<LPARAM>(pfr));
                TVCompareItemData cid =
                {
                    GeStrComparator(IsFlagSet(pfr->Flags, FR_MATCHCASE), IsFlagSet(pfr->Flags, FR_WHOLEWORD)),
                    pfr->lpstrFindWhat,
                };
                HTREEITEM hSearchItem = TreeView_FindItem(m_hTreeCtrl, hItem, IsFlagSet(pfr->Flags, FR_DOWN), TreeView_CompareItemFindReplace, reinterpret_cast<LPARAM>(&cid), TRUE);
                SetCursor(hOldCursor);
                if (hSearchItem)
                    TreeView_Select(m_hTreeCtrl, hSearchItem, TVGN_CARET);
                else
                    m_FindDlgChain.DisplayError(Format(TEXT("Unable to find: %s"), pfr->lpstrFindWhat).c_str(), TEXT("Json Viewer"));
            }
        }
        break;
    }

    MessageChain* Chains[] = { &m_FindDlgChain, &m_ShowMenuShortcutChain };
    for (MessageChain* pChain : Chains)
    {
        if (IsHandled())
            break;

        bool bHandled = false;
        ret = pChain->ProcessMessage(*this, uMsg, wParam, lParam, bHandled);
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
    m_ShowMenuShortcutChain.Init(g_hAccelTable);
}

void RootWindow::OnDropFiles(HDROP hDrop)
{
    TCHAR szName[MAX_PATH];
    DragQueryFile(hDrop, 0, szName, MAX_PATH);
    DragFinish(hDrop);

    Import(szName);
}

LRESULT RootWindow::OnNotify(DWORD dwID, LPNMHDR pNmHdr)
{
    if (dwID == ID_TREE)
    {
        static std::tstring keyedit;
        switch (pNmHdr->code)
        {
        case TVN_ITEMEXPANDING:
        {
            LPNMTREEVIEW pnmtv = (LPNMTREEVIEW)pNmHdr;
            if (pnmtv->action == TVE_EXPAND && !IsFlagSet(pnmtv->itemNew.state, TVIS_EXPANDEDONCE))
            {
                const ordered_json* pv = reinterpret_cast<const ordered_json*>(pnmtv->itemNew.lParam);
                ImportJson(m_hTreeCtrl, pnmtv->itemNew.hItem, *pv, m_values);
            }
            break;
        }

        case TVN_BEGINLABELEDIT:
        {
            LPNMTVDISPINFO pnmtv = (LPNMTVDISPINFO)pNmHdr;
            LPCTSTR value = _tcschr(pnmtv->item.pszText, TEXT(':'));
            if (!value)
                return TRUE;
            keyedit = std::tstring(const_cast<LPCTSTR>(pnmtv->item.pszText), value);
            const ordered_json* pv = reinterpret_cast<const ordered_json*>(pnmtv->item.lParam);
            const bool allow = !pv->is_structured();
            if (allow)
                g_hWndAccel = NULL;
            return !allow;
            break;
        }

        case TVN_ENDLABELEDIT:
        try
        {
            g_hWndAccel = *this;
            LPNMTVDISPINFO pnmtv = (LPNMTVDISPINFO)pNmHdr;
            if (pnmtv->item.pszText)
            {
                // TODO Allow to edit key name
                ordered_json* pv = reinterpret_cast<ordered_json*>(pnmtv->item.lParam);
                const LPCTSTR value = _tcschr(pnmtv->item.pszText, TEXT(':'));
                if (!value)
                    throw std::exception("Unable to find value.");
                if (keyedit != std::tstring(const_cast<LPCTSTR>(pnmtv->item.pszText), value))
                    throw std::exception("Editing key name is not yet supported.");

                *pv = ordered_json::parse(w2a(value + 1));
                _tcscpy_s(pnmtv->item.pszText, pnmtv->item.cchTextMax, s2t(FormatJson(w2a(keyedit), *pv, m_values)).c_str());

                HTREEITEM hParent = TreeView_GetParent(m_hTreeCtrl, pnmtv->item.hItem);
                if (hParent && std::find(m_values.begin(), m_values.end(), w2a(keyedit)) != m_values.end())
                {
                    TCHAR Label[MAX_PATH] = TEXT("");
                    TV_ITEM tvi = {};
                    tvi.hItem = hParent;
                    tvi.mask = TVIF_TEXT | TVIF_PARAM;
                    tvi.pszText = Label;
                    tvi.cchTextMax = ARRAYSIZE(Label);
                    TreeView_GetItem(m_hTreeCtrl, &tvi);
                    const ordered_json* parentpv = reinterpret_cast<ordered_json*>(tvi.lParam);
                    const LPTSTR value = _tcschr(Label, TEXT('{'));
                    if (value && parentpv)
                    {
                        *(value - 1) = TEXT('\0');
                        TreeView_SetText(m_hTreeCtrl, hParent, s2t(FormatJson(w2a(Label), *parentpv, m_values)).c_str());
                    }
                }
            }
            return TRUE;
        }
        catch (const std::exception& e)
        {
            DisplayError(e.what());
            return FALSE;
        }
        }
    }
    return 0;
}

void RootWindow::FillTree()
{
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    TreeView_DeleteAllItems(m_hTreeCtrl);
    ImportJson(m_hTreeCtrl, TVI_ROOT, m_json, m_values);
    SetCursor(hOldCursor);
}

void RootWindow::Import(LPCTSTR lpFilename)
try
{
    if (lpFilename == nullptr || wcscmp(lpFilename, TEXT("-")) == 0)
    {
        SetWindowText(*this, TEXT("Json Viewer - stdin"));
        Import(std::cin);
    }
    else
    {
        SetWindowText(*this, Format(TEXT("Json Viewer - %s"), PathFindFileName(lpFilename)).c_str());
        std::ifstream f(lpFilename);
        //f.exceptions(f.exceptions() | std::ifstream::failbit);
        if (!f)
            throw std::system_error(errno, std::iostream_category(), w2a(lpFilename).c_str());

        Import(f);
        m_filename = lpFilename;
    }
}
catch (const std::exception& e)
{
    SetWindowText(*this, m_filename.empty() ? TEXT("Json Viewer") : Format(TEXT("Json Viewer - %s"), PathFindFileName(m_filename.c_str())).c_str());
    DisplayError(e.what());
}

void RootWindow::SaveAs(LPCTSTR lpFilename)
try
{
    SetWindowText(*this, Format(TEXT("Json Viewer - %s"), PathFindFileName(lpFilename)).c_str());
    std::ofstream f(lpFilename);
    //f.exceptions(f.exceptions() | std::ifstream::failbit);
    if (!f)
        throw std::system_error(errno, std::iostream_category(), w2a(lpFilename).c_str());

    f << std::setw(2) << m_json;
    m_filename = lpFilename;
}
catch (const std::exception& e)
{
    SetWindowText(*this, m_filename.empty() ? TEXT("Json Viewer") : Format(TEXT("Json Viewer - %s"), PathFindFileName(m_filename.c_str())).c_str());
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

    prw->SetValues(values);
    if (lpFilename)
        prw->Import(lpFilename);

    return true;
}

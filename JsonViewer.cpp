#include "Rad/Window.h"
#include "Rad/Dialog.h"
#include "Rad/Windowxx.h"
#include "Rad/Log.h"
#include "Rad/Format.h"
#include <tchar.h>
//#include <strsafe.h>
//#include "resource.h"

#include <CommCtrl.h>
#include <shlwapi.h>
#include <fstream>
#include <sstream>
#include <iostream>

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
            else
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

    void DisplayError(const std::string& e)
    {
        MessageBoxA(NULL, e.c_str(), "Json Viewer", MB_ICONERROR | MB_OK);
    }
    void DisplayError(const std::wstring& e)
    {
        MessageBoxW(NULL, e.c_str(), L"Json Viewer", MB_ICONERROR | MB_OK);
    }

protected:
    static void GetCreateWindow(CREATESTRUCT& cs);
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    BOOL OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();
    void OnSize(UINT state, int cx, int cy);
    void OnSetFocus(HWND hwndOldFocus);

    static LPCTSTR ClassName() { return TEXT("JsonViewer"); }

    HWND m_hTreeCtrl = NULL;
    ordered_json m_json;
};

void RootWindow::GetCreateWindow(CREATESTRUCT& cs)
{
    Window::GetCreateWindow(cs);
    cs.lpszName = TEXT("Json Viewer");
    cs.style = WS_OVERLAPPEDWINDOW;
    cs.cx = 400;
    cs.cy = 400;
}

BOOL RootWindow::OnCreate(const LPCREATESTRUCT lpCreateStruct)
{
    m_hTreeCtrl = CreateWindow(WC_TREEVIEW, nullptr, WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_DISABLEDRAGDROP | TVS_FULLROWSELECT, 0, 0, 0, 0, *this, NULL, NULL, 0);

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

LRESULT RootWindow::HandleMessage(const UINT uMsg, const WPARAM wParam, const LPARAM lParam)
{
    LRESULT ret = 0;
    switch (uMsg)
    {
        HANDLE_MSG(WM_CREATE, OnCreate);
        HANDLE_MSG(WM_DESTROY, OnDestroy);
        HANDLE_MSG(WM_SIZE, OnSize);
        HANDLE_MSG(WM_SETFOCUS, OnSetFocus);
    }

    if (!IsHandled())
        ret = Window::HandleMessage(uMsg, wParam, lParam);

    return ret;
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
            char errmsg[1024];
            strerror_s(errmsg, errno);
            MessageBox(*this, Format(TEXT("Error opening file: %s\n%S"), lpFilename, errmsg).c_str(), TEXT("Json Viewer"), MB_ICONERROR | MB_OK);
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
        if (_wcsicmp(arg, TEXT("/v")) == 0)
            values.push_back(w2a(__targv[++argi]).c_str());
        else if (lpFilename == nullptr)
            lpFilename = arg;
        else
            prw->DisplayError(Format(TEXT("Unknown argument: %s"), arg));
    }
    if (__argc > 1)
        lpFilename = __targv[1];

    prw->Import(lpFilename, values);

    return true;
}

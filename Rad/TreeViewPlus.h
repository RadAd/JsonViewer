#pragma once
#include <Windows.h>
#include <CommCtrl.h>

#ifdef __cplusplus
extern "C"
{
#endif

inline HWND TreeView_Create(HWND hParent, RECT rc, DWORD dwStyle, int id)
{
    return CreateWindow(
        WC_TREEVIEW,
        NULL,
        dwStyle,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hParent,
        (HMENU)(INT_PTR)id,
        NULL,
        NULL);
}

inline BOOL TreeView_GetText(HWND hTreeCtrl, HTREEITEM hItem, LPTSTR pszText, int cchTextMax)
{
    TV_ITEM tvi = {};
    tvi.hItem = hItem;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = pszText;
    tvi.cchTextMax = cchTextMax;
    return TreeView_GetItem(hTreeCtrl, &tvi);
}

inline BOOL TreeView_SetText(HWND hTreeCtrl, HTREEITEM hItem, LPCTSTR pszText)
{
    TV_ITEM tvi = {};
    tvi.hItem = hItem;
    tvi.mask = TVIF_TEXT;
    tvi.pszText = const_cast<LPTSTR>(pszText);
    return TreeView_GetItem(hTreeCtrl, &tvi);
}

inline void TreeView_EnsureChildrenInserted(HWND hTreeCtrl, HTREEITEM hItem)
{
    if (hItem == NULL || hItem == TVI_ROOT)
        return;

    TV_ITEM tvi = {};
    tvi.hItem = hItem;
    tvi.mask = TVIF_STATE | TVIF_CHILDREN;
#if 0
    TCHAR strItem[1024] = TEXT("");
    tvi.mask |= TVIF_TEXT;
    tvi.pszText = strItem;
    tvi.cchTextMax = ARRAYSIZE(strItem);
#endif
    TreeView_GetItem(hTreeCtrl, &tvi);
    if ((tvi.state & TVIS_EXPANDEDONCE) == 0 && tvi.cChildren > 0)
    {
        NMTREEVIEW tv = {};
        tv.hdr.hwndFrom = hTreeCtrl;
        tv.hdr.idFrom = GetDlgCtrlID(hTreeCtrl);
        tv.hdr.code = TVN_ITEMEXPANDING;
        tv.action = TVE_EXPAND;
        tv.itemNew = tvi;
        SendMessage(GetParent(hTreeCtrl), WM_NOTIFY, tv.hdr.idFrom, reinterpret_cast<LPARAM>(&tv));

        TreeView_SetItemState(hTreeCtrl, hItem, TVIS_EXPANDEDONCE, TVIS_EXPANDEDONCE);
    }
}

inline HTREEITEM TreeView_GetLastSibling(HWND hTreeCtrl, HTREEITEM hItem)
{
    _ASSERT(hTreeCtrl);
    _ASSERT(hItem);

    HTREEITEM hNextItem = NULL;
    while (hNextItem = TreeView_GetNextSibling(hTreeCtrl, hItem))
        hItem = hNextItem;

    return hItem;
}

inline HTREEITEM TreeView_GetLastChild(HWND hTreeCtrl, HTREEITEM hItem, BOOL bEnsureChildren)
{
    _ASSERT(hTreeCtrl);
    _ASSERT(hItem);

    if (bEnsureChildren)
        TreeView_EnsureChildrenInserted(hTreeCtrl, hItem);

    HTREEITEM hChild = TreeView_GetChild(hTreeCtrl, hItem);
    if (!hChild)
        return NULL;

    return TreeView_GetLastSibling(hTreeCtrl, hChild);
}

inline HTREEITEM TreeView_GetLastChildRecursive(HWND hTreeCtrl, HTREEITEM hItem, BOOL bEnsureChildren)
{
    _ASSERT(hTreeCtrl);
    _ASSERT(hItem);

    if (bEnsureChildren)
        TreeView_EnsureChildrenInserted(hTreeCtrl, hItem);

    HTREEITEM hChild = TreeView_GetChild(hTreeCtrl, hItem);
    if (!hChild)
        return NULL;

    HTREEITEM hNextItem = NULL;
    while (hNextItem = TreeView_GetLastChild(hTreeCtrl, hItem, bEnsureChildren))
        hItem = hNextItem;

    return hItem;
}

inline HTREEITEM TreeView_GetNextDepthFirst(HWND hTreeCtrl, HTREEITEM hItem, BOOL bEnsureChildren)
{
    if (hItem == NULL || hItem == TVI_ROOT)
        return TreeView_GetChild(hTreeCtrl, TVI_ROOT);

    HTREEITEM hNextItem = NULL;

    if (bEnsureChildren)
        TreeView_EnsureChildrenInserted(hTreeCtrl, hItem);

    hNextItem = TreeView_GetChild(hTreeCtrl, hItem);
    if (hNextItem)
        return hNextItem;

    hNextItem = TreeView_GetNextSibling(hTreeCtrl, hItem);
    if (hNextItem)
        return hNextItem;

    while (hNextItem == NULL && (hItem = TreeView_GetParent(hTreeCtrl, hItem)))
    {
        hNextItem = TreeView_GetNextSibling(hTreeCtrl, hItem);
        if (hNextItem)
            return hNextItem;
    }

    return NULL;
}

inline HTREEITEM TreeView_GetPrevDepthFirst(HWND hTreeCtrl, HTREEITEM hItem, BOOL bEnsureChildren)
{
    if (hItem == NULL || hItem == TVI_ROOT)
        return TreeView_GetLastChildRecursive(hTreeCtrl, TVI_ROOT, bEnsureChildren);

    HTREEITEM hPrevItem = TreeView_GetPrevSibling(hTreeCtrl, hItem);
    if (hPrevItem)
    {
        HTREEITEM hChild = TreeView_GetLastChildRecursive(hTreeCtrl, hPrevItem, bEnsureChildren);
        if (hChild)
            return hChild;
        return hPrevItem;
    }

    return TreeView_GetParent(hTreeCtrl, hItem);
}

inline void TreeView_ExpandAll(HWND hTreeCtrl, UINT code, HTREEITEM hItem = TVI_ROOT)
{
    while (hItem)
    {
        HTREEITEM hChildItem = TreeView_GetChild(hTreeCtrl, hItem);
        if (hChildItem)
        {
            TreeView_Expand(hTreeCtrl, hItem, code);
            TreeView_ExpandAll(hTreeCtrl, code, hChildItem);
        }

        hItem = hItem == TVI_ROOT ? NULL : TreeView_GetNextSibling(hTreeCtrl, hItem);
    }
}

typedef BOOL(CALLBACK* pTreeView_CompareItemFunc)(HWND hTreeCtrl, HTREEITEM hItem, LPARAM lParamData);

inline HTREEITEM TreeView_FindItem(HWND hTreeCtrl, HTREEITEM hItem, BOOL bDown, pTreeView_CompareItemFunc pCompare, LPARAM lParamData, BOOL bEnsureChildren)
{
    if (bDown)
    {
        while (hItem = TreeView_GetNextDepthFirst(hTreeCtrl, hItem, bEnsureChildren))
        {
            if (pCompare(hTreeCtrl, hItem, lParamData))
                return hItem;
        }
    }
    else
    {
        while (hItem = TreeView_GetPrevDepthFirst(hTreeCtrl, hItem, bEnsureChildren))
        {
            if (pCompare(hTreeCtrl, hItem, lParamData))
                return hItem;
        }
    }
    return NULL;
}

#ifdef __cplusplus
}

#endif

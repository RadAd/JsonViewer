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
        TEXT(""),
        dwStyle,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hParent,
        (HMENU)(INT_PTR)id,
        NULL,
        NULL);
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

inline HTREEITEM TreeView_GetLastChild(HWND hTreeCtrl, HTREEITEM hItem)
{
    _ASSERT(hTreeCtrl);
    _ASSERT(hItem);

    HTREEITEM hChild = TreeView_GetChild(hTreeCtrl, hItem);
    if (!hChild)
        return NULL;

    return TreeView_GetLastSibling(hTreeCtrl, hChild);
}

inline HTREEITEM TreeView_GetLastChildRecursive(HWND hTreeCtrl, HTREEITEM hItem)
{
    _ASSERT(hTreeCtrl);
    _ASSERT(hItem);

    HTREEITEM hChild = TreeView_GetChild(hTreeCtrl, hItem);
    if (!hChild)
        return NULL;

    HTREEITEM hNextItem = NULL;
    while (hNextItem = TreeView_GetLastChild(hTreeCtrl, hItem))
        hItem = hNextItem;

    return hItem;
}

inline HTREEITEM TreeView_GetNextDepthFirst(HWND hTreeCtrl, HTREEITEM hItem)
{
    if (hItem == NULL || hItem == TVI_ROOT)
        return TreeView_GetChild(hTreeCtrl, TVI_ROOT);

    HTREEITEM hNextItem = NULL;

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

inline HTREEITEM TreeView_GetPrevDepthFirst(HWND hTreeCtrl, HTREEITEM hItem)
{
    if (hItem == NULL || hItem == TVI_ROOT)
        return TreeView_GetLastChildRecursive(hTreeCtrl, TVI_ROOT);

    HTREEITEM hPrevItem = TreeView_GetPrevSibling(hTreeCtrl, hItem);
    if (hPrevItem)
    {
        HTREEITEM hChild = TreeView_GetLastChildRecursive(hTreeCtrl, hPrevItem);
        if (hChild)
            return hChild;
        return hPrevItem;
    }

    return TreeView_GetParent(hTreeCtrl, hItem);
}

#ifdef __cplusplus
}

#endif

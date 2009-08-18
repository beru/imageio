#include "StdAfx.h"
#include ".\filedialogex.h"

LRESULT CFileDialogEx::OnPostInit(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	SetListView(ODM_VIEW_DETAIL);
	return 0;
}

BOOL CFileDialogEx::SetListView(LISTVIEWCMD cmd)
{
	// Note real dialog is my parent, not me!
	HWND hWndShell = GetParent().GetDlgItem(lst2);
	ATLTRACE(_T("CMyOpenDlg::SetListView: hwnd=%p\n"), hWndShell);
	if (hWndShell) {
		// SHELLDLL_DefView window found: send it the command.
		::SendMessage(hWndShell, WM_COMMAND, cmd, NULL);
		return TRUE;
	}
	return FALSE;
}

LRESULT CFileDialogEx::OnInitDialog(HWND hwndFocus, LPARAM lParam)
{
// 速いPCだと縮小版でも別に苦にならないので詳細表示にする必要が無い…。
//	SetListView(ODM_VIEW_DETAIL); // this will fail
//	PostMessage(MYWM_POSTINIT);
	return TRUE; // set focus to default control
}

void CFileDialogEx::OnInitDone(LPOFNOTIFY lpon)
{
//	GetFileDialogWindow().CenterWindow();
}

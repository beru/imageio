#pragma once
#include <atlcrack.h>

class CFileDialogEx : public CFileDialogImpl<CFileDialogEx>
{
public:
	enum {
		MYWM_POSTINIT = WM_APP+1,
	};
	
	enum { ID_LISTVIEW = lst2 };
	// reverse-engineered command codes for SHELLDLL_DefView
	enum LISTVIEWCMD
	{	ODM_VIEW_ICONS = 0x7029,
		ODM_VIEW_LIST  = 0x702b,
		ODM_VIEW_DETAIL= 0x702c,
		ODM_VIEW_THUMBS= 0x702d,
		ODM_VIEW_TILES = 0x702e,
	};
	
	CFileDialogEx(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		HWND hWndParent = NULL)
		: CFileDialogImpl<CFileDialogEx>(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent)
	{ }

	// override base class map and references to handlers
	BEGIN_MSG_MAP(CFileDialogEx)
		MSG_WM_INITDIALOG(OnInitDialog)
		MESSAGE_HANDLER_EX(MYWM_POSTINIT, OnPostInit)
        CHAIN_MSG_MAP(CFileDialogImpl<CFileDialogEx>)
	END_MSG_MAP()

	LRESULT OnPostInit(UINT uMsg, WPARAM wParam, LPARAM lParam);
	
	BOOL SetListView(LISTVIEWCMD cmd);
	LRESULT OnInitDialog(HWND hwndFocus, LPARAM lParam);
	virtual void OnInitDone(LPOFNOTIFY lpon);
};

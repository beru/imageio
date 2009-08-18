#pragma once

#include "MultiPaneStatusBarEx.h"

class CImageView;

class CFileDialogEx;

class CMainFrame
	:
	public CFrameWindowImpl<CMainFrame>,
	public CUpdateUI<CMainFrame>,
	public CMessageFilter,
	public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)
	
	CMainFrame();
	~CMainFrame();
	
	boost::shared_ptr<CImageView> m_pImageView;
	
	CCommandBarCtrl m_CmdBar;
    CMPSBarWithProgress m_sBar;
	CFileDialogEx* m_pOpenFileDlg;
	CRecentDocumentList m_mru;
	
	void ReadFile(LPCTSTR filePath);
	void OnViewRender(size_t xRatioTarget, size_t xRatioSource, size_t yRatioTarget, size_t yRatioSource, DWORD elapsed);
	bool OnReadProgress(size_t lineStart, size_t lineEnd);


	void OnFilterSelect(enum ResamplingFilterType type);
	
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();
	
	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_RESAMPLINGFILTERS_NEARESTNEIGHBOR, UPDUI_MENUPOPUP)
//		UPDATE_ELEMENT(ID_RESAMPLINGFILTERS_BILINEAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_RESAMPLINGFILTERS_BOX, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_RESAMPLINGFILTERS_BOX_INTRINSICS, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_RESAMPLINGFILTERS_LANCZOS2SINC, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_RESAMPLINGFILTERS_LANCZOS3SINC, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_OPTIONS_PRESERVEASPECTRATIO, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
	END_UPDATE_UI_MAP()
	
	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MSG_WM_DROPFILES(OnDropFiles)
		MSG_WM_CLOSE(OnClose)
		MSG_WM_COMMAND(OnCommand)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER_EX(ID_FILE_OPEN, OnFileOpen)
		COMMAND_RANGE_HANDLER_EX(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, OnFileRecent)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()
	
// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileRecent(UINT uNotifyCode, int nID, HWND hWndCtl);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl);
	LRESULT OnDropFiles(HDROP hdrop);
	LRESULT OnClose(void);
	LRESULT OnCommand(UINT codeNotify, int id, HWND hwndCtl);
};

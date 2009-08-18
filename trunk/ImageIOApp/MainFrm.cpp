#include "StdAfx.h"

#include "resource.h"
#include ".\mainfrm.h"

#include "ImageView.h"

#include "AboutDlg.h"
#include "FileDialogEx.h"

static const TCHAR* MRU_REGISTRY_KEY = _T("Software\\berupon\\ImageIO");

static CFileVersionInfo g_verInfo;

CMainFrame::CMainFrame()
{
	m_pImageView = boost::shared_ptr<CImageView>(new CImageView());
}

CMainFrame::~CMainFrame()
{
	if (m_hWnd) {
		DestroyWindow();
		m_hWnd = NULL;
	}
}

static CString GetAppTitle()
{
	CString str;
	str.LoadString(IDR_MAINFRAME);
	return str + " " + g_verInfo.GetProductVersion();
}

void CMainFrame::ReadFile(LPCTSTR filePath)
{
	CWaitCursor waiter;
	
	m_mru.AddToList(filePath);
	m_mru.WriteToRegistry(MRU_REGISTRY_KEY);
	if (!m_pImageView->ReadImage(filePath)) {
		PostMessage(WM_CLOSE);
	}
	CString str;
	str.Format(_T("%s - %s"), filePath, GetAppTitle());
	SetWindowText(str);
}

void CMainFrame::OnViewRender(size_t xRatioTarget, size_t xRatioSource, size_t yRatioTarget, size_t yRatioSource, DWORD elapsed)
{
	TCHAR buff[128] = {0};
	_stprintf(buff, _T("x:%5d/%5d y:%5d/%5d  %d msec elapsed"), xRatioTarget, xRatioSource, yRatioTarget, yRatioSource, elapsed);
//	_stprintf(buff, "%d msec elapsed", elapsed);
	m_sBar.SetText(1, buff);
}

bool CMainFrame::OnReadProgress(size_t lineStart, size_t lineEnd)
{
	TCHAR buff[128] = {0};
	_stprintf(buff, _T("%d,%d"), lineStart, lineEnd);
	m_sBar.SetText(2, buff);
	
	return true;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;
	
	return FALSE; //m_pImageView->PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	g_verInfo.Create();

	m_pOpenFileDlg = new CFileDialogEx(
		TRUE, NULL, NULL, OFN_HIDEREADONLY|OFN_ENABLESIZING,
		_T("サポートしているファイル (*.tif; *.tiff; *.jpg; *.jpeg; *.png; *.bmp)\0*.tif;*.tiff;*.jpg;*.jpeg;*.png;*.bmp\0ビットマップ ファイル (*.bmp)\0*.bmp\0JPEGファイル (*.jpg; *.jpeg)\0*.jpg;*.jpeg\0TIFFファイル (*.tif; *.tiff)\0*.tif;*.tiff\0PNGファイル (*.png)\0*.png\0\0"),
		m_hWnd
	);
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	// remove old menu
	SetMenu(NULL);
	
	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
	
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);
	
	CreateSimpleStatusBar();
	m_sBar.SubclassWindow(m_hWndStatusBar);
	int arrParts[] = 
	{ 
		ID_DEFAULT_PANE, 
		ID_PANE_1,
		ID_PANE_2,
	};
	m_sBar.SetPanes(arrParts, sizeof(arrParts) / sizeof(arrParts[0]), false);
	m_sBar.SetPaneWidth(ID_PANE_1, 360);
	m_sBar.SetPaneWidth(ID_PANE_2, 100);
	
	// MRUファイルリストを設定
	CMenuHandle menu = m_CmdBar.GetMenu();
	CMenuHandle menuFile = menu.GetSubMenu(0);
	CMenuHandle menuMru = menuFile.GetSubMenu(1);
	m_mru.SetMenuHandle(menuMru);
	m_mru.SetMaxEntries(5);
	m_mru.ReadFromRegistry(MRU_REGISTRY_KEY);
	
	m_hWndClient = m_pImageView->Create(
		m_hWnd,
		rcDefault,
		NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE
	);
	
	UISetCheck(ID_RESAMPLINGFILTERS_BOX_INTRINSICS, 1);
	m_pImageView->m_resamplingFilterType = ResamplingFilterType_BoxIntrinsics;
	UISetCheck(ID_OPTIONS_PRESERVEASPECTRATIO, 1);
	m_pImageView->m_bPreserveAspectRatio = true;
	
	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	
	m_pImageView->m_renderReportDelegate.bind(this, &CMainFrame::OnViewRender);
	m_pImageView->m_readProgressDelegate.bind(this, &CMainFrame::OnReadProgress);
	
	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	SetWindowText(GetAppTitle());
	
	return 0;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.m_pFileVersionInfo = &g_verInfo;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
	if (m_pOpenFileDlg->DoModal() == IDOK) {
		ReadFile(m_pOpenFileDlg->m_szFileName);
	}
	return 0;
}

LRESULT CMainFrame::OnFileRecent(UINT uNotifyCode, int nID, HWND hWndCtl)
{
	CString strFile;
	if (m_mru.GetFromList(nID, strFile)) {
		CFindFile find;
		if (find.FindFile(strFile)) {
			// IDで指定したアイテムをMRUファイルリストの先頭に移動
			m_mru.MoveToTop(nID);
			ReadFile(strFile);
		}else {
			// IDで指定したアイテムをMRUファイルリストから削除
			m_mru.RemoveFromList(nID);
		}
		// MRUファイルリストをレジストリに書き込む
		m_mru.WriteToRegistry(MRU_REGISTRY_KEY);
	}
	return 0;
}

LRESULT CMainFrame::OnDropFiles(HDROP hdrop)
{
	SetForegroundWindow(m_hWnd);
	
	TCHAR filePath[256] = {0};
	DragQueryFile(hdrop, 0, filePath, 255);
	ReadFile(filePath);
	DragFinish(hdrop);
	return 0;
}

LRESULT CMainFrame::OnClose(void)
{
	DestroyWindow();
	m_hWnd = 0;
	return 0;
}

void CMainFrame::OnFilterSelect(enum ResamplingFilterType type)
{
	m_pImageView->m_bForceRender = true;
	if (m_pImageView->m_resamplingFilterType == type) {
		return;
	}
	m_pImageView->m_resamplingFilterType = type;
	UISetCheck(ID_RESAMPLINGFILTERS_NEARESTNEIGHBOR,type == ResamplingFilterType_NearestNeighbor);
//	UISetCheck(ID_RESAMPLINGFILTERS_BILINEAR,		type == ResamplingFilterType_Bilinear);
	UISetCheck(ID_RESAMPLINGFILTERS_BOX,			type == ResamplingFilterType_Box);
	UISetCheck(ID_RESAMPLINGFILTERS_BOX_INTRINSICS,	type == ResamplingFilterType_BoxIntrinsics);
	UISetCheck(ID_RESAMPLINGFILTERS_LANCZOS2SINC,	type == ResamplingFilterType_Lanczos2_Sinc);
	UISetCheck(ID_RESAMPLINGFILTERS_LANCZOS3SINC,	type == ResamplingFilterType_Lanczos3_Sinc);
	m_pImageView->Render();
}

LRESULT CMainFrame::OnCommand(UINT codeNotify, int id, HWND hwndCtl)
{
	switch (id) {
	case ID_RESAMPLINGFILTERS_NEARESTNEIGHBOR:
		OnFilterSelect(ResamplingFilterType_NearestNeighbor);
		break;
	//case ID_RESAMPLINGFILTERS_BILINEAR:
	//	OnFilterSelect(ResamplingFilterType_Bilinear);
	//	break;
	case ID_RESAMPLINGFILTERS_BOX:
		OnFilterSelect(ResamplingFilterType_Box);
		break;
	case ID_RESAMPLINGFILTERS_BOX_INTRINSICS:
		OnFilterSelect(ResamplingFilterType_BoxIntrinsics);
		break;
	case ID_RESAMPLINGFILTERS_LANCZOS2SINC:
		OnFilterSelect(ResamplingFilterType_Lanczos2_Sinc);
		break;
	case ID_RESAMPLINGFILTERS_LANCZOS3SINC:
		OnFilterSelect(ResamplingFilterType_Lanczos3_Sinc);
		break;
	case ID_OPTIONS_PRESERVEASPECTRATIO:
		m_pImageView->m_bPreserveAspectRatio = !m_pImageView->m_bPreserveAspectRatio;
		m_pImageView->m_bForceRender = true;
		UISetCheck(ID_OPTIONS_PRESERVEASPECTRATIO, m_pImageView->m_bPreserveAspectRatio);
		m_pImageView->Render();
		break;
	default:
		SetMsgHandled(FALSE);
		break;
	}
	return 0;
}

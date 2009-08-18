#pragma once

#include "ImageIO/DIBSection.h"

namespace gl
{
	class IBuffer2D;
};

enum ResamplingFilterType
{
	ResamplingFilterType_NearestNeighbor,
	ResamplingFilterType_Bilinear,
	ResamplingFilterType_Box,
	ResamplingFilterType_BoxIntrinsics,
	ResamplingFilterType_Lanczos2_Sinc,
	ResamplingFilterType_Lanczos3_Sinc,
};

class CImageView : public CWindowImpl<CImageView>
{
private:
	ImageIO::DIBSection m_dib;
//	ImageIO::GlobalAlloc_IO m_imgIO;
	
	HBITMAP m_hBMP;
	BITMAPINFO m_bmi;
	void* m_pBits;
	int m_lineOffset;
	CDC m_memDC;
	
	size_t m_orgImageWidth;
	boost::shared_ptr<gl::IBuffer2D> m_pSrc;
	boost::shared_ptr<gl::IBuffer2D> m_pTarget;
	
	size_t m_lastWidthRatioTarget;
	size_t m_lastHeightRatioTarget;
	size_t m_lastWidthRatioSource;
	size_t m_lastHeightRatioSource;
	
	bool render(CSize sz);

	size_t m_tmpLineOffsetBytes;
	std::vector<char*> m_tmpBuffs;
	void freeTmpBuffs();
	std::vector<char*> m_tmpLines;
	
	class ThreadPool* m_pThreadPool;
	SYSTEM_INFO m_systemInfo;
	size_t m_threadsToUse;

public:
	DECLARE_WND_CLASS(NULL)
	
	CImageView();
	~CImageView();

	bool m_bForceRender;
	ResamplingFilterType m_resamplingFilterType;
	bool m_bPreserveAspectRatio;

	fastdelegate::FastDelegate5<size_t, size_t, size_t, size_t, DWORD> m_renderReportDelegate;
	fastdelegate::FastDelegate2<size_t, size_t , bool> m_readProgressDelegate;
	
	bool ReadImage(LPCTSTR filePath);
	void Render();
	
	BEGIN_MSG_MAP(CImageView)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MSG_WM_PAINT(OnPaint)
		MSG_WM_SIZE(OnSize)
		MSG_WM_HSCROLL(OnHScroll)
		MSG_WM_VSCROLL(OnVScroll)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_MOUSEWHEEL(OnMouseWheel)
		MSG_WM_EXITSIZEMOVE(OnExitSizeMove)
	END_MSG_MAP()
	
// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
	
	LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct);
	LRESULT OnDestroy(void);
	LRESULT OnEraseBkgnd(HDC hdc);
	LRESULT OnTimer(UINT id, TIMERPROC lpTimerProc);
	LRESULT OnBitmapInfoAcquired(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnPaint(HDC hdc);
	LRESULT OnSize(UINT state, CSize Size);
	LRESULT OnHScroll(int code, short pos, HWND hwndCtl);
	LRESULT OnVScroll(int code, short pos, HWND hwndCtl);
	LRESULT OnMouseWheel(UINT ControlCodes, short Distance, CPoint Pt);
	LRESULT OnExitSizeMove(void);
	
};

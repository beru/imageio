#include "StdAfx.h"
#include ".\imageview.h"

#include "CreateDIB.h"

#include "fastdelegate.h"

#include "ImageIO/ImageReader.h"

#include <boost/math/common_factor.hpp>

#undef min

#include "converter.h"
#include "gl/windows.h"

#include "gl/AveragingReducer.h"
//#include "gl/AveragingReducer_intrinsics_sse2_inout3b.h"
#include "gl/AveragingReducer_intrinsics_sse2_inout4b.h"

#include "gl/ResampleImage.h"
#include "gl/ColorConverter.h"

#include "gl/IBuffer2D.h"
#include "gl/Buffer2D.h"

#include "ThreadPool.h"

static const size_t MAX_IMAGE_WIDTH = 16384 + 16;
static const size_t MAX_IMAGE_HEIGHT = 16384;

static const size_t MAX_WINDOW_WIDTH = 4096;
static const size_t MAX_WINDOW_HEIGHT = 4096;

CImageView::CImageView()
{
	GetSystemInfo(&m_systemInfo);
	m_pThreadPool = new ThreadPool();
//#if 1
#ifdef _DEBUG
	m_threadsToUse = 1;
#else
	m_threadsToUse = m_systemInfo.dwNumberOfProcessors;
#endif
	m_pThreadPool->Start(m_threadsToUse);
}

CImageView::~CImageView()
{
	m_pThreadPool->Stop();
	delete m_pThreadPool;
	freeTmpBuffs();
}

void CImageView::freeTmpBuffs()
{
	for (size_t i=0; i<m_tmpBuffs.size(); ++i) {
		_mm_free(m_tmpBuffs[i]);
	}
}

LRESULT CImageView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	HDC dc = GetDC();
	m_memDC.CreateCompatibleDC(dc);
	m_memDC.SetMapMode(GetMapMode(dc));
	ReleaseDC(dc);

	CWindow wnd(GetDesktopWindow());
	HDC hDC = wnd.GetDC();
	int width = GetDeviceCaps(hDC, HORZRES) + 10;
	if (width % 16) {
		width += 16 - width % 16;
	}
	int height = GetDeviceCaps(hDC, VERTRES) + 10;
	
	wnd.ReleaseDC(hDC);
	
	m_hBMP = CreateDIB(width, height, 32, m_lineOffset, m_bmi, m_pBits);
	m_memDC.SelectBitmap(m_hBMP);
	m_pTarget = boost::shared_ptr<gl::IBuffer2D>(gl::BuildBuffer2DFromBMP(m_bmi.bmiHeader, m_pBits));

	void* pVoid = m_pTarget->GetPixelVoidPtr(0,0);
	return 0;
}

LRESULT CImageView::OnDestroy(void)
{
	DeleteObject(m_hBMP);
	m_dib.Delete();
	//You should call SetMsgHandled(FALSE) or set bHandled = FALSE for the main window of your application
	return 0;
}

#if 1
typedef gl::fixed<16, int> NumericT;
#else
typedef double NumericT;
#endif

typedef gl::Color4< gl::ColorBGRA<NumericT> > TmpColor;
typedef gl::WinColor4 SrcColor;
typedef gl::WinColor4 TargetColor;

gl::ResampleTable<NumericT> g_resampleTable;
static ImageIO::DIBSectionReader reader(32, MAX_IMAGE_WIDTH, MAX_IMAGE_HEIGHT);

bool CImageView::ReadImage(LPCTSTR filePath)
{
	m_dib.Delete();
	
	reader.pImage_ = &m_dib;

Timer t;
//	io.progressDelegate_ = m_readProgressDelegate;
	if (!reader.Read(filePath)) {
		std::string errmsg = reader.GetLastErrorMessage();
		MessageBox(ConvertToTString(errmsg).c_str(), _T("error"));
		m_pSrc = boost::shared_ptr<gl::IBuffer2D>();
		return false;
	}
TCHAR buff[64];
_stprintf(buff, _T("%d\n"), t.Elapsed());
OutputDebugString(buff);

	m_orgImageWidth = reader.GetImageInfo().width;
	m_pSrc = boost::shared_ptr<gl::IBuffer2D>(gl::BuildBuffer2DFromBMP(m_dib.bmih_, m_dib.pBits_));
	
	gl::Buffer2D<TargetColor>* pTarget = (gl::Buffer2D<TargetColor>*) m_pTarget.get();
	gl::Buffer2D_Fill(*pTarget, TargetColor(0));

	m_lastWidthRatioTarget = 0;
	m_lastHeightRatioTarget = 0;
	m_lastWidthRatioSource = 0;
	m_lastHeightRatioSource = 0;
	
	freeTmpBuffs();
	m_tmpLineOffsetBytes = gl::align(std::min(m_pSrc->GetWidth(), MAX_WINDOW_WIDTH) * 4 * sizeof(NumericT), 16);
	const size_t height = m_pSrc->GetHeight();
	size_t lineIdx = 0;
	const size_t allocSize = m_tmpLineOffsetBytes * (height + 1);
	const size_t blockLines = 3 * m_threadsToUse;
	const size_t blockSize = m_tmpLineOffsetBytes * blockLines;
	const size_t blockCount = allocSize / blockSize + ((allocSize % blockSize) ? 1 : 0);
	m_tmpBuffs.resize(blockCount);
	m_tmpLines.resize(blockCount * blockLines);
	for (size_t i=0; i<blockCount; ++i) {
		char* addr = (char*) _mm_malloc(blockSize, 16);
		if (addr == 0) {
			errno_t err;
			_get_errno(&err);
			MessageBox(_T("メモリ不足です。"), _T("error"));
			return false;
		}
		m_tmpBuffs[i] = addr;
		for (size_t i=0; i<blockLines; ++i) {
			m_tmpLines[lineIdx++] = addr;
			addr += m_tmpLineOffsetBytes;
		}
	}
	
	Render();
	
	return true;
/*
	if (OpenClipboard()==FALSE) {
		return;
	}
	EmptyClipboard();
	SetClipboardData(CF_DIB, m_imgIO.hg_);
	CloseClipboard();
*/
}

LRESULT CImageView::OnEraseBkgnd(HDC hdc)
{
	return TRUE; // background is erased
}

LRESULT CImageView::OnPaint(HDC hdc)
{
	CPaintDC dc(m_hWnd);
	CRect rect = dc.m_ps.rcPaint;
//	TRACE("%d %d %d %d\n", rect.left, rect.top, rect.right, rect.bottom);
	if (m_dib.pBits_) {
		/*
		SCROLLINFO si;
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(SB_HORZ, &si);
		int scrollX = si.nPos;
		GetScrollInfo(SB_VERT, &si);
		int scrollY = si.nPos;
		*/
		int scrollX = 0;
		int scrollY = 0;
		dc.BitBlt(
			rect.left,
			rect.top,
			rect.Width(),
			rect.Height(),
			m_memDC,
			rect.left + scrollX,
			rect.top + scrollY,
			SRCCOPY
		);

	}else {
		dc.FillSolidRect(rect, RGB(100,100,100));
	}

	return 0;
}

struct ProgressReporter
{
	bool operator () (size_t nLine)
	{
		return true;
	}

	__forceinline bool operator () (const size_t curLine, const size_t lines)
	{
		return true;
	}

};

void CImageView::Render()
{
	CRect rec;
	GetClientRect(rec);
	render(rec.Size());
	Invalidate();
	UpdateWindow();
}

template <typename NumericT>
inline NumericT lanczos2(NumericT v0, NumericT v1, NumericT v2, NumericT v3, NumericT v4)
{
	const NumericT F1 = (0.496 + 0.284) * 1.1;
	const NumericT F2 = ((0.284 + -0.032) * 0.9) / 2;
	const NumericT F3 = -0.032 / 4;
	return F1 * v2 + F2 * (v1 + v3) + F3 * (v0 + v4);
}

#include "gl/windows.h"

#if 1
typedef gl::fixed<8, int>							fixed8uint;
typedef gl::Color4< gl::ColorBGRA< fixed8uint > >	TmpColorT;
typedef gl::fixed<31, int>							fixed31uint;
typedef fixed31uint NumericT2;
#else
typedef gl::Color3< gl::ColorBGR< float > > TmpColorT;
typedef float NumericT2;
#endif

template<typename TargetColorT>
struct ColorConverter
{
	__forceinline TargetColorT operator () (const TmpColorT& from)
	{
		return TargetColorT(
			from.r,
			from.g,
			from.b
		);
	}
	__forceinline TargetColorT operator () (const TargetColorT& from)
	{
		return from;
	}
};

template <typename T>
struct Caller
{
	static void StaticCall(unsigned char slotNo, void* param)
	{
		Caller* pCaller = (Caller*) param;
		pCaller->Call(slotNo);
	}
	__forceinline void Call(unsigned char slotNo)
	{
		callee.Process(slotNo);
	}
	Caller(T& callee)
		:
		callee(callee)
	{
	}
	T& callee;
};


bool CImageView::render(CSize sz)
{
	if (sz.cx > MAX_WINDOW_WIDTH || sz.cy > MAX_WINDOW_HEIGHT) {
		return false;
	}
	
	if (!m_pSrc.get()) {
		return false;
	}
	
	const size_t srcWidth = m_pSrc->GetWidth();
	const size_t srcHeight = m_pSrc->GetHeight();
	size_t widthRatioTarget = std::min<int>(sz.cx, m_orgImageWidth);
	size_t widthRatioSource = m_orgImageWidth;
	size_t heightRatioTarget = std::min<int>(sz.cy, m_pSrc->GetHeight());
	size_t heightRatioSource = srcHeight;

	if (m_bPreserveAspectRatio) {
		const double hScale = widthRatioTarget / (double)widthRatioSource;
		const double vScale = heightRatioTarget / (double)heightRatioSource;
		if (hScale > vScale) {
			widthRatioTarget = (size_t)(widthRatioSource * vScale + 0.5);
		}else if (hScale < vScale) {
			heightRatioTarget = (size_t)(heightRatioSource * hScale + 0.5);
		}
	}
	const size_t targetWidth = widthRatioTarget;
	const size_t targetHeight = heightRatioTarget;
	const int targetLineOffset = m_pTarget->GetLineOffset();
	
	const size_t widthRatioGCD = boost::math::gcd(widthRatioTarget, widthRatioSource);
	widthRatioTarget /= widthRatioGCD;
	widthRatioSource /= widthRatioGCD;
	
	const size_t heightRatioGCD = boost::math::gcd(heightRatioTarget, heightRatioSource);
	heightRatioTarget /= heightRatioGCD;
	heightRatioSource /= heightRatioGCD;
	//	m_memDC.FillSolidRect(0,0,sz.cx,sz.cy, RGB(100,100,100));

#if 0
	TCHAR buff[256];
	_stprintf(buff, "%d %d\n", widthRatioTarget, widthRatioSource);
	OutputDebugString(buff);
#endif

	if (m_bForceRender
		|| widthRatioTarget != m_lastWidthRatioTarget
		|| heightRatioTarget != m_lastHeightRatioTarget
		|| widthRatioSource != m_lastWidthRatioSource
		|| heightRatioSource != m_lastHeightRatioSource
	) {
		m_lastWidthRatioTarget = widthRatioTarget;
		m_lastHeightRatioTarget = heightRatioTarget;
		m_lastWidthRatioSource = widthRatioSource;
		m_lastHeightRatioSource = heightRatioSource;
		
		const int srcLineOffset = m_pSrc->GetLineOffset();
		const int targetLineOffset = m_pTarget->GetLineOffset();
		const void* pSrc = m_pSrc->GetPixelVoidPtr(0,0);
		void* pTarget = m_pTarget->GetPixelVoidPtr(0,0);
		char* pTmp = m_tmpBuffs[0];
		
		DWORD startTime = timeGetTime();
		switch (m_resamplingFilterType) {
		case ResamplingFilterType_NearestNeighbor:
			gl::ResampleImage<fixed8uint>(
				(const SrcColor*)pSrc, m_orgImageWidth, srcHeight, srcLineOffset,
				(TargetColor*)pTarget, targetWidth, targetHeight, targetLineOffset,
				gl::LineSampler_NearestNeighbor(), ProgressReporter()
			);
			break;
		case ResamplingFilterType_Bilinear:
			gl::ResampleImage<fixed8uint>(
				(const SrcColor*)pSrc, m_orgImageWidth, srcHeight, srcLineOffset,
				(TargetColor*)pTarget, targetWidth, targetHeight, targetLineOffset,
				gl::LineSampler_Bilinear<TmpColorT>(), ProgressReporter()
			);
			break;
		case ResamplingFilterType_Box:
			gl::AveragingReduce<NumericT2>(
				widthRatioTarget, widthRatioSource,
				heightRatioTarget, heightRatioSource,
				(const SrcColor*) pSrc, srcWidth, srcHeight, srcLineOffset,
				(TargetColor*) pTarget, targetLineOffset,
				(TmpColorT*) pTmp,
				TargetColor(),
				ColorConverter<TargetColor>(),
				ProgressReporter()
			);
			break;
		case ResamplingFilterType_BoxIntrinsics:
			{
				gl::intrinsics_sse2_inout4b::AveragingReduceParams params = {
					heightRatioTarget, heightRatioSource, srcHeight,
					widthRatioTarget, widthRatioSource, m_orgImageWidth,
					(const __m128i*)pSrc, srcLineOffset,
					(__m128i*)pTarget, targetLineOffset,
					(__m128i*)pTmp
				};
				static gl::intrinsics_sse2_inout4b::AveragingReducerCaller reducer;
				reducer.Setup(&params, m_threadsToUse);
				if (m_threadsToUse == 1) {
					reducer.Process(0);
				}else {
					Caller<gl::intrinsics_sse2_inout4b::AveragingReducerCaller> caller(reducer);
					for (size_t i=0; i<m_threadsToUse; ++i) {
						m_pThreadPool->SetSlot(i, Caller<gl::intrinsics_sse2_inout4b::AveragingReducerCaller>::StaticCall, &caller);
					}
					m_pThreadPool->ExecJobs();
					m_pThreadPool->WaitJobs();
				}
			}
			break;
		case ResamplingFilterType_Lanczos2_Sinc:
			gl::ResampleImage(
				(const SrcColor*)pSrc, m_orgImageWidth, srcHeight, srcLineOffset,
				(TargetColor*)pTarget, targetLineOffset,
				(TmpColor**)(&m_tmpLines[0]),
				widthRatioSource, widthRatioTarget,
				heightRatioSource, heightRatioTarget,
				gl::lanczos2_function<double>(), g_resampleTable
			);
			break;
		case ResamplingFilterType_Lanczos3_Sinc:
			gl::ResampleImage(
				(const SrcColor*)pSrc, m_orgImageWidth, srcHeight, srcLineOffset,
				(TargetColor*)pTarget, targetLineOffset,
				(TmpColor**)(&m_tmpLines[0]),
				widthRatioSource, widthRatioTarget,
				heightRatioSource, heightRatioTarget,
				gl::lanczos3_function<double>(), g_resampleTable
			);
			break;
		}

		m_renderReportDelegate(widthRatioTarget, widthRatioSource, heightRatioTarget, heightRatioSource, timeGetTime()-startTime);
	}

	// 縮小比率が変わらなくても、窓の大きさは変わる事があるので。
	// でも時間計測の範囲外で処理はあまり好ましくない…。
	if (m_bPreserveAspectRatio) {
		void* pTarget = m_pTarget->GetPixelVoidPtr(0,0);
		{
			int* pLine = (int*)pTarget;
			for (size_t y=0; y<sz.cy; ++y) {
				std::fill(pLine + targetWidth, pLine + sz.cx, 0);
				OffsetPtr(pLine, targetLineOffset);
			}
		}
		{
			int* pLine = (int*)pTarget;
			OffsetPtr(pLine, targetLineOffset * targetHeight);
			for (size_t y=targetHeight; y<sz.cy; ++y) {
				std::fill(pLine, pLine+sz.cx, 0);
				OffsetPtr(pLine, targetLineOffset);
			}
		}
	}
	m_bForceRender = false;
	return true;
}

LRESULT CImageView::OnSize(UINT state, CSize Size)
{
	if (IsWindow() && Size.cx != 0 && Size.cy != 0) {
		if (m_dib.pBits_) {
			render(Size);
		}
	}
	return 0;
}

LRESULT CImageView::OnHScroll(int code, short pos, HWND hwndCtl)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS|SIF_PAGE;
	GetScrollInfo(SB_HORZ, &si);
	int xPos = -1;
	switch (code) {
	case SB_LEFT:
		xPos = 0;
		break;
	case SB_RIGHT:
		xPos = si.nMax;
		break;
	case SB_LINELEFT:
		xPos = si.nPos - 1;
		break;
	case SB_LINERIGHT:
		xPos = si.nPos + 1;
		break;
	case SB_PAGELEFT:
		xPos = si.nPos - si.nPage;
		break;
	case SB_PAGERIGHT:
		xPos = si.nPos + si.nPage;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		xPos = pos;
		break;
	}
	if (xPos != -1) {
		si.nPos = xPos;
		SetScrollInfo(SB_HORZ, &si);
		Invalidate();
		UpdateWindow();
	}
	return 0;
}

LRESULT CImageView::OnVScroll(int code, short pos, HWND hwndCtl)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS|SIF_PAGE;
	GetScrollInfo(SB_VERT, &si);
	int yPos = -1;
	switch (code) {
	case SB_LEFT:
		yPos = 0;
		break;
	case SB_RIGHT:
		yPos = si.nMax;
		break;
	case SB_LINELEFT:
		yPos = si.nPos - 1;
		break;
	case SB_LINERIGHT:
		yPos = si.nPos + 1;
		break;
	case SB_PAGELEFT:
		yPos = si.nPos - si.nPage;
		break;
	case SB_PAGERIGHT:
		yPos = si.nPos + si.nPage;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		yPos = pos;
		break;
	}
	if (yPos != -1) {
		SetScrollPos(SB_VERT, yPos);
		Invalidate();
		UpdateWindow();
	}
	return 0;
}

LRESULT CImageView::OnMouseWheel(UINT ControlCodes, short Distance, CPoint Pt)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_POS|SIF_PAGE;
	GetScrollInfo(SB_VERT, &si);
	int offset = (Distance < 0 ) ? 1 : -1;
	const int max = abs(Distance) / WHEEL_DELTA;
	si.nPos += offset*max*40;
	SetScrollInfo(SB_VERT, &si);
	Invalidate();
	UpdateWindow();
	return 0;
}

LRESULT CImageView::OnExitSizeMove(void)
{
	return 0;
}

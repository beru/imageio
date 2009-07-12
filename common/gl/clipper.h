#pragma once

/*
	線分のClipping処理用
	
	http://www2.starcat.ne.jp/~fussy/algo/algo1-2.htm
	
	を参考に実装。
*/


#include "common.h"

namespace gl
{

template <typename T>
class Clipper
{
private:
	T minX_, minY_, maxX_, maxY_;
	
	enum {
		None = 0,
		Left = 1 << 1,
		Right = 1 << 2,
		Top = 1 << 3,
		Bottom = 1 << 4,
	};
	
	int calcSeqCode(T x, T y)
	{
		int code = None;
		if (x < minX_) code |= Left;
		if (x > maxX_) code |= Right;
		if (y < minY_) code |= Top;
		if (y > maxY_) code |= Bottom;
		return code;
	}
	
	int calcClippedPoint(int code, const T& x1, const T& y1, const T& x2, const T& y2, T* x, T* y)
	{
		T cx, cy;
		/* ウィンドウの左端より外側にある */
		if (code & Left) {
			if (x2 - x1 == T(0)) {
				cy = y1;
			}else {
				cy = (y2 - y1) * ((minX_ - x1) / (x2 - x1)) + y1;	/* ...(1) */
			}
			if (minY_<=cy && cy<=maxY_) {
				*x = minX_;
				*y = cy;
				return 1;  /* エリア内に収まったら終了 */
			}
		}

		/* ウィンドウの右端より外側にある */
		if (code & Right) {
			if (x2 - x1 == T(0)) {
				cy = y1;
			}else {
				cy = (y2 - y1) * ((maxX_ - x1) / (x2 - x1)) + y1;	/* ...(1) */
			}
			if (minY_<=cy && cy<=maxY_) {
				*x = maxX_;
				*y = cy;
				return 1;  /* エリア内に収まったら終了 */
			}
		}

		/* ウィンドウの上端より外側にある */
		if (code & Top) {
			if (y2 - y1 == T(0)) {
				cx = x1;
			}else {
				cx = (x2 - x1) * ((minY_ - y1) / (y2 - y1)) + x1;
			}
			if (minX_<=cx && cx<=maxX_) {
				*x = cx;
				*y = minY_;	
				return 1;  /* エリア内に収まったら終了 */
			}
		}

		/* ウィンドウの下端より外側にある */
		if (code & Bottom) {
			if (y2 - y1 == T(0)) {
				cx = x1;
			}else {
				cx = (x2 - x1) * ((maxY_ - y1) / (y2 - y1)) + x1;
			}
			if (minX_<=cx && cx<=maxX_) {
				*x = cx;
				*y = maxY_;
				return 1;  /* エリア内に収まったら終了 */
			}
		}
		return -1;  /* クリッピングされなかった場合、線分は完全に不可視 */
	}

public:
	Clipper(T minX, T minY, T maxX, T maxY)
	{
		setClipRegion(minX, minY, maxX, maxY);
	}

	Clipper()
	{
	}

	~Clipper() {}

	void SetClipRegion(T minX, T minY, T maxX, T maxY)
	{
		if (maxX < minX)
			std::swap(maxX, minX);
		if (maxY < minY)
			std::swap(maxY, minY);
		assert(minX >= T(0));
		assert(minY >= T(0));
		assert(maxX > T(0));
		assert(maxY > T(0));
		minX_ = minX;
		minY_ = minY;
		maxX_ = maxX;
		maxY_ = maxY;
	}
	
	T GetXMin() { return minX_; }
	T GetYMin() { return minY_; }
	T GetXMax() { return maxX_; }
	T GetYMax() { return maxY_; }

	/* クリッピングメインルーチン
	>0 ... クリッピングされた
	0  ... クリッピングの必要なし
	<0 ... 線分は完全に不可視 */
	int ClipLinePoint(
		T* x1, T* y1,	/* 始点 */
		T* x2, T* y2	/* 終点 */
	)
	{
		/* 端点分類コード */
		int code0, code1;
		code0 = calcSeqCode( *x1, *y1 );  /* 始点の端点分類コードを求める */
		code1 = calcSeqCode( *x2, *y2 );  /* 終点の端点分類コードを求める */

		/* 端点分類コードがどちらも0000ならばクリッピングは必要なし */
		if (code0==None && code1==None) return 0;
		
		/* 始点･終点の端点分類コードの論理積が非０ならば線分は完全に不可視 */
		if (code0 & code1)
			return -1;
		/* 始点のクリッピング */
		if (code0)
			if (calcClippedPoint( code0, *x1, *y1, *x2, *y2, x1, y1 ) < 0)
				return -1;

		/* 終点のクリッピング */
		if (code1)
			if (calcClippedPoint( code1, *x1, *y1, *x2, *y2, x2, y2 ) < 0)
				return -1;

		return 1;
	}
	
};

} // namespace gl

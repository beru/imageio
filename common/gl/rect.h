#pragma once

/*!
	
	矩形
	Rectangle
*/

#define _USE_MATH_DEFINES
#include <math.h>
#include "vector2.h"

namespace gl
{

template <typename T>
class Rect
{
public:
	T x;
	T y;
	T w;
	T h;

	Rect()
		:
		x(0),
		y(0),
		w(0),
		h(0)
	{
	}

	Rect(T x, T y, T w, T h)
		:
		x(x),
		y(y),
		w(w),
		h(h)
	{
	}
	
	void Set(T x, T y, T w, T h)
	{
		this->x	= x;
		this->y	= y;
		this->w	= w;
		this->h	= h;
	}
	
	Vector2<T> Center() const {
		return Vector2<T>(
			(x + w / 2),
			(y + h / 2)
		);
	}
	
	bool IsEmptyArea() const {
		return (w < 0) || (h < 0);
	}
	
	bool operator == (const Rect& rect) const {
		return (x == rect.x && y == rect.y && w == rect.w && h == rect.h);
	}
	
	bool Contains (T x, T y) const {
		return
			this->x <= x
			&& x <= (this->x + this->w)
			&& this->y <= y
			&& y <= (this->y + this->h)
		;
	}
	
	bool Contains(const Vector2<T>& pt) const {
		return contains(pt.x, pt.y);
	}
	
	//! 引数の矩形を内包するかどうか
	bool Contains(const Rect& rect) const {
		return
			x <= rect.x
			&& (rect.x+rect.w) <= (x+w)
			&& y <= rect.y
			&& (rect.y+rect.h) <= (y+h)
		;
	}
	
	void Inflate(T dx, T dy) {
		x -= dx;
		y -= dy;
		w += 2 * dx;
		h += 2 * dy;
	}
	
	void Inflate(const Vector2<T>& point) {
		Inflate(point.x, point.y);
	}
	
	static bool Intersect(Rect& c, const Rect& a, const Rect& b) {
		T minX = max(a.x, b.x);
		T maxX = min(a.x+a.w, b.x+b.w);
		T minY = max(a.y, b.y);
		T maxY = min(a.y+a.h, b.y+b.h);
		
		c.x = minX;
		c.w = maxX - minX;
		c.y = minY;
		c.h = maxY - minY;
		return !c.IsEmptyArea();
	}
	
	bool IntersectsWith(const Rect& rect) const {
		return 
			x < rect.x+rect.w
			&& x+w > rect.x
			&& y < y+h
			&& y+h > rect.y
		;
	}
	
	static bool Union(Rect& c, const Rect& a, const Rect& b) {
		T x2 = std::max(a.x+a.w, b.x+b.w);
		T y2 = std::max(a.y+a.h, b.y+b.h);
		T x = std::min(a.x, b.x);
		T y = std::min(a.y, b.y);
		
		c.x = x;
		c.y = y;
		c.w = x2 - x;
		c.h = y2 - y;
		return c.IsEmptyArea();
	}
	
	void Offset(T x, T y) {
		this->x += x;
		this->y += y;
	}
	
	void Offset(const Vector2<T>& point) {
		Offset(point.x, point.y);
	}
	
};




} // namespace gl


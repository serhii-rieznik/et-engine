/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/line2d.h>

namespace et
{
	template <typename T>
	struct Segment2d
	{
		vector2<T> start;
		vector2<T> end;

	public:
		Segment2d()
			{ }
		
		Segment2d(const vector2<T>& s, const vector2<T>& e) :
			start(s), end(e) { }

		vector2<T> direction() const
			{ return normalize(end - start); }
		
		Line2d<T> line() const
			{ return Line2d<T>(start, end); }
		
		bool containsPoint(const vector2<T>& pt) const
		{
			vector2<T> startToEnd = end - start;
			vector2<T> startToPoint = pt - start;

			T len = startToEnd.dotSelf();
			T epsilonScale = len * std::log(std::sqrt(len)) * INV_LN_2;
			
			T epsilon = std::numeric_limits<float>::epsilon() *
				(epsilonScale > static_cast<T>(1) ? epsilonScale : static_cast<T>(1));
			
			T outerProduct = std::abs(startToEnd.x * startToPoint.y - startToEnd.y * startToPoint.x);
			if (outerProduct <= epsilon)
			{
				if (dot(startToEnd, startToPoint) >= 0)
					return dot(start - end, pt - end) >= 0;
				
				return false;
			}
			
			return false;
		}
		
		bool intersects(const Segment2d<T>& s, vector2<T>* point) const
		{
			vector2<T> i;
			if (line().intersects(s.line(), &i))
			{
				if (containsPoint(i) && s.containsPoint(i))
				{
					if (point)
						*point = i;
					
					return true;
				}
				return false;
			}
			return false;
		}
	};
}

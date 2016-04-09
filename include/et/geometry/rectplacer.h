/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/geometry.h>

namespace et
{
	class RectPlacer
	{
	public:
        using RectList = Vector<recti>;

	public:
		RectPlacer(const vec2i& contextSize, bool addSpace);
		
		bool place(const vec2i& size, recti& placedPosition);

		const RectList& placedItems() const 
			{ return _placedItems; }
		
		void addPlacedRect(const recti&);
		
		const vec2i& contextSize() const
			{ return _contextSize; }

	private:
		vec2i _contextSize;
		RectList _placedItems;
		bool _addSpace;
	};
}

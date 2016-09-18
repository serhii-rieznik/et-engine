/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/geometry/rectplacer.h>

using namespace et;

RectPlacer::RectPlacer(const vec2i& contextSize, bool addSpace) :
	_contextSize(contextSize), _addSpace(addSpace)
{

}

void RectPlacer::addPlacedRect(const recti& r)
{
	_placedItems.push_back(r);
}

bool RectPlacer::place(const vec2i& size, recti& placedPosition)
{
	int w = size.x;
	int h = size.y;

	if (_addSpace)
	{
		if (w < _contextSize.x - 1)
			w++;

		if (h < _contextSize.y - 1)
			h++;
	}

	placedPosition = recti(vec2i(0), vec2i(w, h));

	if (_placedItems.size() == 0)
	{
		_placedItems.push_back(placedPosition);
		return true;
	}
	
	for (const recti& i : _placedItems)
	{
		placedPosition.setOrigin(i.origin() + vec2i(i.size().x, 0));

		bool placed = (placedPosition.origin().x + w <= _contextSize.x) &&
			(placedPosition.origin().y + h <= _contextSize.y);
		
		if (placed)
		{
			for (const recti& ii : _placedItems)
			{
				if (ii.intersects(placedPosition) && (ii != i))
				{
					placed = false;
					break;
				}
			}
		}

		if (placed)
		{
			_placedItems.push_back(placedPosition);
			return true;
		}

		placedPosition.setOrigin(i.origin() + vec2i(0, i.size().y));
		placed = (placedPosition.origin().x + w <= _contextSize.x) &&
			(placedPosition.origin().y + h <= _contextSize.y);
		
		if (placed)
		{
			for (const recti& ii : _placedItems)
			{
				if (ii.intersects(placedPosition) && (ii != i))
				{
					placed = false;
					break;
				}
			}
		}

		if (placed) 
		{
			_placedItems.push_back(placedPosition);
			return true;
		}
	}
	
	return false;
}

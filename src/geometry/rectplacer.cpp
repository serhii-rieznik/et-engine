/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/geometry/rectplacer.h>

using namespace et;

RectPlacer::RectPlacer(const vec2i& contextSize, bool addSpace) :
	_contextSize(contextSize), _addSpace(addSpace)
{

}

void RectPlacer::addPlacedRect(const rect& r)
{
	_placedItems.push_back(r);
}

bool RectPlacer::place(const vec2i& size, rect& placedPosition)
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

	placedPosition = rect(vec2(0.0f), vec2(static_cast<float>(w), static_cast<float>(h)));

	if (_placedItems.size() == 0)
	{
		_placedItems.push_back(placedPosition);
		return true;
	}
	
	for (const rect& i : _placedItems)
	{
		placedPosition.setOrigin(i.origin() + vec2(i.size().x, 0.0f));

		bool placed = (placedPosition.origin().x + w <= _contextSize.x) &&
			(placedPosition.origin().y + h <= _contextSize.y);
		
		if (placed)
		{
			for (const rect& ii : _placedItems)
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

		placedPosition.setOrigin(i.origin() + vec2(0.0f, i.size().y));
		placed = (placedPosition.origin().x + w <= _contextSize.x) &&
			(placedPosition.origin().y + h <= _contextSize.y);
		
		if (placed)
		{
			for (const rect& ii : _placedItems)
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

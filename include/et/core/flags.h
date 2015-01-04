/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	class FlagsHolder
	{
	public:
		FlagsHolder(size_t flags = 0) :
			_flags(flags) { }

		size_t flags() const
			{ return _flags; }

		void setFlag(size_t flag)
			{ _flags = _flags | flag; }

		void removeFlag(size_t flag)
			{ _flags = _flags & (~flag); }

		bool hasFlag(size_t flag) const
			{ return (_flags & flag) == flag; }

		void setFlags(size_t value)
			{ _flags = value; }

		void toggleFlag(size_t value) 
		{ 
			if (hasFlag(value)) 
				removeFlag(value);
			else
				setFlag(value);
		}

	private:
		size_t _flags;
	};
}

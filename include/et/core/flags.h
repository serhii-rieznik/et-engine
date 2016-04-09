/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	class FlagsHolder
	{
	public:
		FlagsHolder(uint64_t flags = 0) :
			_flags(flags) { }

		uint64_t flags() const
			{ return _flags; }

		void setFlag(uint64_t flag)
			{ _flags = _flags | flag; }

		void removeFlag(uint64_t flag)
			{ _flags = _flags & (~flag); }

		bool hasFlag(uint64_t flag) const
			{ return (_flags & flag) == flag; }

		void setFlags(uint64_t value)
			{ _flags = value; }

		void toggleFlag(uint64_t value)
		{ 
			if (hasFlag(value)) 
				removeFlag(value);
			else
				setFlag(value);
		}

	private:
		uint64_t _flags = 0;
	};
}

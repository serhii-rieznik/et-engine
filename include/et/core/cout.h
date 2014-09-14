/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <ostream>

namespace et
{
	template <typename T>
	inline std::ostream& operator << (std::ostream& stream, const vector2<T>& value)
		{ return (stream << value.x << ET_DEFAULT_DELIMITER << value.y); }
	
	template <typename T>
	inline std::ostream& operator << (std::ostream& stream, const vector3<T>& value)
		{ return (stream << value.x << ET_DEFAULT_DELIMITER << value.y << ET_DEFAULT_DELIMITER << value.z); }
	
	template <typename T>
	inline std::ostream& operator << (std::ostream& stream, const vector4<T>& value)
	{
		return (stream << value.x << ET_DEFAULT_DELIMITER << value.y << ET_DEFAULT_DELIMITER <<
			value.z << ET_DEFAULT_DELIMITER << value.w);
	}
	
	template <typename T>
	inline std::ostream& operator << (std::ostream& stream, const Rect<T>& value)
		{ return (stream << value.left << ", " << value.top << ", " << value.width << "x" << value.height); }
	
	template <typename T>
	inline std::ostream& operator << (std::ostream& stream, const matrix4<T>& value)
	{
		return (stream << "{" << std::endl << "\t" << value[0] << std::endl <<
			"\t" << value[1] << std::endl << "\t" << value[2] << std::endl <<
			"\t" << value[3] << std::endl << "}" << std::endl);
	}
	
	std::ostream& operator << (std::ostream& stream, const StringList& list);
}

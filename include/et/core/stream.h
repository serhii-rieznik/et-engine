/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{
	enum StreamMode
	{
		StreamMode_Text,
		StreamMode_Binary
	};

	class InputStreamPrivate;
	class InputStream : public Shared
	{
	public:
		ET_DECLARE_POINTER(InputStream)
		
	public:
		InputStream();
		InputStream(const std::string& file, StreamMode mode);

		~InputStream();

		bool valid();
		bool invalid();

		std::istream& stream();

	private:
		ET_DECLARE_PIMPL(InputStream, 32)
	};
}

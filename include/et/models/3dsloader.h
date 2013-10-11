/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <istream>

namespace et
{
	class Autodesk3DSLoader
	{
	public:
		void load(std::istream& stream);
		void load(const std::string& filename);

	private:
		void read3dEditorChunk(std::istream& stream, size_t size);
		void readObject(std::istream& stream, size_t size);
		void readMesh(std::istream& stream, size_t size);
	};
}
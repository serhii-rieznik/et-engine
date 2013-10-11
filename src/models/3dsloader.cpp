/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#include <fstream>
#include <et/core/containers.h>
#include <et/core/tools.h>
#include <et/models/3dsloader.h>
#include <et/models/3dsloader.const.h>

/*
* Supporting methods
*/

namespace et
{
	namespace a3ds
	{
		inline A3DSChunk readChunk(std::istream& s)
		{
			A3DSChunk chunk = { };
			s.read(chunk.binary, sizeof(chunk.d));
			chunk.d.size -= sizeof(chunk.d);
			return chunk;
		}

		inline void skipChunk(std::istream& stream, const A3DSChunk& chunk)
		{
			stream.seekg(chunk.d.size, std::ios::cur);
		}

		uint16_t readInt16(std::istream& s)
		{
			char value[2] = { };
			s.read(value, 2);
			return *(reinterpret_cast<uint16_t*>(value));
		}

		uint32_t readInt32(std::istream& s)
		{
			char value[4] = { };
			s.read(value, 4);
			return *(reinterpret_cast<uint32_t*>(value));
		}

		float readFloat(std::istream& s)
		{
			char value[4] = { };
			s.read(value, 4);
			return *(reinterpret_cast<float*>(value));
		}

		std::string readString(std::istream& s)
		{
			char data[1024] = { };
			size_t index = 0;
			do 
			{ 
				s.read(&data[index], 1);
			}
			while (data[index++] != 0);

			return std::string(data);
		}
	}
}

using namespace et;
using namespace et::a3ds;

void Autodesk3DSLoader::load(std::istream& stream)
{
	if (stream.fail()) return;

	A3DSChunk cmain = readChunk(stream);
	if (cmain.d.id != A3DSChunkId_MAIN) return;

	std::streamoff initialPos = stream.tellg();
	std::streamoff currentPos = initialPos;
	do
	{
		A3DSChunk chunk = readChunk(stream);
		if (chunk.d.id == A3DSChunkId_MAIN_VERSION)
		{
			uint32_t version = readInt32(stream);
			std::cout << "3DS file version: " << version << std::endl;
		}
		else if (chunk.d.id == A3DSChunkId_3D_EDITOR)
		{
			read3dEditorChunk(stream, chunk.d.size);
		}
		else if (chunk.d.id == A3DSChunkId_KEYFRAMER)
		{
			skipChunk(stream, chunk);
		}
		else
		{
			skipChunk(stream, chunk);
		}
		currentPos = stream.tellg();
	}
	while (currentPos - initialPos < cmain.d.size);
}

void Autodesk3DSLoader::load(const std::string& filename)
{
	std::fstream file(filename.c_str(), std::ios::in | std::ios::binary);
	load(file);
}

void Autodesk3DSLoader::read3dEditorChunk(std::istream& stream, size_t size)
{
	std::streamoff initialPos = stream.tellg();
	std::streamoff currentPos = initialPos;
	do
	{
		A3DSChunk chunk = readChunk(stream);
		if (chunk.d.id == A3DSChunkId_3D_EDITOR_VERSION)
		{
			uint32_t version = readInt32(stream);
			std::cout << "3DS file 3d data version: " << version << std::endl;
		}
		else if (chunk.d.id == A3DSChunkId_MASTER_SCALE)
		{
			float scale = readFloat(stream);
			std::cout << "3DS file 3d master scale: " << scale << std::endl;
		}
		else if (chunk.d.id == A3DSChunkId_OBJECT_BLOCK)
		{
			readObject(stream, chunk.d.size);
		}
		else if (chunk.d.id == A3DSChunkId_MATERIAL_BLOCK)
		{
			skipChunk(stream, chunk);
		}
		else
		{
			std::cout << "Unknown chunk in 3d editor block: " << chunk.d.id << std::endl;
			skipChunk(stream, chunk);
		}

		currentPos = stream.tellg();
	}
	while (currentPos - initialPos < size);
}

void Autodesk3DSLoader::readObject(std::istream& stream, size_t size)
{
	std::string name = readString(stream);
	std::cout << "New object: " << name;
	size -= name.length() + 1;

	std::streamoff initialPos = stream.tellg();
	std::streamoff currentPos = initialPos;
	do
	{
		A3DSChunk chunk = readChunk(stream);
		if (chunk.d.id == A3DSChunkId_TRIANGULAR_MESH)
		{
			std::cout << " - mesh" << std::endl;
			readMesh(stream, chunk.d.size);
		}
		else if (chunk.d.id == A3DSChunkId_CAMERA)
		{
			std::cout << " - camera" << std::endl;
			skipChunk(stream, chunk);
		}
		else if (chunk.d.id == A3DSChunkId_LIGHT)
		{
			std::cout << " - light" << std::endl;
			skipChunk(stream, chunk);
		}
		else
		{
			skipChunk(stream, chunk);
		}
		currentPos = stream.tellg();
	}
	while (currentPos - initialPos < size);
}

void Autodesk3DSLoader::readMesh(std::istream& stream, size_t size)
{
	std::streamoff initialPos = stream.tellg();
	std::streamoff currentPos = initialPos;
	do
	{
		A3DSChunk chunk = readChunk(stream);
		if (chunk.d.id == A3DSChunkId_VERTICES_LIST)
		{
			uint16_t numVertices = readInt16(stream);
			DataStorage<vec3> data(numVertices);
			stream.read(data.binary(), data.dataSize());
		}
		else if (chunk.d.id == A3DSChunkId_FACES_DESCRIPTION)
		{
			struct a_face
			{
				uint16_t face[3];
				uint16_t flag;
				a_face() { }
			};

			uint16_t numPolygons = readInt16(stream);
			DataStorage<a_face> data(numPolygons);
			stream.read(data.binary(), data.dataSize());
		}
		else if (chunk.d.id == A3DSChunkId_MAPPING_COORDINATES_LIST)
		{
			uint16_t numCoords = readInt16(stream);
			DataStorage<vec2> data(numCoords);
			stream.read(data.binary(), data.dataSize());
		}
		else if (chunk.d.id == A3DSChunkId_LOCAL_COORDINATES_SYSTEM)
		{
			float mat[12] = { };
			stream.read(reinterpret_cast<char*>(mat), sizeof(mat));
		}
		else if (chunk.d.id == A3DSChunkId_SMOOTHING_GROUP_LIST)
		{
			size_t numGroups = chunk.d.size / 4;
			DataStorage<uint32_t> data(numGroups);
			stream.read(data.binary(), data.dataSize());
		}
		else if (chunk.d.id == A3DSChunkId_FACES_MATERIAL)
		{
			std::string materialName = readString(stream);
			std::cout << "Material: " << materialName << std::endl;
			uint16_t numFaces = readInt16(stream);
			if (numFaces)
			{
				DataStorage<uint16_t> data(numFaces);
				stream.read(data.binary(), data.dataSize());
			}
		}
		else 
		{
			std::cout << "Unknown chunk in mesh block: " << chunk.d.id << std::endl;
			skipChunk(stream, chunk);
		}
		currentPos = stream.tellg();
	}
	while (currentPos - initialPos < size);
}
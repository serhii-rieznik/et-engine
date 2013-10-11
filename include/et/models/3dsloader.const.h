/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <stdint.h>

namespace et
{
	namespace a3ds
	{
#pragma pack(2)

		union A3DSChunk
		{
			struct ChunkData
			{ 
				uint16_t id;
				uint32_t size; 
			} d;

			char binary[sizeof(ChunkData)];
		};

#pragma pack()

		enum A3DSChunkId
		{
			A3DSChunkId_MAIN = 0x4D4D,
			A3DSChunkId_MAIN_VERSION = 0x0002,
			A3DSChunkId_MASTER_SCALE = 0x0100,
			A3DSChunkId_3D_EDITOR = 0x3D3D,
			A3DSChunkId_3D_EDITOR_VERSION = 0x3D3E,
			A3DSChunkId_OBJECT_BLOCK = 0x4000,
			A3DSChunkId_TRIANGULAR_MESH = 0x4100,
			A3DSChunkId_VERTICES_LIST = 0x4110,
			A3DSChunkId_FACES_DESCRIPTION = 0x4120,
			A3DSChunkId_FACES_MATERIAL = 0x4130,
			A3DSChunkId_MAPPING_COORDINATES_LIST = 0x4140,
			A3DSChunkId_SMOOTHING_GROUP_LIST = 0x4150,
			A3DSChunkId_LOCAL_COORDINATES_SYSTEM = 0x4160,
			A3DSChunkId_LIGHT = 0x4600,
			A3DSChunkId_SPOTLIGHT = 0x4610,
			A3DSChunkId_CAMERA = 0x4700,
			A3DSChunkId_MATERIAL_BLOCK = 0xAFFF,
			A3DSChunkId_MATERIAL_NAME = 0xA000,
			A3DSChunkId_AMBIENT_COLOR = 0xA010,
			A3DSChunkId_DIFFUSE_COLOR = 0xA020,
			A3DSChunkId_SPECULAR_COLOR = 0xA030,
			A3DSChunkId_TEXTURE_MAP_1 = 0xA200,
			A3DSChunkId_BUMP_MAP = 0xA230,
			A3DSChunkId_REFLECTION_MAP = 0xA220,
			A3DSChunkId_MAPPING_FILENAME = 0xA300,
			A3DSChunkId_MAPPING_PARAMETERS = 0xA351,
			A3DSChunkId_KEYFRAMER = 0xB000,
			A3DSChunkId_MESH_INFORMATION_BLOCK = 0xB002,
			A3DSChunkId_SPOT_LIGHT_INFORMATION_BLOCK = 0xB007,
			A3DSChunkId_FRAMES = 0xB008,
			A3DSChunkId_OBJECT_NAME = 0xB010,
			A3DSChunkId_OBJECT_PIVOT_POINT = 0xB013,
			A3DSChunkId_POSITION_TRACK = 0xB020,
			A3DSChunkId_ROTATION_TRACK = 0xB021,
			A3DSChunkId_SCALE_TRACK = 0xB022,
			A3DSChunkId_HIERARCHY_POSITION = 0xB030,
		};
	}
}
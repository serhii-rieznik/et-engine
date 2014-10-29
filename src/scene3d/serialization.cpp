/*
 * This file is part of `et engine`
 * Copyright 2009-2014 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/scene3d/serialization.h>

namespace et
{
	namespace s3d
	{
		const SceneVersion SceneVersionLatest = SceneVersion_1_0_4;
		const StorageVersion StorageVersionLatest = StorageVersion_1_0_1;

		ChunkId HeaderScene = "ETSCN";
		ChunkId HeaderData = "SDATA";
		ChunkId HeaderElements = "ELMTS";
		ChunkId HeaderMaterials = "MTRLS";
		ChunkId HeaderVertexArrays = "VARRS";
		ChunkId HeaderIndexArrays = "IARRS";

		bool chunkEqualTo(ChunkId chunk, ChunkId comp)
		{
			size_t offset = 1;
			while (*chunk && (*chunk++ == *comp++)) ++offset;
			return offset == SerializationChunkLength;
		}
	}
}

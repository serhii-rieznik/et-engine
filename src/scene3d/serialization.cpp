/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/scene3d/serialization.h>

namespace et
{
	namespace s3d
	{
		const SceneVersion SceneVersionLatest = SceneVersion_1_1_0;
		const StorageVersion StorageVersionLatest = StorageVersion_1_1_0;

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

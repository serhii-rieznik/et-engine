/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/containers.h>
#include <et/geometry/geometry.h>
#include <et/rendering/primitives.h>

using namespace et;

inline int getIndex(int u, int v, int u_sz, int v_sz)
	{ return clamp<int32_t>(u, 0, u_sz - 1) + clamp<int32_t>(v, 0, v_sz - 1) * u_sz; }

uint32_t primitives::primitiveCountForIndexCount(uint32_t numIndexes, PrimitiveType geometryType)
{
	switch (geometryType)
	{
		case PrimitiveType::Points:
			return numIndexes;
		
		case PrimitiveType::Lines:
			return numIndexes / 2;

		case PrimitiveType::Triangles:
			return numIndexes / 3;

		case PrimitiveType::LineStrips:
			return numIndexes - 1;

		case PrimitiveType::TriangleStrips:
			return numIndexes - 2;
		
		default:
			ET_FAIL("Invalid PrimitiveType");
	}
	
	return 0;
}

uint32_t primitives::indexCountForRegularMesh(const vec2i& meshSize, PrimitiveType geometryType)
{
	switch (geometryType)
	{
		case PrimitiveType::Points:
			return static_cast<uint32_t>(meshSize.square());

		case PrimitiveType::Triangles:
			return static_cast<uint32_t>(((meshSize.x > 1) ? meshSize.x - 1 : 1) * ((meshSize.y > 1) ? meshSize.y - 1 : 1) * 6);

		case PrimitiveType::TriangleStrips:
			return static_cast<uint32_t>(((meshSize.y > 1) ? meshSize.y - 1 : 1) * (2 * meshSize.x + 1) - 1);
			
		default:
			ET_FAIL("Unsupported element type")
	}

	return 0;
}

void primitives::createPhotonMap(VertexArray::Pointer buffer, const vec2i& density)
{
	buffer->increase(static_cast<uint32_t>(density.square()));
	
	VertexDataChunk chunk = buffer->chunk(VertexAttributeUsage::Position);
	ET_ASSERT(chunk->type() == DataType::Vec2);
	
	RawDataAcessor<vec2> data = chunk.accessData<vec2>(0);
	
	vec2 texel = vec2(1.0f / density.x, 1.0f / density.y);
	vec2 dxdy = vec2(0.5f / density.x, 0.5f / density.y);

	uint32_t k = 0;
	for (int i = 0; i < density.y; ++i)
	{
		for (int j = 0; j < density.x; ++j)
		{
			vec2 randomDev(2.0f * randomFloat(-dxdy.x, dxdy.x) / 3.0f, 2.0f * randomFloat(-dxdy.y, dxdy.y) / 3.0f);
			data[k++] = vec2(vec2(j * texel.x, i * texel.y) + dxdy + randomDev);
		}
	}
}

void primitives::createSphere(VertexStorage::Pointer data, float radius, const vec2i& density,
	const vec3& center, const vec2& hemiSphere)
{
	size_t lastIndex = data->capacity();
	data->increaseSize(static_cast<uint32_t>(density.square()));

	auto pos = data->accessData<et::DataType::Vec3>(et::VertexAttributeUsage::Position, lastIndex);
	auto nrm = data->accessData<et::DataType::Vec3>(et::VertexAttributeUsage::Normal, lastIndex);
	auto tex = data->accessData<et::DataType::Vec2>(et::VertexAttributeUsage::TexCoord0, lastIndex);

	bool hasPos = pos.valid();
	bool hasNorm = nrm.valid();
	bool hasTex = tex.valid();

	float dPhi = hemiSphere.x * DOUBLE_PI / (density.x - 1.0f);
	float dTheta = hemiSphere.y * PI / (density.y - 1.0f);

	int counter = 0;
	float theta = -HALF_PI;
	for (int i = 0; i < density.y; ++i)
	{
		float phi = 0;
		for (int j = 0; j < density.x; ++j)
		{
			vec3 p = fromSpherical(theta, phi);
			
			if (hasPos)
				pos[counter] = center + p * radius;

			if (hasNorm)
				nrm[counter] = p;

			if (hasTex)
			{
				tex[counter] = vec2(static_cast<float>(j) / (density.x - 1),
					1.0f - static_cast<float>(i) / (density.y - 1));
			}
			
			phi += dPhi;
			++counter;
		}

		theta += dTheta;
	} 
}

void primitives::createTorus(VertexArray::Pointer data, float centralRadius, float sizeRadius,
	const vec2i& density)
{
	uint32_t lastIndex = data->size();
	data->increase(static_cast<uint32_t>(density.square()));

	VertexDataChunk pos_c = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk norm_c = data->chunk(VertexAttributeUsage::Normal);
	VertexDataChunk tex_c = data->chunk(VertexAttributeUsage::TexCoord0);
	RawDataAcessor<vec3> pos = pos_c.valid() ? pos_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> norm = norm_c.valid() ? norm_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec2> tex = tex_c.valid() ? tex_c.accessData<vec2>(lastIndex) : RawDataAcessor<vec2>();

	bool hasPos = pos.valid();
	bool hasNorm = norm.valid();
	bool hasTex = tex.valid();

	float dPhi = DOUBLE_PI / static_cast<float>(density.x - 1);
	float dTheta = DOUBLE_PI / static_cast<float>(density.y - 1);

	int counter = 0;
	float theta = -HALF_PI;
	for (int i = 0; i < density.y; ++i)
	{
		float phi = 0.0;
		for (int j = 0; j < density.x; ++j)
		{
			if (hasPos)
			{
				vec3 angleScale(std::cos(phi), std::sin(theta), std::sin(phi));
				vec3 offset(centralRadius, 0.0f, centralRadius);
				pos[counter] = (sizeRadius * vec3(std::cos(theta), 1.0f, std::cos(theta)) + offset) * angleScale;
			}

			if (hasNorm)
				norm[counter] = fromSpherical(theta, phi);

			if (hasTex)
			{
				float u = static_cast<float>(j) / (density.x - 1);
				float v = static_cast<float>(i) / (density.y - 1);
				tex[counter] = vec2(u, v);
			}

			phi += dPhi;
			++counter;
		}

		theta += dTheta;
	} 
}

void primitives::createCylinder(VertexArray::Pointer data, float radius, float height, const vec2i& density,
	const vec3& center)
{
	uint32_t lastIndex = data->size();
	data->increase(static_cast<uint32_t>(density.square()));

	VertexDataChunk pos_c = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk norm_c = data->chunk(VertexAttributeUsage::Normal);
	VertexDataChunk tex_c = data->chunk(VertexAttributeUsage::TexCoord0);
	RawDataAcessor<vec3> pos = pos_c.valid() ? pos_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> norm = norm_c.valid() ? norm_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec2> tex = tex_c.valid() ? tex_c.accessData<vec2>(lastIndex) : RawDataAcessor<vec2>();

	bool hasPos = pos.valid();
	bool hasNorm = norm.valid();
	bool hasTex = tex.valid();

	float dPhi = DOUBLE_PI / (density.x - 1.0f);

	int counter = 0;
	float y = -0.5f * height;
	float dy = height / static_cast<float>(density.y - 1);

	for (int v = 0; v < density.y; ++v)
	{
		float phi = 0.0f;
		for (int j = 0; j < density.x; ++j)
		{
			vec3 p = vec3(std::cos(phi) * radius, y, std::sin(phi) * radius);

			if (hasPos)
				pos[counter] = center + p;

			if (hasNorm)
				norm[counter] = normalize(p * vec3(1.0f, 0.0f, 1.0f));

			if (hasTex)
				tex[counter] = vec2(phi / DOUBLE_PI, static_cast<float>(v) / (density.y - 1));

			phi += dPhi;
			++counter;
		}

		y += dy;
	}


}

void primitives::createSquarePlane(VertexArray::Pointer data, const vec3& normal, const vec2& size,
	const vec2i& density, const vec3& center, const vec2& texCoordScale, const vec2& texCoordOffset)
{
	uint32_t lastIndex = data->size();
	data->increase(static_cast<uint32_t>(density.square()));

	VertexDataChunk pos_c = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk norm_c = data->chunk(VertexAttributeUsage::Normal);
	VertexDataChunk tex_c = data->chunk(VertexAttributeUsage::TexCoord0);
	RawDataAcessor<vec3> pos = pos_c.valid() ? pos_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> norm = norm_c.valid() ? norm_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec2> tex = tex_c.valid() ? tex_c.accessData<vec2>(lastIndex) : RawDataAcessor<vec2>();

	bool hasPos = pos.valid();
	bool hasNorm = norm.valid();
	bool hasTex = tex.valid();

	vec3 angles = toSpherical(normal);
	angles += vec3(-HALF_PI, HALF_PI, 0.0f);

	vec3 normalized = normalize(normal);
	vec3 o2 = fromSpherical(angles.y, angles.x);
	vec3 o1 = cross(normalized, o2);
	o2 = cross(o1, normalized);

	vec3 v00 = -size.x * o1 - size.y * o2;
	vec3 v01 = -size.x * o1 + size.y * o2;
	vec3 v10 = size.x * o1 - size.y * o2;
	vec3 v11 = size.x * o1 + size.y * o2;

	vec3 n = plane(triangle(v00, v01, v10)).normal();
	vec2i dv(density.x < 2 ? 2 : density.x, density.y < 2 ? 2 : density.y);

	float dx = 1.0f / static_cast<float>(dv.x - 1);
	float dy = 1.0f / static_cast<float>(dv.y - 1);

	uint32_t counter = 0;
	for (int v = 0; v < dv.y; v++)
	{
		float py = v * dy;
		vec3 m0 = mix(v00, v10, py);
		vec3 m1 = mix(v01, v11, py);

		for (int u = 0; u < dv.x; u++)
		{
			float px = u * dx;
			if (hasPos)
				pos[counter] = center + mix(m0, m1, px);
			if (hasTex)
				tex[counter] = texCoordOffset + vec2(py, 1.0f - px) * texCoordScale;
			++counter;
		}

	}

	if (!hasNorm) return;

	for (int v = 0; v < dv.y; v++)
	{
		for (int u = 0; u < dv.x; u++)
		{
			int n00 = getIndex(u, v, dv.x, dv.y);
			int n01 = 0;
			int n10 = 0;

			vec3 p00 = pos[n00];
			vec3 p01;
			vec3 p10;

			if (u == dv.x - 1)
			{
				n01 = getIndex(u - 1, v , dv.x, dv.y);
				p01 = p00 + (p00 - pos[n01]);
			}
			else
			{
				n01 = getIndex(u + 1, v , dv.x, dv.y);
				p01 = pos[n01];
			} 
			if (v == dv.y -1)
			{
				n10 = getIndex(u, v - 1, dv.x, dv.y);
				p10 = p00 + (p00 - pos[n10]); 
			}
			else
			{
				n10 = getIndex(u, v + 1, dv.x, dv.y);
				p10 = pos[n10];
			}
			norm[n00] = plane(triangle(p00, p10, p01)).normal();
		}
	}

	for (int v = 0; v < dv.y; v++)
	{
		for (int u = 0; u < dv.x; u++)
		{
			n = 1.0f * norm[getIndex(u-1, v-1, dv.x, dv.y)] + 
				2.0f * norm[getIndex(u, v-1, dv.x, dv.y)] + 
				1.0f * norm[getIndex(u+1, v-1, dv.x, dv.y)] + 
				2.0f * norm[getIndex(u-1, v, dv.x, dv.y)] + 
				4.0f * norm[getIndex(u, v, dv.x, dv.y)] + 
				2.0f * norm[getIndex(u+1, v, dv.x, dv.y)] + 
				1.0f * norm[getIndex(u-1, v+1, dv.x, dv.y)] +
				2.0f * norm[getIndex(u, v+1, dv.x, dv.y)] +
				1.0f * norm[getIndex(u+1, v+1, dv.x, dv.y)];
			norm[getIndex(u, v, dv.x, dv.y)] = normalize(n);
		}
	}
}

IndexArray::Pointer primitives::createCirclePlane(VertexArray::Pointer data, const vec3& normal, float radius,
	uint32_t density, const vec3& center, const vec2&, const vec2&)
{
	IndexArray::Pointer result = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, 3 * density, PrimitiveType::Triangles);
	
	data->fitToSize(1 + density);
	auto pos = data->chunk(VertexAttributeUsage::Position).accessData<vec3>(0);
	auto nrm = data->chunk(VertexAttributeUsage::Normal).accessData<vec3>(0);
	
	vec3 angles = toSpherical(normal);
	angles += vec3(-HALF_PI, HALF_PI, 0.0f);
	
	vec3 normalized = normalize(normal);
	vec3 o2 = fromSpherical(angles.y, angles.x);
	vec3 o1 = cross(normalized, o2);
	o2 = cross(o1, normalized);
	
	pos[0] = center;
	nrm[0] = normal;
	
	uint32_t index = 0;
	float angle = 0.0f;
	float da = DOUBLE_PI / (static_cast<float>(density - 1));
	for (uint32_t i = 1; i < density; ++i)
	{
		pos[i] = center + (o1 * std::cos(angle) - o2 * std::sin(angle)) * radius;
		nrm[i] = normal;
		result->setIndex(0, index++);
		result->setIndex(i, index++);
		result->setIndex((i + 1 < density) ? static_cast<uint32_t>(i + 1) : 1, index++);
		angle += da;
	}
		
	return result;
}

uint32_t primitives::buildTriangleStripIndexes(IndexArray& buffer, const vec2i& dim,
	uint32_t index0, uint32_t offset)
{
	uint32_t k = offset;
	uint32_t rowSize = static_cast<uint32_t>(dim.x);
	uint32_t colSize = static_cast<uint32_t>(dim.y);

	for (uint32_t v = 0; v < static_cast<uint32_t>(dim.y - 1); ++v)
	{
		uint32_t thisRow = index0 + v * rowSize;
		uint32_t nextRow = thisRow + rowSize;

		if (v % 2 == 0)
		{
			for (uint32_t u = 0; u < rowSize; ++u)
			{
				buffer.setIndex(u + thisRow, k++);
				buffer.setIndex(u + nextRow, k++);
			}
			if (v != colSize - 2)
				buffer.setIndex((rowSize - 1) + nextRow, k++);
		}
		else
		{
			for (uint32_t u = rowSize - 1; ; --u)
			{
				buffer.setIndex(u + thisRow, k++);
				buffer.setIndex(u + nextRow, k++);
				if (u == 0)	break;
			}

			if (v != colSize - 2)
				buffer.setIndex(nextRow, k++);
		}
	}

	return k; 
}

uint32_t primitives::buildTrianglesIndexes(IndexArray& buffer, const vec2i& dim,
	uint32_t vertexOffset, uint32_t indexOffset)
{
	uint32_t k = indexOffset;
	uint32_t rowSize = static_cast<uint32_t>(dim.x);
	uint32_t colSize = static_cast<uint32_t>(dim.y);

	for (uint32_t i = 0; i < colSize - 1; ++i)
	{
		for (uint32_t j = 0; j < rowSize - 1; ++j)
		{
			uint32_t value = vertexOffset + j + i * rowSize;

			buffer.setIndex(value, k++);
			buffer.setIndex(value + rowSize, k++);
			buffer.setIndex(value + 1, k++);
			buffer.setIndex(value + rowSize, k++);
			buffer.setIndex(value + rowSize + 1, k++);
			buffer.setIndex(value + 1, k++);
		} 
	}
	
	return k;
}

uint32_t primitives::buildTrianglesIndexes(IndexArray::Pointer buffer, const vec2i& dim,
	uint32_t vertexOffset, uint32_t indexOffset)
{
	ET_ASSERT(buffer.valid());
	return buildTrianglesIndexes(buffer.reference(), dim, vertexOffset, indexOffset);
}

uint32_t primitives::buildTriangleStripIndexes(IndexArray::Pointer buffer, const vec2i& dim,
	uint32_t vertexOffset, uint32_t indexOffset)
{
	ET_ASSERT(buffer.valid());
	return buildTriangleStripIndexes(buffer.reference(), dim, vertexOffset, indexOffset);
}

void primitives::calculateNormals(VertexStorage::Pointer data, const IndexArray::Pointer& buffer, uint32_t first, uint32_t last)
{
	ET_ASSERT(first < last);
	
	if (!data->hasAttributeWithType(VertexAttributeUsage::Position, DataType::Vec3))
	{
		log::error("primitives::calculateNormals - vertex storage does not contain positions of type vec3.");
		return;
	}
	if (!data->hasAttributeWithType(VertexAttributeUsage::Normal, DataType::Vec3))
	{
		log::error("primitives::calculateNormals - vertex storage does not contain normals of type vec3.");
		return;
	}
	
	RawDataAcessor<vec3> pos = data->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
	RawDataAcessor<vec3> nrm = data->accessData<DataType::Vec3>(VertexAttributeUsage::Normal, 0);
	
	const auto& e = buffer->primitive(last);
	
	for (auto i = buffer->primitive(first); i != e; ++i)
	{
		const auto& p = (*i);
		nrm[p[0]] = vec3(0.0f);
		nrm[p[1]] = vec3(0.0f);
		nrm[p[2]] = vec3(0.0f);
	}
	
	for (auto i = buffer->primitive(first); i != e; ++i)
	{
		const auto& p = (*i);
		triangle t(pos[p[0]], pos[p[1]], pos[p[2]]);
		vec3 n = t.normalizedNormal() * t.square();
		nrm[p[0]] += n;
		nrm[p[1]] += n;
		nrm[p[2]] += n;
	}
	
	for (auto i = buffer->primitive(first); i != e; ++i)
	{
		const auto& p = (*i);
		nrm[p[0]].normalize();
		nrm[p[1]].normalize();
		nrm[p[2]].normalize();
	}
}

void primitives::calculateNormals(VertexArray::Pointer data, const IndexArray::Pointer& buffer, uint32_t first, uint32_t last)
{
	ET_ASSERT(first < last);

	VertexDataChunk posChunk = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk nrmChunk = data->chunk(VertexAttributeUsage::Normal);

	if (posChunk.invalid() || (posChunk->type() != DataType::Vec3) || !nrmChunk.valid() ||
		(nrmChunk->type() != DataType::Vec3))
	{
		log::error("primitives::calculateNormals - data is invalid.");
		return;
	}
	
	RawDataAcessor<vec3> nrm = nrmChunk.accessData<vec3>(0);
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(0);
	
	const auto& e = buffer->primitive(last);
	
	for (auto i = buffer->primitive(first); i != e; ++i)
	{
		const auto& p = (*i);
		nrm[p[0]] = vec3(0.0f);
		nrm[p[1]] = vec3(0.0f);
		nrm[p[2]] = vec3(0.0f);
	}

	for (auto i = buffer->primitive(first); i != e; ++i)
	{
		const auto& p = (*i);
		triangle t(pos[p[0]], pos[p[1]], pos[p[2]]);
		vec3 n = t.normalizedNormal() * t.square();
		nrm[p[0]] += n;
		nrm[p[1]] += n;
		nrm[p[2]] += n;
	}
	
	for (auto i = buffer->primitive(first); i != e; ++i)
	{
		const auto& p = (*i);
		nrm[p[0]].normalize();
		nrm[p[1]].normalize();
		nrm[p[2]].normalize();
	}
}

void primitives::calculateTangents(VertexArray::Pointer data, const IndexArray::Pointer& buffer,
	uint32_t first, uint32_t last)
{
	ET_ASSERT(first < last);
	
	VertexDataChunk posChunk = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk nrmChunk = data->chunk(VertexAttributeUsage::Normal);
	VertexDataChunk uvChunk = data->chunk(VertexAttributeUsage::TexCoord0);
	VertexDataChunk tanChunk = data->chunk(VertexAttributeUsage::Tangent);
	
	if (posChunk.invalid() || (posChunk->type() != DataType::Vec3) ||
		nrmChunk.invalid() || (nrmChunk->type() != DataType::Vec3) ||
		tanChunk.invalid() || (tanChunk->type() != DataType::Vec3) ||
		uvChunk.invalid() || (uvChunk->type() != DataType::Vec2))
	{
		log::error("primitives::calculateTangents - data is invalid.");
		return;
	}

	DataStorage<vec3> tan1(data->size(), 0);
	DataStorage<vec3> tan2(data->size(), 0);

	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(0);
	RawDataAcessor<vec3> nrm = nrmChunk.accessData<vec3>(0);
	RawDataAcessor<vec3> tan = tanChunk.accessData<vec3>(0);
	RawDataAcessor<vec2> uv = uvChunk.accessData<vec2>(0);

	for (IndexArray::PrimitiveIterator i = buffer->primitive(first), e = buffer->primitive(last); i != e; ++i)
	{
		IndexArray::Primitive& p = (*i);
		
		vec3& v1 = pos[p[0]];
		vec3& v2 = pos[p[1]];
		vec3& v3 = pos[p[2]];
		vec2& w1 = uv[p[0]];
		vec2& w2 = uv[p[1]];
		vec2& w3 = uv[p[2]];

		float x1 = v2.x - v1.x; 
		float x2 = v3.x - v1.x; 
		float y1 = v2.y - v1.y; 
		float y2 = v3.y - v1.y; 
		float z1 = v2.z - v1.z; 
		float z2 = v3.z - v1.z; 
		float s1 = w2.x - w1.x; 
		float s2 = w3.x - w1.x; 
		float t1 = w2.y - w1.y; 
		float t2 = w3.y - w1.y; 
		float r = 1.0f / (s1 * t2 - s2 * t1); 

		vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r); 

		tan1[p[0]] += sdir; 
		tan1[p[1]] += sdir; 
		tan1[p[2]] += sdir;
		tan2[p[0]] += tdir; 
		tan2[p[1]] += tdir; 
		tan2[p[2]] += tdir; 
	} 

	for (IndexArray::PrimitiveIterator i = buffer->primitive(first), e = buffer->primitive(last); i != e; ++i)
	{
		const IndexArray::Primitive& p = (*i);
		for (uint32_t k = 0; k < 3; ++k)
		{
			vec3& n = nrm[p[k]];
			vec3& t = tan1[p[k]]; 
			float value = dot(cross(n, t), tan2[p[k]]);
			tan[p[k]] = normalize(t - n * dot(n, t)) * signOrZero(value);
		}
	}
}

void primitives::calculateTangents(VertexStorage::Pointer data, const IndexArray::Pointer& buffer,
	uint32_t first, uint32_t last)
{
	ET_ASSERT(first < last);

	if (!data->hasAttributeWithType(VertexAttributeUsage::Position, DataType::Vec3))
	{
		log::error("primitives::calculateNormals - vertex storage does not contain positions of type vec3.");
		return;
	}
	if (!data->hasAttributeWithType(VertexAttributeUsage::Normal, DataType::Vec3))
	{
		log::error("primitives::calculateNormals - vertex storage does not contain normals of type vec3.");
		return;
	}
	if (!data->hasAttributeWithType(VertexAttributeUsage::TexCoord0, DataType::Vec2))
	{
		log::error("primitives::calculateNormals - vertex storage does not contain TexCoord0 of type vec2.");
		return;
	}
	if (!data->hasAttributeWithType(VertexAttributeUsage::Tangent, DataType::Vec3))
	{
		log::error("primitives::calculateNormals - vertex storage does not contain tangents of type vec3.");
		return;
	}

	RawDataAcessor<vec3> pos = data->accessData<DataType::Vec3>(VertexAttributeUsage::Position, 0);
	RawDataAcessor<vec3> nrm = data->accessData<DataType::Vec3>(VertexAttributeUsage::Normal, 0);
	RawDataAcessor<vec3> tan = data->accessData<DataType::Vec3>(VertexAttributeUsage::Tangent, 0);
	RawDataAcessor<vec2> uv = data->accessData<DataType::Vec2>(VertexAttributeUsage::TexCoord0, 0);

	DataStorage<vec3> tan1(data->capacity(), 0);
	DataStorage<vec3> tan2(data->capacity(), 0);

	for (IndexArray::PrimitiveIterator i = buffer->primitive(first), e = buffer->primitive(last); i != e; ++i)
	{
		IndexArray::Primitive& p = (*i);

		vec3& v1 = pos[p[0]];
		vec3& v2 = pos[p[1]];
		vec3& v3 = pos[p[2]];
		vec2& w1 = uv[p[0]];
		vec2& w2 = uv[p[1]];
		vec2& w3 = uv[p[2]];

		float x1 = v2.x - v1.x;
		float x2 = v3.x - v1.x;
		float y1 = v2.y - v1.y;
		float y2 = v3.y - v1.y;
		float z1 = v2.z - v1.z;
		float z2 = v3.z - v1.z;
		float s1 = w2.x - w1.x;
		float s2 = w3.x - w1.x;
		float t1 = w2.y - w1.y;
		float t2 = w3.y - w1.y;
		float r = 1.0f / (s1 * t2 - s2 * t1);

		vec3 sdir((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
		vec3 tdir((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);

		tan1[p[0]] += sdir;
		tan1[p[1]] += sdir;
		tan1[p[2]] += sdir;

		tan2[p[0]] += tdir;
		tan2[p[1]] += tdir;
		tan2[p[2]] += tdir;
	}

	for (IndexArray::PrimitiveIterator i = buffer->primitive(first), e = buffer->primitive(last); i != e; ++i)
	{
		const IndexArray::Primitive& p = (*i);
		for (uint32_t k = 0; k < 3; ++k)
		{
			vec3& n = nrm[p[k]];
			vec3& t = tan1[p[k]];
			float value = dot(cross(n, t), tan2[p[k]]);
			tan[p[k]] = normalize(t - n * dot(n, t)) * signOrZero(value);
		}
	}
}

void primitives::smoothTangents(VertexArray::Pointer data, const IndexArray::Pointer&,
	uint32_t first, uint32_t last)
{
	ET_ASSERT(first < last);
	
	VertexDataChunk posChunk = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk tanChunk = data->chunk(VertexAttributeUsage::Tangent);
	
	if (posChunk.invalid() || (posChunk->type() != DataType::Vec3) || !tanChunk.valid() ||
		(tanChunk->type() != DataType::Vec3))
	{
		log::error("primitives::smoothTangents - data is invalid.");
		return;
	}

	uint32_t len = last - first;
	DataStorage<vec3> tanSmooth(data->size());
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(first);
	RawDataAcessor<vec3> tan = tanChunk.accessData<vec3>(first);
	for (uint32_t p = 0; p < len; ++p)
	{
		tanSmooth[p] = tan[p];
		for (uint32_t i = p + 1; i < len; ++i)
		{
			if ((pos[p] - pos[i]).dotSelf() <= 1.0e-3f)
				tanSmooth[p] += tan[i];
		}
	}

	for (uint32_t i = 0; i < len; ++i)
		tan[i] = normalize(tanSmooth[i]);
}

#define ET_ADD_TRIANGLE(c1, c2, c3) \
	{ \
		pos[ k ] = corners[c1]; \
		pos[k+1] = corners[c2]; \
		pos[k+2] = corners[c3]; \
		if (hasNormals) \
		{ \
			nrm[k+0] = nrm[k+1] = nrm[k+2] = \
				triangle(corners[c1], corners[c2], corners[c3]).normalizedNormal(); \
		} \
		k += 3; \
	}


void primitives::createBox(VertexArray::Pointer data, const vec3& size, const vec3& center)
{
	VertexDataChunk posChunk = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk nrmChunk = data->chunk(VertexAttributeUsage::Normal);
	bool hasPosition = posChunk.valid() && (posChunk->type() == DataType::Vec3);
	bool hasNormals = nrmChunk.valid() && (nrmChunk->type() == DataType::Vec3);
	
	ET_ASSERT(hasPosition);
	(void)hasPosition;
	
	uint32_t offset = data->size();
	data->fitToSize(offset + 12 * 3);
	
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(offset);
	RawDataAcessor<vec3> nrm = nrmChunk.accessData<vec3>(offset);
	
	vec3 corners[8] =
	{
		center + vec3(-size.x,  size.y, -size.z),
		center + vec3( size.x,  size.y, -size.z),
		center + vec3(-size.x,  size.y,  size.z),
		center + vec3( size.x,  size.y,  size.z),
		center + vec3(-size.x, -size.y, -size.z),
		center + vec3( size.x, -size.y, -size.z),
		center + vec3(-size.x, -size.y,  size.z),
		center + vec3( size.x, -size.y,  size.z),
	};
	
	uint32_t k = 0;
	
	ET_ADD_TRIANGLE(0, 1, 4)
	ET_ADD_TRIANGLE(1, 3, 5)
	ET_ADD_TRIANGLE(1, 5, 4)
	ET_ADD_TRIANGLE(3, 0, 2)
	ET_ADD_TRIANGLE(3, 1, 0)
	ET_ADD_TRIANGLE(3, 7, 5)
	ET_ADD_TRIANGLE(4, 2, 0)
	ET_ADD_TRIANGLE(4, 5, 7)
	ET_ADD_TRIANGLE(4, 6, 2)
	ET_ADD_TRIANGLE(4, 7, 6)
	ET_ADD_TRIANGLE(6, 3, 2)
	ET_ADD_TRIANGLE(6, 7, 3)
}

void primitives::createOctahedron(VertexArray::Pointer data, float radius)
{
	VertexDataChunk posChunk = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk nrmChunk = data->chunk(VertexAttributeUsage::Normal);
	bool hasPosition = posChunk.valid() && (posChunk->type() == DataType::Vec3);
	bool hasNormals = nrmChunk.valid() && (nrmChunk->type() == DataType::Vec3);
	
	ET_ASSERT(hasPosition);
	(void)hasPosition;
	
	uint32_t offset = data->size();
	data->fitToSize(24);
					
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(offset);
	RawDataAcessor<vec3> nrm = nrmChunk.accessData<vec3>(offset);
	
	float d = 0.5f * SQRT_2 * radius;
	
	vec3 corners[6] =
	{
		vec3(0.0f,    d, 0.0f),
		vec3(  -d, 0.0f,    d),
		vec3(  -d, 0.0f,   -d),
		vec3(   d, 0.0f,    d),
		vec3(   d, 0.0f,   -d),
		vec3(0.0f,   -d, 0.0f)
	};
	
	uint32_t k = 0;
	ET_ADD_TRIANGLE(0, 2, 1)
	ET_ADD_TRIANGLE(0, 4, 2)
	ET_ADD_TRIANGLE(0, 3, 4)
	ET_ADD_TRIANGLE(0, 1, 3)
	ET_ADD_TRIANGLE(1, 2, 5)
	ET_ADD_TRIANGLE(2, 4, 5)
	ET_ADD_TRIANGLE(4, 3, 5)
	ET_ADD_TRIANGLE(3, 1, 5)
}

void primitives::createDodecahedron(VertexArray::Pointer data, float radius)
{
	VertexDataChunk posChunk = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk nrmChunk = data->chunk(VertexAttributeUsage::Normal);
	bool hasPosition = posChunk.valid() && (posChunk->type() == DataType::Vec3);
	bool hasNormals = nrmChunk.valid() && (nrmChunk->type() == DataType::Vec3);
	
	ET_ASSERT(hasPosition);
	(void)hasPosition;
	
	uint32_t offset = data->size();
	data->fitToSize(180);
	
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(offset);
	RawDataAcessor<vec3> nrm = nrmChunk.accessData<vec3>(offset);
		
	float alpha = radius * std::sqrt(2.0f / (3.0f + std::sqrt(5.0f)));

	float beta = radius * (1.0f + std::sqrt(6.0f / (3.0f + std::sqrt(5.0f)) -
		2.0f + 2.0f * std::sqrt(2.0f / (3.0f + std::sqrt(5.0f)))));
	
	vec3 dodec[20];
	
	dodec[0][0] = -alpha; dodec[0][1] = 0; dodec[0][2] = beta;
	dodec[1][0] = alpha; dodec[1][1] = 0; dodec[1][2] = beta;
	dodec[2][0] = -radius; dodec[2][1] = -radius; dodec[2][2] = -radius;
	dodec[3][0] = -radius; dodec[3][1] = -radius; dodec[3][2] = radius;
	dodec[4][0] = -radius; dodec[4][1] = radius; dodec[4][2] = -radius;
	dodec[5][0] = -radius; dodec[5][1] = radius; dodec[5][2] = radius;
	dodec[6][0] = radius; dodec[6][1] = -radius; dodec[6][2] = -radius;
	dodec[7][0] = radius; dodec[7][1] = -radius; dodec[7][2] = radius;
	dodec[8][0] = radius; dodec[8][1] = radius; dodec[8][2] = -radius;
	dodec[9][0] = radius; dodec[9][1] = radius; dodec[9][2] = radius;
	dodec[10][0] = beta; dodec[10][1] = alpha; dodec[10][2] = 0.0f;
	dodec[11][0] = beta; dodec[11][1] = -alpha; dodec[11][2] = 0.0f;
	dodec[12][0] = -beta; dodec[12][1] = alpha; dodec[12][2] = 0.0f;
	dodec[13][0] = -beta; dodec[13][1] = -alpha; dodec[13][2] = 0.0f;
	dodec[14][0] = -alpha; dodec[14][1] = 0.0f; dodec[14][2] = -beta;
	dodec[15][0] = alpha; dodec[15][1] = 0.0f; dodec[15][2] = -beta;
	dodec[16][0] = 0.0f; dodec[16][1] = beta; dodec[16][2] = alpha;
	dodec[17][0] = 0.0f; dodec[17][1] = beta; dodec[17][2] = -alpha;
	dodec[18][0] = 0.0f; dodec[18][1] = -beta; dodec[18][2] = alpha;
	dodec[19][0] = 0.0f; dodec[19][1] = -beta; dodec[19][2] = -alpha;
	
#define ET_PUSH_PENTAGON(v0, v1, v2, v3, v4) \
	{ \
		vec3 c = 0.2f * (dodec[v0] + dodec[v1] + dodec[v2] + dodec[v3] + dodec[v4]); \
		if (hasNormals) \
		{ \
			triangle t(dodec[v0], dodec[v1], dodec[v2]); \
			vec3 n = t.normalizedNormal(); \
			pos[k] = c; nrm[k++] = n; \
			pos[k] = dodec[v0]; nrm[k++] = n; \
			pos[k] = dodec[v1]; nrm[k++] = n; \
			pos[k] = c; nrm[k++] = n; \
			pos[k] = dodec[v1]; nrm[k++] = n; \
			pos[k] = dodec[v2]; nrm[k++] = n; \
			pos[k] = c; nrm[k++] = n; \
			pos[k] = dodec[v2]; nrm[k++] = n; \
			pos[k] = dodec[v3]; nrm[k++] = n; \
			pos[k] = c; nrm[k++] = n; \
			pos[k] = dodec[v3]; nrm[k++] = n; \
			pos[k] = dodec[v4]; nrm[k++] = n; \
			pos[k] = c; nrm[k++] = n; \
			pos[k] = dodec[v4]; nrm[k++] = n; \
			pos[k] = dodec[v0]; nrm[k++] = n; \
		} \
		else \
		{ \
			pos[k++] = c; \
			pos[k++] = dodec[v0];\
			pos[k++] = dodec[v1];\
			pos[k++] = c;\
			pos[k++] = dodec[v1];\
			pos[k++] = dodec[v2];\
			pos[k++] = c;\
			pos[k++] = dodec[v2];\
			pos[k++] = dodec[v3];\
			pos[k++] = c;\
			pos[k++] = dodec[v3];\
			pos[k++] = dodec[v4];\
			pos[k++] = c;\
			pos[k++] = dodec[v4];\
			pos[k++] = dodec[v0];\
		} \
	}
	
	uint32_t k = 0;
	ET_PUSH_PENTAGON(0, 1, 9, 16, 5)
	ET_PUSH_PENTAGON(1, 0, 3, 18, 7)
	ET_PUSH_PENTAGON(1, 7, 11, 10, 9)
	
	ET_PUSH_PENTAGON(11, 7, 18, 19, 6)
	ET_PUSH_PENTAGON(8, 17, 16, 9, 10)
	ET_PUSH_PENTAGON(2, 14, 15, 6, 19)
	
	ET_PUSH_PENTAGON(2, 13, 12, 4, 14)
	ET_PUSH_PENTAGON(2, 19, 18, 3, 13)
	ET_PUSH_PENTAGON(3, 0, 5, 12, 13)
	
	ET_PUSH_PENTAGON(6, 15, 8, 10, 11)
	ET_PUSH_PENTAGON(4, 17, 8, 15, 14)
	ET_PUSH_PENTAGON(4, 12, 5, 16, 17)
	
#undef ET_PUSH_PENTAGON
}

void primitives::createIcosahedron(VertexArray::Pointer data, float radius, bool top, bool middle, bool bottom)
{
	VertexDataChunk posChunk = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk nrmChunk = data->chunk(VertexAttributeUsage::Normal);
	bool hasPosition = posChunk.valid() && (posChunk->type() == DataType::Vec3);
	bool hasNormals = nrmChunk.valid() && (nrmChunk->type() == DataType::Vec3);

	ET_ASSERT(hasPosition);
	(void)hasPosition;
	
	uint32_t offset = data->size();
	data->fitToSize(15 * ((top ? 1 : 0) + (middle ? 2 : 0 ) + (bottom ? 1 : 0)));
	
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(offset);
	RawDataAcessor<vec3> nrm = nrmChunk.accessData<vec3>(offset);
		
	float u = radius;
	float v = radius * GOLDEN_RATIO;
	
	vec3 corners[12] =
	{
		vec3(-u,  v, 0.0f),
		vec3( u,  v, 0.0f),
		vec3(-u, -v, 0.0f),
		vec3( u, -v, 0.0f),
		vec3(0.0f, -u,  v),
		vec3(0.0f,  u,  v),
		vec3(0.0f, -u, -v),
		vec3(0.0f,  u, -v),
		vec3( v, 0.0f, -u),
		vec3( v, 0.0f,  u),
		vec3(-v, 0.0f, -u),
		vec3(-v, 0.0f,  u)
	};
	
	uint32_t k = 0;

	if (top)
	{
		ET_ADD_TRIANGLE(0, 11, 5);
		ET_ADD_TRIANGLE(0, 5, 1);
		ET_ADD_TRIANGLE(0, 1, 7);
		ET_ADD_TRIANGLE(0, 7, 10);
		ET_ADD_TRIANGLE(0, 10, 11);
	}

	if (middle)
	{
		ET_ADD_TRIANGLE(1, 5, 9);
		ET_ADD_TRIANGLE(4, 9, 5);
		ET_ADD_TRIANGLE(5, 11, 4);
		ET_ADD_TRIANGLE(2, 4, 11);
		ET_ADD_TRIANGLE(11, 10, 2);
		ET_ADD_TRIANGLE(6, 2, 10);
		ET_ADD_TRIANGLE(10, 7, 6);
		ET_ADD_TRIANGLE(8, 6, 7);
		ET_ADD_TRIANGLE(7, 1, 8);
		ET_ADD_TRIANGLE(9, 8, 1);
	}

	if (bottom)
	{
		ET_ADD_TRIANGLE(3, 9, 4);
		ET_ADD_TRIANGLE(3, 4, 2);
		ET_ADD_TRIANGLE(3, 2, 6);
		ET_ADD_TRIANGLE(3, 6, 8);
		ET_ADD_TRIANGLE(3, 8, 9);
	}

}

#undef ET_ADD_TRIANGLE

void primitives::tesselateTriangles(VertexArray::Pointer data, IndexArray::Pointer indexArray, const vec3& aspect)
{
	ET_ASSERT(indexArray->primitiveType() == PrimitiveType::Triangles);
		   
	bool hasPosition = data->chunk(VertexAttributeUsage::Position).valid() &&
		(data->chunk(VertexAttributeUsage::Position)->type() == DataType::Vec3);
	
	bool hasNormals = data->chunk(VertexAttributeUsage::Normal).valid() &&
		(data->chunk(VertexAttributeUsage::Normal)->type() == DataType::Vec3);

	bool hasColor = data->chunk(VertexAttributeUsage::Color).valid() &&
		(data->chunk(VertexAttributeUsage::Color)->type() == DataType::Vec4);
	
	ET_ASSERT(hasPosition);
	(void)hasPosition;
	
	VertexArray::Pointer oldData(data->duplicate());
	
	VertexDataChunk oldPos = oldData->chunk(VertexAttributeUsage::Position);
	VertexDataChunk oldNrm = oldData->chunk(VertexAttributeUsage::Normal);
	VertexDataChunk oldClr = oldData->chunk(VertexAttributeUsage::Color);
	VertexDataChunk newPos = data->chunk(VertexAttributeUsage::Position);
	VertexDataChunk newNrm = data->chunk(VertexAttributeUsage::Normal);
	VertexDataChunk newClr = data->chunk(VertexAttributeUsage::Color);
	
	uint32_t numTriangles = indexArray->primitivesCount();
	
	data->fitToSize(12 * numTriangles);
	
	RawDataAcessor<vec3> opos = oldPos.accessData<vec3>(0);
	RawDataAcessor<vec3> npos = newPos.accessData<vec3>(0);
	RawDataAcessor<vec3> onrm = oldNrm.accessData<vec3>(0);
	RawDataAcessor<vec3> nnrm = newNrm.accessData<vec3>(0);
	RawDataAcessor<vec4> oclr = oldClr.accessData<vec4>(0);
	RawDataAcessor<vec4> nclr = newClr.accessData<vec4>(0);
	
	uint32_t nump = 0;
	uint32_t numn = 0;
	uint32_t numc = 0;
	
	for (auto i = indexArray->begin(), e = indexArray->end(); i != e; ++i)
	{
		const vec3& a = opos[i[0]];
		const vec3& b = opos[i[1]];
		const vec3& c = opos[i[2]];
		
		vec3 u = mix(a, b, aspect.x);
		vec3 v = mix(b, c, aspect.y);
		vec3 w = mix(c, a, aspect.z);
		npos[nump++] = a; npos[nump++] = u; npos[nump++] = w;
		npos[nump++] = u; npos[nump++] = b; npos[nump++] = v;
		npos[nump++] = w; npos[nump++] = v; npos[nump++] = c;
		npos[nump++] = w; npos[nump++] = u; npos[nump++] = v;
		
		if (hasNormals)
		{
			const vec3& na = onrm[i[0]];
			const vec3& nb = onrm[i[1]];
			const vec3& nc = onrm[i[2]];
			
			vec3 nu = normalize(mix(na, nb, aspect.x));
			vec3 nv = normalize(mix(nb, nc, aspect.y));
			vec3 nw = normalize(mix(nc, na, aspect.z));
			
			nnrm[numn++] = na; nnrm[numn++] = nu; nnrm[numn++] = nw;
			nnrm[numn++] = nu; nnrm[numn++] = nb; nnrm[numn++] = nv;
			nnrm[numn++] = nw; nnrm[numn++] = nv; nnrm[numn++] = nc;
			nnrm[numn++] = nw; nnrm[numn++] = nu; nnrm[numn++] = nv;
		}
		
		if (hasColor)
		{
			const vec4& ca = oclr[i[0]];
			const vec4& cb = oclr[i[1]];
			const vec4& cc = oclr[i[2]];
			
			vec4 cu = mix(ca, cb, aspect.x);
			vec4 cv = mix(cb, cc, aspect.x);
			vec4 cw = mix(cc, ca, aspect.x);
			
			nclr[numc++] = ca; nclr[numc++] = cu; nclr[numc++] = cw;
			nclr[numc++] = cu; nclr[numc++] = cb; nclr[numc++] = cv;
			nclr[numc++] = cw; nclr[numc++] = cv; nclr[numc++] = cc;
			nclr[numc++] = cw; nclr[numc++] = cu; nclr[numc++] = cv;
		}
		
	}
}

void primitives::tesselateTriangles(VertexArray::Pointer data, const vec3& aspect)
{
	IndexArray::Pointer linearIndices = IndexArray::Pointer::create(IndexArrayFormat::Format_32bit,
		data->size(), PrimitiveType::Triangles);
	linearIndices->linearize(data->size());
	tesselateTriangles(data, linearIndices, aspect);
}

uint64_t primitives::vector3Hash(const vec3& v)
{
	char raw[256] = { };
	
	int64_t ix = static_cast<int64_t>(174763.0f * v.x);
	int64_t iy = static_cast<int64_t>(478441.0f * v.y);
	int64_t iz = static_cast<int64_t>(720743.0f * v.z);
	int symbols = sprintf(raw, "%lld%lld%lld%lld", ix, iy, iz, ix+iy+iz);
	
	uint64_t* ptr = reinterpret_cast<uint64_t*>(raw);
	uint64_t* end = ptr + symbols / sizeof(uint64_t) + 1;
	uint64_t result = *ptr++;
	
	while (ptr != end)
		result ^= *ptr++;
	
	return result;
}

VertexArray::Pointer primitives::buildLinearIndexArray(VertexArray::Pointer vertexArray, IndexArray::Pointer indexArray)
{
	/*
	 * Count unique vertices
	 */
	uint32_t dataSize = vertexArray->size();
	Map<uint64_t, uint32_t> countMap;
	Vector<uint64_t> hashes;
	hashes.reserve(vertexArray->size());
	
	auto oldPos = vertexArray->chunk(VertexAttributeUsage::Position).accessData<vec3>(0);
	auto oldNrm = vertexArray->chunk(VertexAttributeUsage::Normal).accessData<vec3>(0);
	auto oldClr = vertexArray->chunk(VertexAttributeUsage::Color).accessData<vec4>(0);
	
	for (uint32_t i = 0; i < dataSize; ++i)
	{
		uint64_t hash = vector3Hash(oldPos[i]);
		countMap[hash] = 0;
		hashes.push_back(hash);
	}
	
	indexArray->resizeToFit(dataSize);
	
	VertexArray::Pointer result = VertexArray::Pointer::create(vertexArray->decl(), static_cast<uint32_t>(countMap.size()));
	
	auto newPos = result->chunk(VertexAttributeUsage::Position).accessData<vec3>(0);
	auto newNrm = result->chunk(VertexAttributeUsage::Normal).accessData<vec3>(0);
	auto newClr = result->chunk(VertexAttributeUsage::Color).accessData<vec4>(0);
	
	bool hasNormals = newNrm.valid();
	bool hasColor = newClr.valid();
	
	std::map<uint64_t, uint32_t> indexMap;
	for (uint32_t i = 0; i < dataSize; ++i)
	{
		uint64_t hash = hashes[i];
		if (indexMap.count(hash) == 0)
		{
			uint32_t mapSize = static_cast<uint32_t>(indexMap.size());
			newPos[mapSize] = oldPos[i];
			
			if (hasNormals)
				newNrm[mapSize] = oldNrm[i];
			if (hasColor)
				newClr[mapSize] = oldClr[i];
			
			indexArray->setIndex(mapSize, i);
			indexMap[hash] = mapSize;
		}
		else
		{
			if (hasNormals)
				newNrm[indexMap[hash]] += oldNrm[i];
			
			if (hasColor)
				newClr[indexMap[hash]] = 0.5f * (newClr[indexMap[hash]] + oldClr[i]);
			
			indexArray->setIndex(static_cast<uint32_t>(indexMap[hash]), i);
		}
	}
	
	if (hasNormals)
	{
		for (uint32_t i = 0; i < newNrm.size(); ++i)
			newNrm[i].normalize();
	}
	
	return result;
}

VertexArray::Pointer primitives::linearizeTrianglesIndexArray(VertexArray::Pointer data, IndexArray::Pointer indexArray)
{
	ET_ASSERT(indexArray->primitiveType() == PrimitiveType::Triangles);
	
	VertexDeclaration decl = data->decl();
	VertexArray::Pointer result = VertexArray::Pointer::create(data->decl(), 3 * indexArray->primitivesCount());
	for (auto& e : decl.elements())
	{
		if (e.type() == DataType::Vec4)
		{
			auto oldValues = data->chunk(e.usage()).accessData<vec4>(0);
			auto newValues = result->chunk(e.usage()).accessData<vec4>(0);
			uint32_t i = 0;
			for (auto p : indexArray.reference())
			{
				newValues[i++] = oldValues[p[0]];
				newValues[i++] = oldValues[p[1]];
				newValues[i++] = oldValues[p[2]];
			}
		}
		else if (e.type() == DataType::Vec3)
		{
			auto oldValues = data->chunk(e.usage()).accessData<vec3>(0);
			auto newValues = result->chunk(e.usage()).accessData<vec3>(0);
			uint32_t i = 0;
			for (auto p : indexArray.reference())
			{
				newValues[i++] = oldValues[p[0]];
				newValues[i++] = oldValues[p[1]];
				newValues[i++] = oldValues[p[2]];
			}
		}
		else if (e.type() == DataType::Vec2)
		{
			auto oldValues = data->chunk(e.usage()).accessData<vec2>(0);
			auto newValues = result->chunk(e.usage()).accessData<vec2>(0);
			uint32_t i = 0;
			for (auto p : indexArray.reference())
			{
				newValues[i++] = oldValues[p[0]];
				newValues[i++] = oldValues[p[1]];
				newValues[i++] = oldValues[p[2]];
			}
		}
	}
	return result;
}

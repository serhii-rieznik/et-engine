/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/opengl/opengl.h>
#include <et/core/containers.h>
#include <et/primitives/primitives.h>

using namespace et;

inline int getIndex(int u, int v, int u_sz, int v_sz)
		{ return clamp<int>(u, 0, u_sz - 1) + clamp<int>(v, 0, v_sz - 1) * u_sz; }

IndexType primitives::indexCountForRegularMesh(const vec2i& meshSize, PrimitiveType geometryType)
{
	switch (geometryType)
	{
		case PrimitiveType_Points:
			return static_cast<IndexType>(meshSize.square());

		case PrimitiveType_Triangles:
			return static_cast<IndexType>(((meshSize.x > 1) ? meshSize.x - 1 : 1) * ((meshSize.y > 1) ? meshSize.y - 1 : 1) * 6);

		case PrimitiveType_TriangleStrips:
			return static_cast<IndexType>(((meshSize.y > 1) ? meshSize.y - 1 : 1) * (2 * meshSize.x + 1) - 1);
			
		default:
			assert("Unimplemented" && 0);
	}

	return 0;
}

void primitives::createPhotonMap(VertexArray::Pointer buffer, const vec2i& density)
{
	buffer->increase(static_cast<size_t>(density.square()));
	
	VertexDataChunk chunk = buffer->chunk(Usage_Position);
	assert(chunk->type() == Type_Vec2);
	
	RawDataAcessor<vec2> data = chunk.accessData<vec2>(0);
	
	vec2 texel = vec2(1.0f / density.x, 1.0f / density.y);
	vec2 dxdy = vec2(0.5f / density.x, 0.5f / density.y);

	size_t k = 0;
	for (int i = 0; i < density.y; ++i)
		for (int j = 0; j < density.x; ++j)
			data[k++] = vec2(vec2(j * texel.x, i * texel.y) + dxdy);
}

void primitives::createSphere(VertexArray::Pointer data, float radius, const vec2i& density,
	const vec3& center, const vec2& hemiSphere)
{
	size_t lastIndex = data->size();
	data->increase(static_cast<size_t>(density.square()));

	VertexDataChunk pos_c = data->chunk(Usage_Position);
	VertexDataChunk norm_c = data->chunk(Usage_Normal);
	VertexDataChunk tex_c = data->chunk(Usage_TexCoord0);
	RawDataAcessor<vec3> pos = pos_c.valid() ? pos_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> norm = norm_c.valid() ? norm_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec2> tex = tex_c.valid() ? tex_c.accessData<vec2>(lastIndex) : RawDataAcessor<vec2>();

	bool hasPos = pos.valid();
	bool hasNorm = norm.valid();
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
				norm[counter] = p;

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
	size_t lastIndex = data->size();
	data->increase(static_cast<size_t>(density.square()));

	VertexDataChunk pos_c = data->chunk(Usage_Position);
	VertexDataChunk norm_c = data->chunk(Usage_Normal);
	VertexDataChunk tex_c = data->chunk(Usage_TexCoord0);
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
			{
				vec3 n;
				n.x = std::cos(phi) * std::cos(theta);
				n.y = std::sin(theta);
				n.z = std::sin(phi) * std::cos(theta);
				norm[counter] = normalize(n);
			}

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
	size_t lastIndex = data->size();
	data->increase(static_cast<size_t>(density.square()));

	VertexDataChunk pos_c = data->chunk(Usage_Position);
	VertexDataChunk norm_c = data->chunk(Usage_Normal);
	VertexDataChunk tex_c = data->chunk(Usage_TexCoord0);
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
	size_t lastIndex = data->size();
	data->increase(static_cast<size_t>(density.square()));

	VertexDataChunk pos_c = data->chunk(Usage_Position);
	VertexDataChunk norm_c = data->chunk(Usage_Normal);
	VertexDataChunk tex_c = data->chunk(Usage_TexCoord0);
	RawDataAcessor<vec3> pos = pos_c.valid() ? pos_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> norm = norm_c.valid() ? norm_c.accessData<vec3>(lastIndex) : RawDataAcessor<vec3>();
	RawDataAcessor<vec2> tex = tex_c.valid() ? tex_c.accessData<vec2>(lastIndex) : RawDataAcessor<vec2>();

	bool hasPos = pos.valid();
	bool hasNorm = norm.valid();
	bool hasTex = tex.valid();

	vec3 angles = toSpherical(normal);
	angles += vec3(-HALF_PI, HALF_PI, 0.0);

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

	size_t counter = 0;
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
	size_t density, const vec3& center, const vec2& texCoordScale, const vec2& texCoordOffset)
{
	IndexArray::Pointer result = IndexArray::Pointer::create(IndexArrayFormat_16bit, 3 * density, PrimitiveType_Triangles);
	
	data->fitToSize(1 + density);
	auto pos = data->chunk(Usage_Position).accessData<vec3>(0);
	auto nrm = data->chunk(Usage_Normal).accessData<vec3>(0);
	
	vec3 angles = toSpherical(normal);
	angles += vec3(-HALF_PI, HALF_PI, 0.0);
	
	vec3 normalized = normalize(normal);
	vec3 o2 = fromSpherical(angles.y, angles.x);
	vec3 o1 = cross(normalized, o2);
	o2 = cross(o1, normalized);
	
	pos[0] = center;
	nrm[0] = normal;
	
	size_t index = 0;
	float angle = 0.0f;
	float da = DOUBLE_PI / (static_cast<float>(density - 1));
	for (size_t i = 1; i < density; ++i)
	{
		pos[i] = center + (o1 * cos(angle) - o2 * sin(angle)) * radius;
		nrm[i] = normal;
		result->setIndex(0, index++);
		result->setIndex(static_cast<IndexType>(i), index++);
		result->setIndex((i + 1 < density) ? static_cast<IndexType>(i + 1) : 1, index++);
		angle += da;
	}
		
	return result;
}

IndexType primitives::buildTriangleStripIndexes(IndexArray& buffer, const vec2i& dim,
	IndexType index0, IndexType offset)
{
	IndexType k = offset;
	IndexType rowSize = static_cast<IndexType>(dim.x);
	IndexType colSize = static_cast<IndexType>(dim.y);

	for (IndexType v = 0; v < static_cast<IndexType>(dim.y - 1); ++v)
	{
		IndexType thisRow = index0 + v * rowSize;
		IndexType nextRow = thisRow + rowSize;

		if (v % 2 == 0)
		{
			for (IndexType u = 0; u < rowSize; ++u)
			{
				buffer.setIndex(u + thisRow, k++);
				buffer.setIndex(u + nextRow, k++);
			}
			if (v != colSize - 2)
				buffer.setIndex((rowSize - 1) + nextRow, k++);
		}
		else
		{
			for (IndexType u = rowSize - 1; ; --u)
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

IndexType primitives::buildTrianglesIndexes(IndexArray& buffer, const vec2i& dim,
	IndexType vertexOffset, IndexType indexOffset)
{
	IndexType k = indexOffset;
	IndexType rowSize = static_cast<IndexType>(dim.x);
	IndexType colSize = static_cast<IndexType>(dim.y);

	for (IndexType i = 0; i < colSize - 1; ++i)
	{
		for (IndexType j = 0; j < rowSize - 1; ++j)
		{
			IndexType value = vertexOffset + j + i * rowSize;

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

IndexType primitives::buildTrianglesIndexes(IndexArray::Pointer buffer, const vec2i& dim,
	IndexType vertexOffset, IndexType indexOffset)
{
	assert(buffer.valid());
	return buildTrianglesIndexes(buffer.reference(), dim, vertexOffset, indexOffset);
}

IndexType primitives::buildTriangleStripIndexes(IndexArray::Pointer buffer, const vec2i& dim,
	IndexType vertexOffset, IndexType indexOffset)
{
	assert(buffer.valid());
	return buildTriangleStripIndexes(buffer.reference(), dim, vertexOffset, indexOffset);
}

void primitives::calculateNormals(VertexArray::Pointer data, const IndexArray::Pointer& buffer,
	IndexType first, IndexType last)
{
	assert(first < last);

	VertexDataChunk posChunk = data->chunk(Usage_Position);
	VertexDataChunk nrmChunk = data->chunk(Usage_Normal);

	if (posChunk.invalid() || (posChunk->type() != Type_Vec3) || !nrmChunk.valid() ||
		(nrmChunk->type() != Type_Vec3))
	{
		log::error("primitives::calculateNormals - data is invalid.");
		return;
	}
	
	RawDataAcessor<vec3> nrm = nrmChunk.accessData<vec3>(0);
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(0);
	
	for (auto i = buffer->primitive(first), e = buffer->primitive(last); i != e; ++i)
	{
		auto p = (*i);
		nrm[p[0]] = vec3(0.0f);
		nrm[p[1]] = vec3(0.0f);
		nrm[p[2]] = vec3(0.0f);
	}

	for (auto i = buffer->primitive(first), e = buffer->primitive(last); i != e; ++i)
	{
		auto p = (*i);
		triangle t(pos[p[0]], pos[p[1]], pos[p[2]]);
		vec3 n = t.normalizedNormal() * t.square();
		nrm[p[0]] += n;
		nrm[p[1]] += n;
		nrm[p[2]] += n;
	}

	for (auto i = buffer->primitive(first), e = buffer->primitive(last); i != e; ++i)
	{
		auto p = (*i);
		nrm[p[0]].normalize();
		nrm[p[1]].normalize();
		nrm[p[2]].normalize();
	}
}

void primitives::calculateTangents(VertexArray::Pointer data, const IndexArray::Pointer& buffer,
	IndexType first, IndexType last)
{
	assert(first < last);
	
	VertexDataChunk posChunk = data->chunk(Usage_Position);
	VertexDataChunk nrmChunk = data->chunk(Usage_Normal);
	VertexDataChunk uvChunk = data->chunk(Usage_TexCoord0);
	VertexDataChunk tanChunk = data->chunk(Usage_Tangent);
	
	if (posChunk.invalid() || (posChunk->type() != Type_Vec3) ||
		nrmChunk.invalid() || (nrmChunk->type() != Type_Vec3) ||
		tanChunk.invalid() || (tanChunk->type() != Type_Vec3) ||
		uvChunk.invalid() || (uvChunk->type() != Type_Vec2))
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
		for (size_t k = 0; k < 3; ++k)
		{
			vec3& n = nrm[p[k]];
			vec3& t = tan1[p[k]]; 
			float value = dot(cross(n, t), tan2[p[k]]);
			tan[p[k]] = normalize(t - n * dot(n, t)) * signOrZero(value);
		}
	}
}

void primitives::smoothTangents(VertexArray::Pointer data, const IndexArray::Pointer&,
	IndexType first, IndexType last)
{
	assert(first < last);
	
	VertexDataChunk posChunk = data->chunk(Usage_Position);
	VertexDataChunk tanChunk = data->chunk(Usage_Tangent);
	
	if (posChunk.invalid() || (posChunk->type() != Type_Vec3) || !tanChunk.valid() ||
		(tanChunk->type() != Type_Vec3))
	{
		log::error("primitives::smoothTangents - data is invalid.");
		return;
	}

	size_t len = last - first;
	DataStorage<vec3> tanSmooth(data->size());
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(first);
	RawDataAcessor<vec3> tan = tanChunk.accessData<vec3>(first);
	for (size_t p = 0; p < len; ++p)
	{
		tanSmooth[p] = tan[p];
		for (size_t i = p + 1; i < len; ++i)
		{
			if ((pos[p] - pos[i]).dotSelf() <= 1.0e-3f)
				tanSmooth[p] += tan[i];
		}
	}

	for (size_t i = 0; i < len; ++i)
		tan[i] = normalize(tanSmooth[i]);
}

void primitives::createIcosahedron(VertexArray::Pointer data, float radius)
{
	VertexDataChunk posChunk = data->chunk(Usage_Position);
	VertexDataChunk nrmChunk = data->chunk(Usage_Normal);
	bool hasPosition = posChunk.valid() && (posChunk->type() == Type_Vec3);
	bool hasNormals = nrmChunk.valid() && (nrmChunk->type() == Type_Vec3);

	assert(hasPosition);
	(void)hasPosition;
	
	size_t offset = data->size();
	data->fitToSize(60);
	
	RawDataAcessor<vec3> nrm;
	RawDataAcessor<vec3> pos = posChunk.accessData<vec3>(offset);
	for (size_t i = 0; i < pos.size(); ++i)
		pos[i] = vec3(0.0f);
	
	if (hasNormals)
		nrm = nrmChunk.accessData<vec3>(offset);
		
	float u = radius;
	float v = radius * (1.0f + std::sqrt(5.0f)) / 2.0f;
	
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
	
	size_t k = 0;
	
#define ET_ADD_IH_TRIANGLE(c1, c2, c3) { \
		pos[k] = corners[c1]; pos[k+1] = corners[c2]; pos[k+2] = corners[c3]; \
		if (hasNormals) nrm[k+0] = nrm[k+1] = nrm[k+2] = \
			triangle(corners[c1], corners[c2], corners[c3]).normalizedNormal(); \
		k += 3; } \

	// top cap
	ET_ADD_IH_TRIANGLE(0, 11, 5);
	ET_ADD_IH_TRIANGLE(0, 5, 1);
	ET_ADD_IH_TRIANGLE(0, 1, 7);
	ET_ADD_IH_TRIANGLE(0, 7, 10);
	ET_ADD_IH_TRIANGLE(0, 10, 11);

	// middle
	ET_ADD_IH_TRIANGLE(1, 5, 9);
	ET_ADD_IH_TRIANGLE(4, 9, 5);
	ET_ADD_IH_TRIANGLE(5, 11, 4);
	ET_ADD_IH_TRIANGLE(2, 4, 11);
	ET_ADD_IH_TRIANGLE(11, 10, 2);
	ET_ADD_IH_TRIANGLE(6, 2, 10);
	ET_ADD_IH_TRIANGLE(10, 7, 6);
	ET_ADD_IH_TRIANGLE(8, 6, 7);
	ET_ADD_IH_TRIANGLE(7, 1, 8);
	ET_ADD_IH_TRIANGLE(9, 8, 1);

	// bottom cap
	ET_ADD_IH_TRIANGLE(3, 9, 4);
	ET_ADD_IH_TRIANGLE(3, 4, 2);
	ET_ADD_IH_TRIANGLE(3, 2, 6);
	ET_ADD_IH_TRIANGLE(3, 6, 8);
	ET_ADD_IH_TRIANGLE(3, 8, 9);

#undef ET_ADD_IH_TRIANGLE
}

void primitives::tesselateTriangles(VertexArray::Pointer data)
{
	assert((data->size() % 3 == 0) && "VertexArray should have integer number of triangles.");
	
	bool hasPosition = data->chunk(Usage_Position).valid() &&
		(data->chunk(Usage_Position)->type() == Type_Vec3);
	
	bool hasNormals = data->chunk(Usage_Normal).valid() &&
		(data->chunk(Usage_Normal)->type() == Type_Vec3);
	
	assert(hasPosition);
	(void)hasPosition;
	
	VertexArray::Pointer oldData(data->duplicate());
	
	VertexDataChunk oldPos = oldData->chunk(Usage_Position);
	VertexDataChunk oldNrm = oldData->chunk(Usage_Normal);
	VertexDataChunk newPos = data->chunk(Usage_Position);
	VertexDataChunk newNrm = data->chunk(Usage_Normal);
	
	size_t numVertices = data->size();
	size_t numTriangles = numVertices / 3;

	data->fitToSize(12 * numTriangles);
	RawDataAcessor<vec3> opos = oldPos.accessData<vec3>(0);
	RawDataAcessor<vec3> npos = newPos.accessData<vec3>(0);
	RawDataAcessor<vec3> onrm = hasNormals ? oldNrm.accessData<vec3>(0) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> nnrm = hasNormals ? newNrm.accessData<vec3>(0) : RawDataAcessor<vec3>();
	
	size_t i = 0;
	size_t np = 0;
	size_t nn = 0;
	while (i < numVertices)
	{
		const vec3& a = opos[i++];
		const vec3& b = opos[i++];
		const vec3& c = opos[i++];
		vec3 u = 0.5f * (a + b);
		vec3 v = 0.5f * (b + c);
		vec3 w = 0.5f * (c + a);
		npos[np++] = a; npos[np++] = u; npos[np++] = w;
		npos[np++] = u; npos[np++] = b; npos[np++] = v;
		npos[np++] = w; npos[np++] = v; npos[np++] = c;
		npos[np++] = w; npos[np++] = u; npos[np++] = v;
		if (hasNormals)
		{
			const vec3& na = onrm[i-3];
			const vec3& nb = onrm[i-2];
			const vec3& nc = onrm[i-1];
			vec3 nu = normalize(na + nb);
			vec3 nv = normalize(nb + nc);
			vec3 nw = normalize(nc + na);
			nnrm[nn++] = na; nnrm[nn++] = nu; nnrm[nn++] = nw;
			nnrm[nn++] = nu; nnrm[nn++] = nb; nnrm[nn++] = nv;
			nnrm[nn++] = nw; nnrm[nn++] = nv; nnrm[nn++] = nc;
			nnrm[nn++] = nw; nnrm[nn++] = nu; nnrm[nn++] = nv;
		}
	}
}

void primitives::tesselateTriangles(VertexArray::Pointer data, IndexArray::Pointer indexArray)
{
	assert(indexArray->primitiveType() == PrimitiveType_Triangles);
		   
	bool hasPosition = data->chunk(Usage_Position).valid() &&
	(data->chunk(Usage_Position)->type() == Type_Vec3);
	
	bool hasNormals = data->chunk(Usage_Normal).valid() &&
	(data->chunk(Usage_Normal)->type() == Type_Vec3);
	
	assert(hasPosition);
	(void)hasPosition;
	
	VertexArray::Pointer oldData(data->duplicate());
	
	VertexDataChunk oldPos = oldData->chunk(Usage_Position);
	VertexDataChunk oldNrm = oldData->chunk(Usage_Normal);
	VertexDataChunk newPos = data->chunk(Usage_Position);
	VertexDataChunk newNrm = data->chunk(Usage_Normal);
	
	size_t numTriangles = indexArray->primitivesCount();
	
	data->fitToSize(12 * numTriangles);
	RawDataAcessor<vec3> opos = oldPos.accessData<vec3>(0);
	RawDataAcessor<vec3> npos = newPos.accessData<vec3>(0);
	RawDataAcessor<vec3> onrm = hasNormals ? oldNrm.accessData<vec3>(0) : RawDataAcessor<vec3>();
	RawDataAcessor<vec3> nnrm = hasNormals ? newNrm.accessData<vec3>(0) : RawDataAcessor<vec3>();
	
	size_t np = 0;
	size_t nn = 0;
	for (auto i = indexArray->begin(), e = indexArray->end(); i != e; ++i)
	{
		const vec3& a = opos[i[0]];
		const vec3& b = opos[i[1]];
		const vec3& c = opos[i[2]];
		vec3 u = 0.5f * (a + b);
		vec3 v = 0.5f * (b + c);
		vec3 w = 0.5f * (c + a);
		npos[np++] = a; npos[np++] = u; npos[np++] = w;
		npos[np++] = u; npos[np++] = b; npos[np++] = v;
		npos[np++] = w; npos[np++] = v; npos[np++] = c;
		npos[np++] = w; npos[np++] = u; npos[np++] = v;
		if (hasNormals)
		{
			const vec3& na = onrm[i[0]];
			const vec3& nb = onrm[i[1]];
			const vec3& nc = onrm[i[2]];
			vec3 nu = normalize(na + nb);
			vec3 nv = normalize(nb + nc);
			vec3 nw = normalize(nc + na);
			nnrm[nn++] = na; nnrm[nn++] = nu; nnrm[nn++] = nw;
			nnrm[nn++] = nu; nnrm[nn++] = nb; nnrm[nn++] = nv;
			nnrm[nn++] = nw; nnrm[nn++] = nv; nnrm[nn++] = nc;
			nnrm[nn++] = nw; nnrm[nn++] = nu; nnrm[nn++] = nv;
		}
	}
}

uint64_t vectorHash(const vec3& v)
{
	char raw[256] = { };
	int symbols = sprintf(raw, "%.3f_%0.3f_%0.3f_%0.3f_%0.3f_%0.3f", v.x, v.y, v.z, v.x * v.y, v.x * v.z, v.y * v.z);
	
	uint64_t* ptr = reinterpret_cast<uint64_t*>(raw);
	uint64_t* end = ptr + symbols / sizeof(uint64_t) + 1;
	uint64_t result = *ptr++;
	while (ptr != end) result ^= *ptr++;
	return result;
}

VertexArray::Pointer primitives::buildIndexArray(VertexArray::Pointer data, IndexArray::Pointer indexArray)
{
	/*
	 * Count unique vertices
	 */
	size_t dataSize = data->size();
	std::map<uint64_t, size_t> countMap;
	std::vector<uint64_t> hashes;
	hashes.reserve(data->size());
	
	auto oldPos = data->chunk(Usage_Position).accessData<vec3>(0);
	for (size_t i = 0; i < dataSize; ++i)
	{
		uint64_t hash = vectorHash(oldPos[i]);
		countMap[hash] = 0;
		hashes.push_back(hash);
	}
	
	indexArray->resizeToFit(dataSize);
	
	VertexArray::Pointer result(new VertexArray(data->decl(), countMap.size()));
	auto newPos = result->chunk(Usage_Position).accessData<vec3>(0);

	std::map<uint64_t, size_t> indexMap;
	for (size_t i = 0; i < dataSize; ++i)
	{
		uint64_t hash = hashes[i];
		if (indexMap.count(hash) == 0)
		{
			newPos[indexMap.size()] = oldPos[i];
			indexArray->setIndex(static_cast<IndexType>(indexMap.size()), i);
			indexMap[hash] = indexMap.size();
		}
		else
		{
			indexArray->setIndex(static_cast<IndexType>(indexMap[hash]), i);
		}
	}
	
	return result;
}

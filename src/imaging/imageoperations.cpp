/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <algorithm>

#include <et/imaging/imageoperations.h>

using namespace et;

mat3i ImageOperations::matrixFilterBlur = mat3i(
	0, 1, 0,
	1, 2, 1,
	0, 1, 0 );

mat3i ImageOperations::matrixFilterStrongBlur = mat3i(
	1, 2, 1,
	2, 4, 2,
	1, 2, 1 );

mat3i ImageOperations::matrixFilterSharpen = mat3i(
	 0, -1,  0,
	-1,  5, -1,
	 0, -1,  0);

int indexForCoord(const vec2i& coord, const vec2i& size);
bool grayscaleSortFunction(const vec4ub& v1, const vec4ub& v2);

inline int roundf(float v, int minV, int maxV)
	{ return clamp(static_cast<int>(v), minV, maxV); }

void ImageOperations::transfer(const BinaryDataStorage& src, const vec2i& srcSize, int srcComponents,
	BinaryDataStorage& dst, const vec2i& dstSize, int dstComponents, const vec2i& position)
{
	int startX = clamp(position.x, 0, dstSize.x - 1);
	int startY = clamp(position.y, 0, dstSize.y - 1);			
	int endX = clamp(position.x + srcSize.x, 0, dstSize.x - 1);	
	int endY = clamp(position.y + srcSize.y, 0, dstSize.y - 1);	
	for (int y = startY, srcY = 0; y < endY; ++y, ++srcY)
	{
		for (int x = startX, srcX = 0; x < endX; ++x, ++srcX)
		{
			int dstIndex = dstComponents * (x + (dstSize.y - y - 1) * dstSize.x);
			int srcIndex = srcComponents * (srcX + (srcSize.y - srcY - 1) * srcSize.x);
			for (int k = 0; k < dstComponents; ++k)
				dst[dstIndex + k] = (k < srcComponents) ? src[srcIndex + k] : 255;
		}
	}
}

void ImageOperations::draw(const BinaryDataStorage& src, const vec2i& srcSize, int srcComponents,
			BinaryDataStorage& dst, const vec2i& dstSize, int dstComponents, const recti& destRect,
			ImageBlendType blend, ImageFilteringType)
{
	int startX = clamp(destRect.left, 0, dstSize.x);
	int startY = clamp(destRect.top, 0, dstSize.y);
	int endX = clamp(destRect.left + destRect.width, 0, dstSize.x);
	int endY = clamp(destRect.top + destRect.height, 0, dstSize.y);

	float fWidth = static_cast<float>(destRect.width);
	float fHeight = static_cast<float>(destRect.height);
	
	for (int y = startY; y < endY; ++y)
	{
		float fV = static_cast<float>(y - startY) / fHeight;
		int v = roundf(fV * srcSize.y, 0, srcSize.y - 1);
		int nextV = roundf(fV * srcSize.y + 1.0f, 0, srcSize.y - 1);
		float dv = fV * srcSize.y - static_cast<float>(v);

		for (int x = startX; x < endX; ++x)
		{
			float fU = static_cast<float>(x - startX) / fWidth;
			int u = roundf(fU * srcSize.x, 0, srcSize.x - 1);
			int nextU = roundf(fU * srcSize.x + 1.0f, 0, srcSize.x - 1);
			float du = fU * srcSize.x - static_cast<float>(u);

			int index0 = srcComponents * (u + (srcSize.y - v - 1) * srcSize.x);
			int indexNextU = srcComponents * (nextU + (srcSize.y - v - 1) * srcSize.x);
			int indexNextV = srcComponents * (u + (srcSize.y - nextV - 1) * srcSize.x);
			int indexNextUV = srcComponents * (nextU + (srcSize.y - nextV - 1) * srcSize.x);

			vec4ub color(0);
			for (int c = 0; c < srcComponents; ++c)
			{
				unsigned char topInterpolation = static_cast<unsigned char>(
					static_cast<float>(src[index0 + c]) * (1.0f - du) +  static_cast<float>(src[indexNextU + c]) * du);
				unsigned char bottomInterpolation = static_cast<unsigned char>(
					static_cast<float>(src[indexNextV + c]) * (1.0f - du) +  static_cast<float>(src[indexNextUV + c]) * du);
				color[c] =  static_cast<unsigned char>(
					static_cast<float>(topInterpolation) * (1.0f - dv) +  static_cast<float>(bottomInterpolation) * dv);
			}

			if (srcComponents < 4)
				color.w = 255;

			int dstIndex = dstComponents * (x + (dstSize.y - 1 - y) * dstSize.x);
			for (int c = 0; c < dstComponents; ++c)
			{
				if (blend == ImageBlendType_Additive)
				{
					dst[dstIndex + c] = static_cast<unsigned char>(clamp(dst[dstIndex + c] + color[c] * color.w / 255, 0, 255));
				}
				else if (blend == ImageBlendType_Default)
				{
					dst[dstIndex + c] = static_cast<unsigned char>((dst[dstIndex + c] * (255 - color.w) + color[c] * color.w) / 255);
				}
			}

		}
	}	
}

void ImageOperations::fill(BinaryDataStorage& dst, const vec2i& dstSize, int dstComponents, const recti& r, const vec4ub& color)
{
	int startX = clamp(r.left, 0, dstSize.x - 1);
	int startY = clamp(r.top, 0, dstSize.y - 1);
	int endY = clamp(r.bottom(), 0, dstSize.y - 1);
	int endX = clamp(r.right(), 0, dstSize.x - 1);

	for (int y = startY; y < endY; ++y)
	{
		for (int x = startX; x < endX; ++x)
		{
			int dstIndex = dstComponents * (x + (dstSize.y - 1 - y) * dstSize.x);
			for (int k = 0; k < dstComponents; ++k)
				dst[dstIndex + k] = color[k];
		}
	}
}

void ImageOperations::applyPixelFilter(BinaryDataStorage& data, const vec2i& size, int components, PixelFilter* filter, void* context)
{
	int k = 0;
	for (int y = 0; y < size.y; ++y)
	{
		for (int x = 0; x < size.x; ++x)
		{
			vec4ub pixel(0);

			for (int c = 0; c < components; ++c)
				pixel[c] = data[k + c];

			filter->applyRGBA(pixel, context);

			for (int c = 0; c < components; ++c)
				data[k + c] = pixel[c];

			k += components;
		}
	}
}

void ImageOperations::blur(BinaryDataStorage& data, const vec2i& size, int components, vec2i direction, int radius, ImageBlurType type)
{
	type = ImageBlurType_Average;

	BinaryDataStorage source(data);
	for (int y = 0; y < size.y; ++y)
	{
		for (int x = 0; x < size.x; ++x)
		{
			int i0 = components * indexForCoord(vec2i(x, y), size);

			vec4i sum;
			int scale = (type == ImageBlurType_Average) ? 1 : (radius + 1);
			int totalScale = scale;
			for (int c = 0; c < components; ++c)
				sum[c] = source[i0 + c] * scale;

			for (int r = 1; r <= radius; ++r)
			{
				vec2i vNext = vec2i(x, y) + direction * r;
				vec2i vPrev = vec2i(x, y) - direction * r;
				int iNext = components * indexForCoord(vNext, size);
				int iPrev = components * indexForCoord(vPrev, size);
				scale = (type == ImageBlurType_Average) ? 1 : (radius + 1 - r);
				totalScale += 2 * scale;
				for (int c = 0; c < components; ++c)
				{
					sum[c] += source[iNext + c] * scale;
					sum[c] += source[iPrev + c] * scale;
				}

			}

			sum /= totalScale;
			for (int c = 0; c < components; ++c)
				data[i0 + c] = static_cast<unsigned char>(sum[c]);
		}
	}
}

void ImageOperations::median(BinaryDataStorage& data, const vec2i& size, int components, int radius)
{
	BinaryDataStorage source(data);

	std::vector<vec4ub> matrix;
	matrix.reserve(static_cast<std::vector<vec4ub>::size_type>((1 + 2 * radius) * (1 + 2 * radius)));

	for (int y = 0; y < size.y; ++y)
	{
		for (int x = 0; x < size.x; ++x)
		{
			int i0 = components * indexForCoord(vec2i(x, y), size);

			matrix.clear();
			for (int v = -radius; v <= radius; ++v)
			{
				for (int u = - radius; u <= radius; ++u)
				{
					int index = components * indexForCoord(vec2i(x + u, y + v), size);
					vec4ub color(0);
					for (int c = 0; c < components; ++c)
						color[c] = source[index+c];
					matrix.push_back(color);
				}
			}

			std::sort(matrix.begin(), matrix.end(), grayscaleSortFunction);

			vec4ub& middle = matrix.at(matrix.size() / 2);
			for (int c = 0; c < components; ++c)
				data[i0 + c] = middle[c];
		}
	}
}

void ImageOperations::applyMatrixFilter(BinaryDataStorage& data, const vec2i& size, int components, const mat3i& m)
{
	BinaryDataStorage source(data);
	matrix3<unsigned char> colorMatrix[4];

	for (int y = 0; y < size.y; ++y)
	{
		for (int x = 0; x < size.x; ++x)
		{
			for (int dy = -1; dy <= 1; ++dy)
			{
				for (int dx = -1; dx <= 1; ++dx)
				{
					int index = components * indexForCoord(vec2i(x + dx, y + dy), size);
					for (int c = 0; c < components; ++c)
						colorMatrix[c][dy+1][dx+1] = source[index+c];
				}
			}

			vec4i result(0);
			int cSum = 0;
			for (int v = 0; v < 3; ++v)
			{
				for (int u = 0; u < 3; ++u)
				{
					int value = m[v][u];
					result.x += colorMatrix[0][v][u] * value;
					result.y += colorMatrix[1][v][u] * value;
					result.z += colorMatrix[2][v][u] * value;
					result.w += colorMatrix[3][v][u] * value;
					cSum += value;
				}
			}

			if (cSum)
				result /= cSum;

			int i0 = components * indexForCoord(vec2i(x, y), size);
			for (int c = 0; c < components; ++c)
				data[i0 + c] = static_cast<unsigned char>(clamp(result[c], 0, 255));
		}
	}
}

void ImageOperations::normalMapFilter(BinaryDataStorage& data, const vec2i& size, int components, const vec2& scale)
{
	assert(components > 2);

	vec2 fScale = scale / 255.0f;
	BinaryDataStorage source(data);
	for (int y = 0; y < size.y; ++y)
	{
		bool halfY = y < size.y / 2;
		for (int x = 0; x < size.x; ++x)
		{
			bool halfX = x < size.x / 2;
			vec2i nextX(x + (halfX ? 1 : -1), y);
			vec2i nextY(x, y + (halfY ? 1 : -1));

			int c00 = components * indexForCoord(vec2i(x, y), size);
			int c01 = components * indexForCoord(nextX, size);
			int c10 = components * indexForCoord(nextY, size);

			short h00 = source[c00];
			short h01 = source[c01];
			short h10 = source[c10];

			float dx = static_cast<float>(halfX ? h01 - h00 : h00 - h01) * fScale.x;
			float dy = static_cast<float>(halfY ? h10 - h00 : h00 - h10) * fScale.y;

			vec3 du(1.0f, 0.0f, dx);
			vec3 dv(0.0f, 1.0f, dy);

			vec3 produce = normalize(cross(du, dv));

			vec3ub result = vec3fto3ubscaled(produce);
			data[c00+0] = result.x;
			data[c00+1] = result.y;
			data[c00+2] = result.z;
		}
	}
}

/*
 * Internal Stuff
 */
int indexForCoord(const vec2i& coord, const vec2i& size)
{
	int xVal = coord.x < 0 ? 0 : (coord.x >= size.x ? size.x - 1 : coord.x);
	int yVal = coord.y < 0 ? 0 : (coord.y >= size.y ? size.y - 1 : coord.y);
	return yVal * size.x + xVal;
}

bool grayscaleSortFunction(const vec4ub& v1, const vec4ub& v2)
{
	int g1 = 76 * v1.x + 150 * v1.y + 29 * v1.z;
	int g2 = 76 * v2.x + 150 * v2.y + 29 * v2.z;
	return g1 < g2;
}

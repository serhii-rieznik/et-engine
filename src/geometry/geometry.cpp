/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <algorithm>
#include <et/geometry/geometry.h>

using namespace et;

static const float fRandMax = static_cast<float>(RAND_MAX);

const vec3 et::unitX(1.0f, 0.0f, 0.0f);
const vec3 et::unitY(0.0f, 1.0f, 0.0f);
const vec3 et::unitZ(0.0f, 0.0f, 1.0f);

const mat3 et::identityMatrix3(vec3(1.0f, 0.0f, 0.0f),
							   vec3(0.0f, 1.0f, 0.0f),
							   vec3(0.0f, 0.0f, 1.0f));

const mat4 et::identityMatrix(vec4(1.0f, 0.0f, 0.0f, 0.0f),
							  vec4(0.0f, 1.0f, 0.0f, 0.0f),
							  vec4(0.0f, 0.0f, 1.0f, 0.0f),
							  vec4(0.0f, 0.0f, 0.0f, 1.0f));

const mat4 et::lightProjectionMatrix(vec4(0.5f, 0.0f, 0.0f, 0.0f),
									 vec4(0.0f, 0.5f, 0.0f, 0.0f),
									 vec4(0.0f, 0.0f, 0.5f, 0.0f),
									 vec4(0.5f, 0.5f, 0.5f, 1.0f));

float et::signOrZero(float s)
{
	return (s == 0.0f) ? 0.0f : s / std::abs(s);
}

float et::signNoZero(float s)
{
	return (s < 0.0f) ? -1.0f : 1.0f; 
}

uint32_t et::randomInteger(uint32_t limit)
{
#if (ET_PLATFORM_APPLE)
	return arc4random() % limit;
#else
	return rand() % limit;
#endif
}

vec3 et::randVector(float sx, float sy, float sz)
{
	return vec3(sx * randomFloat(-1.0f, 1.0f), sy * randomFloat(-1.0f, 1.0f), sz * randomFloat(-1.0f, 1.0f));
}

float et::randomFloat(float low, float up)
{
	return low + (up - low) * static_cast<float>(randomInteger()) / fRandMax;
}

quaternion et::matrixToQuaternion(const mat3& r)
{
	float q0 = ( r[0][0] + r[1][1] + r[2][2] + 1.0f) / 4.0f;
	float q1 = ( r[0][0] - r[1][1] - r[2][2] + 1.0f) / 4.0f;
	float q2 = (-r[0][0] + r[1][1] - r[2][2] + 1.0f) / 4.0f;
	float q3 = (-r[0][0] - r[1][1] + r[2][2] + 1.0f) / 4.0f;

	q0 = (q0 < 0.0f) ? 0.0f : std::sqrt(q0);
	q1 = (q1 < 0.0f) ? 0.0f : std::sqrt(q1);
	q2 = (q2 < 0.0f) ? 0.0f : std::sqrt(q2);
	q3 = (q3 < 0.0f) ? 0.0f : std::sqrt(q3);

	if ((q0 >= q1) && (q0 >= q2) && (q0 >= q3))
	{
		q1 *= signNoZero(r[2][1] - r[1][2]);
		q2 *= signNoZero(r[0][2] - r[2][0]);
		q3 *= signNoZero(r[1][0] - r[0][1]);
	}
	else if ((q1 >= q0) && (q1 >= q2) && (q1 >= q3))
	{
		q0 *= signNoZero(r[2][1] - r[1][2]);
		q2 *= signNoZero(r[1][0] + r[0][1]);
		q3 *= signNoZero(r[0][2] + r[2][0]);
	}
	else if ((q2 >= q0) && (q2 >= q1) && (q2 >= q3))
	{
		q0 *= signNoZero(r[0][2] - r[2][0]);
		q1 *= signNoZero(r[1][0] + r[0][1]);
		q3 *= signNoZero(r[2][1] + r[1][2]);
	}
	else if ((q3 >= q0) && (q3 >= q1) && (q3 >= q2))
	{
		q0 *= signNoZero(r[1][0] - r[0][1]);
		q1 *= signNoZero(r[2][0] + r[0][2]);
		q2 *= signNoZero(r[2][1] + r[1][2]);
	}
	else
	{
		ET_FAIL("Unable to convert matrix to quaternion");
	}

	return normalize(quaternion(q0, q1, q2, q3));
}

quaternion et::matrixToQuaternion(const mat4& r)
{
	return matrixToQuaternion(r.mat3());
}

void et::decomposeMatrix(mat4 mat, vec3& translation, quaternion& rotation, vec3& scale)
{
	mat3 rot = mat.mat3();
	translation = mat[3].xyz();
	scale = removeMatrixScale(rot);
	rotation = matrixToQuaternion(rot);
}

vec3 et::removeMatrixScale(mat3& mat)
{
	vec3 c0 = mat.column(0);
	vec3 c1 = mat.column(1);
	vec3 c2 = mat.column(2);

	float lengths[3] = { c0.length(), c1.length(), c2.length() };
	
	ET_ASSERT(lengths[0] > 0.0f);
	ET_ASSERT(lengths[1] > 0.0f);
	ET_ASSERT(lengths[2] > 0.0f);

	if (mat.determinant() < 0.0f)
	{
		float minValues[3] =
		{
			etMin(c0.x, etMin(c0.y, c0.z)),
			etMin(c1.x, etMin(c1.y, c1.z)),
			etMin(c2.x, etMin(c2.y, c2.z))
		};
		long offset = std::min_element(minValues, minValues + 3) - minValues;
		lengths[offset] = -lengths[offset];
	}

	for (size_t i = 0; i < 3; ++i)
	{
		mat[0][i] /= lengths[i];
		mat[1][i] /= lengths[i];
		mat[2][i] /= lengths[i];
	}
	
	return vec3(lengths[0], lengths[1], lengths[2]);
}

mat4 et::rotation2DMatrix(float angle)
{	
	float ca = std::cos(angle);
	float sa = std::sin(angle);
	mat4 m;
	m[0][0] = ca; m[0][1] = -sa;
	m[1][0] = sa; m[1][1] =  ca;
	m[2][2] = m[3][3] = 1.0f;
	return m;
}

mat4 et::transform2DMatrix(float a, const vec2& scale, const vec2& translate)
{
	float ca = std::cos(a);
	float sa = std::sin(a);
	mat4 m;
	m[0][0] = ca * scale.x; m[0][1] = -sa * scale.y;
	m[1][0] = sa * scale.x; m[1][1] =  ca * scale.y;
	m[2][2] = 1.0f;
	m[3][0] = translate.x;
	m[3][1] = translate.y;
	m[3][3] = 1.0f;
	return m;
}

mat3 et::rotation2DMatrix3(float angle)
{	
	float ca = std::cos(angle);
	float sa = std::sin(angle);
	mat3 m;
	m[0][0] = ca; m[0][1] = -sa;
	m[1][0] = sa; m[1][1] =  ca;
	m[2][2] = m[3][3] = 1.0f;
	return m;
}

template <>
vector2<float> et::bezierCurve(const std::vector< vector2<float> >& points, float time)
{
	ET_ASSERT(points.size() > 0);
	
	if (points.size() == 1)
		return points.front();
	else if (points.size() == 2)
		return mix(points.front(), points.back(), time);

	std::vector< vector2<float> > firstPoints(points.size() - 1);
	std::vector< vector2<float> > lastPoints(points.size() - 1);
	for (size_t i = 0; i < points.size() - 1; ++i)
	{
		firstPoints[i] = points[i];
		lastPoints[i] = points[i+1];
	}
	return mix( bezierCurve(firstPoints, time), bezierCurve(lastPoints, time), time );
}

vec3 et::circleFromPoints(const vec2& p1, const vec2& p2, const vec2& p3)
{
	// determine if some of points are same
	if (((p1 - p2).dotSelf() < std::numeric_limits<float>::epsilon()) ||
		((p3 - p2).dotSelf() < std::numeric_limits<float>::epsilon()))
	{
		return vec3(0.0f);
	}
	
	// determine is some points on one line
	vec2 n1 = normalize(vec2(p2.x - p1.x, p2.y - p1.y));
	vec2 n2 = normalize(vec2(p2.x - p3.x, p2.y - p3.y));
	
	if (std::abs(n1.x * n2.y - n2.x * n1.y) < std::numeric_limits<float>::epsilon())
		return vec3(0.0f);
	
	vec2 c1(0.5f * (p1 + p2));
	n1 = vec2(n1.y, -n1.x);
	
	vec2 c2(0.5f * (p3 + p2));
	n2 = vec2(n2.y, -n2.x);
	
	vec3 pl1(n1.y, -n1.x, c1.x * n1.y - c1.y * n1.x);
	vec3 pl2(n2.y, -n2.x, c2.x * n2.y - c2.y * n2.x);
	
	float d = pl1.x * pl2.y - pl2.x * pl1.y;
	float dx = pl1.z * pl2.y - pl2.z * pl1.y;
	float dy = pl1.x * pl2.z - pl2.x * pl1.z;
	
	vec2 pos(dx / d, dy / d);
	
	return vec3(pos.x, pos.y, (pos - p2).length());
}
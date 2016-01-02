/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <random>
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
	return rand() % limit;
}

vec3 et::randVector(float sx, float sy, float sz)
{
	return vec3(sx * randomFloat(-1.0f, 1.0f), sy * randomFloat(-1.0f, 1.0f), sz * randomFloat(-1.0f, 1.0f));
}

float et::randomFloat()
{
	return static_cast<float>(rand()) / fRandMax;
}

float et::randomFloat(float low, float up)
{
	return low + (up - low) * randomFloat();
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

void et::decomposeMatrix(const mat4& mat, vec3& translation, quaternion& rotation, vec3& scale)
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

	for (size_t i = 0; i < 3; ++i)
	{
		mat[0][i] /= lengths[i];
		mat[1][i] /= lengths[i];
		mat[2][i] /= lengths[i];
	}
	
	if (mat.determinant() < 0.0f)
	{
		float minValues[3] =
		{
			etMin(c0.x, etMin(c0.y, c0.z)),
			etMin(c1.x, etMin(c1.y, c1.z)),
			etMin(c2.x, etMin(c2.y, c2.z))
		};
		auto offset = std::min_element(minValues, minValues + 3) - minValues;
		lengths[offset] = -lengths[offset];
	}
	
	return vec3(lengths[0], lengths[1], lengths[2]);
}

mat4 et::rotation2DMatrix(float angle)
{	
	float ca = std::cos(angle);
	float sa = std::sin(angle);
	mat4 m(1.0f);
	m[0][0] = ca; m[0][1] = -sa;
	m[1][0] = sa; m[1][1] =  ca;
	m[2][2] = m[3][3] = 1.0f;
	return m;
}

mat4 et::transform2DMatrix(float a, const vec2& scale, const vec2& translate)
{
	float ca = std::cos(a);
	float sa = std::sin(a);
	mat4 m(1.0f);
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
	mat3 m(1.0f);
	m[0][0] = ca; m[0][1] = -sa;
	m[1][0] = sa; m[1][1] =  ca;
	m[2][2] = m[3][3] = 1.0f;
	return m;
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

quaternion et::quaternionFromAngles(float yaw, float pitch, float roll)
{
    float yawOver2 = 0.5f * yaw;
    float pitchOver2 = 0.5f * pitch;
	float rollOver2 = 0.5f * roll;
	
    float cosYawOver2 = std::cos(yawOver2);
	float sinYawOver2 = std::sin(yawOver2);
	
    float cosPitchOver2 = std::cos(pitchOver2);
	float sinPitchOver2 = std::sin(pitchOver2);
	
    float cosRollOver2 = std::cos(rollOver2);
    float sinRollOver2 = std::sin(rollOver2);
	
    quaternion result;
    result.scalar = cosYawOver2 * cosPitchOver2 * cosRollOver2 + sinYawOver2 * sinPitchOver2 * sinRollOver2;
    result.vector.x = sinYawOver2 * cosPitchOver2 * cosRollOver2 - cosYawOver2 * sinPitchOver2 * sinRollOver2;
    result.vector.y = cosYawOver2 * sinPitchOver2 * cosRollOver2 + sinYawOver2 * cosPitchOver2 * sinRollOver2;
    result.vector.z = cosYawOver2 * cosPitchOver2 * sinRollOver2 - sinYawOver2 * sinPitchOver2 * cosRollOver2;
	return result;
}

vec3 et::perpendicularVector(const vec3& normal)
{
	vec3 componentsLength(sqr(normal.x), sqr(normal.y), sqr(normal.z));
	
	if (componentsLength.x > 0.5f)
	{
		float scaleFactor = std::sqrt(componentsLength.z + componentsLength.x);
		return vec3(normal.z / scaleFactor, 0.0f, -normal.x / scaleFactor);
	}
	else if (componentsLength.y > 0.5f)
	{
		float scaleFactor = std::sqrt(componentsLength.y + componentsLength.x);
		return vec3(-normal.y / scaleFactor, normal.x / scaleFactor, 0.0f);
	}
	
	float scaleFactor = std::sqrt(componentsLength.z + componentsLength.y);
	return vec3(0.0f, -normal.z / scaleFactor, normal.y / scaleFactor);
}

vec3 et::randomVectorOnHemisphere(const vec3& normal, float distributionAngle)
{
	float phi = randomFloat() * DOUBLE_PI;
	float theta = std::sin(randomFloat() * clamp(distributionAngle, 0.0f, HALF_PI));
	vec3 u = perpendicularVector(normal);
	return ((u * std::cos(phi) + cross(u, normal) * std::sin(phi)) * std::sqrt(theta) +
		normal * std::sqrt(1.0f - theta)).normalized();
}

vec3 et::randomVectorOnDisk(const vec3& normal)
{
	vec3 u = perpendicularVector(normal);
	vec3 v = cross(u, normal);
	float phi = randomFloat(-PI, PI);
	return (u * std::sin(phi) + v * std::cos(phi)).normalized();
}

vec3 et::rotateAroundVector(const vec3& v, const vec3& p, float a)
{
	float cosa = std::cos(a);
	float sina = std::sin(a);
	float invCosa = 1.0f - cosa;

	mat3 transform;
	transform[0][0] = cosa + invCosa * v.x * v.x;
	transform[0][1] = invCosa * v.y * v.x - sina * v.z;
	transform[0][2] = invCosa * v.x * v.z + sina * v.y;

	transform[1][0] = invCosa * v.y * v.x + sina * v.z;
	transform[1][1] = cosa + invCosa * v.y * v.y;
	transform[1][2] = invCosa * v.y * v.z - sina * v.x;

	transform[2][0] = invCosa * v.x * v.z - sina * v.y;
	transform[2][1] = invCosa * v.z * v.y + sina * v.x;
	transform[2][2] = cosa + invCosa * v.z * v.z;

	return transform * p;
}

/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/tools.h>
#include <et/collision/collision.h>

using namespace et;

float et::distanceSquareFromPointToLine(const vec3& p, const vec3& l0, const vec3& l1, vec3& projection)
{
	vec3 diff = p - l0;
	vec3 v = l1 - l0;

	float t = dot(v, diff);
	if (t > 0) 
	{
		float dotVV = v.dotSelf();
		if (t < dotVV) 
		{
			t /= dotVV;
			diff -= t * v;
		}
		else 
		{
			t = 1.0f;
			diff -= v;
		}
		projection = l0 + t * v;
	} 
	else
	{
		projection = l0;
	}

	return diff.dotSelf();
}

vec2 et::barycentricCoordinates(vec3 v2, const triangle& t)
{
	v2 -= t.v1();
	float dot00 = dot(t.edge3to1(), t.edge3to1());
	float dot01 = dot(t.edge3to1(), t.edge2to1());
	float dot02 = dot(t.edge3to1(), v2);
	float dot11 = dot(t.edge2to1(), t.edge2to1());
	float dot12 = dot(t.edge2to1(), v2);
	float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	return vec2((dot11 * dot02 - dot01 * dot12) * invDenom, (dot00 * dot12 - dot01 * dot02) * invDenom);
}

vec3 et::worldCoordinatesFromBarycentric(const vec2& b, const triangle& t)
{
	return t.v1() * (1.0f - b.x - b.y) + t.v2() * b.y + t.v3() * b.x;
}

vec3 et::closestPointOnTriangle(const vec3& sourcePosition, const triangle& triangle)
{
	vec3 edge0 = triangle.edge2to1();
	vec3 edge1 = triangle.edge3to1();
	vec3 v0 = triangle.v1() - sourcePosition;

	float a = dot(edge0, edge0);
	float b = dot(edge0, edge1);
	float c = dot(edge1, edge1);
	float d = dot(edge0, v0);
	float e = dot(edge1, v0);

	float det = a*c - b * b;
	float s = b * e - c * d;
	float t = b * d - a * e;

	if (s + t < det)
	{
		if ( s < 0.0f )
		{
			if ( t < 0.0f )
			{
				if ( d < 0.0f )
				{
					s = clamp( -d/a, 0.0f, 1.0f );
					t = 0.0f;
				}
				else
				{
					s = 0.0f;
					t = clamp( -e/c, 0.0f, 1.0f );
				}
			}
			else
			{
				s = 0.0f;
				t = clamp( -e/c, 0.0f, 1.0f );
			}
		}
		else if ( t < 0.0f )
		{
			s = clamp( -d/a, 0.0f, 1.0f );
			t = 0.0f;
		}
		else
		{
			float invDet = 1.0f / det;
			s *= invDet;
			t *= invDet;
		}
	}
	else
	{
		if ( s < 0.0f )
		{
			float tmp0 = b+d;
			float tmp1 = c+e;
			if ( tmp1 > tmp0 )
			{
				s = clamp((tmp1 - tmp0) / (a - 2*b + c), 0.0f, 1.0f );
				t = 1-s;
			}
			else
			{
				t = clamp( -e/c, 0.0f, 1.0f );
				s = 0.0f;
			}
		}
		else if ( t < 0.0f )
		{
			if ( a+d > b+e )
			{
				s = clamp( (c+e-b-d)/(a-2*b+c), 0.0f, 1.0f );
				t = 1-s;
			}
			else
			{
				s = clamp( -e/c, 0.0f, 1.0f );
				t = 0.0f;
			}
		}
		else
		{
			s = clamp((c+e-b-d)/(a-2*b+c), 0.0f, 1.0f );
			t = 1.0f - s;
		}
	}

	return triangle.v1() + s * edge0 + t * edge1;
}

bool et::pointInsideTriangle(const vec3& p, const triangle& t)
{
	vec2 b = barycentricCoordinates(p, t);
	return (b.x >= 0.0f) && (b.y >= 0.0f) && (b.x + b.y < 1.0f);
}

bool et::pointInsideTriangle(const vec3& p, const triangle& t, const vec3& n)
{
	float r1 = dot(t.edge2to1().cross(n), p - t.v1());
	float r2 = dot(t.edge3to2().cross(n), p - t.v2());
	float r3 = dot(t.edge3to1().cross(n), t.v3() - p);

	return ((r1 > 0) && (r2 > 0) && (r3 > 0)) || ((r1 <= 0) && (r2 <= 0) && (r3 <= 0));
}

bool et::intersect::raySphere(const ray3d& r, const Sphere& s, vec3* ip1, vec3* ip2)
{
	vec3 dv = r.origin - s.center();
	
	float b = dot(r.direction, dv);
	if (b > 0.0) return false;
	
	float d = sqr(b) - dv.dotSelf() + sqr(s.radius());
	if (d < 0.0f) return false;

	if (ip1 || ip2)
	{
		d = std::sqrt(d);
		
		if (ip1)
			*ip1 = r.origin - (b + d) * r.direction;
		
		if (ip2)
			*ip2 = r.origin + (d - b) * r.direction;
	}
	
	return true;
}

bool et::intersect::rayPlane(const ray3d& r, const plane& p, vec3* intersection_pt)
{
	float d = dot(r.direction, p.normal());
	if (d < 0.0f)
	{
		if (intersection_pt)
			*intersection_pt = r.origin + r.direction * dot(p.normal(), p.planePoint() - r.origin) / d;
		return true;
	}
	return false;
}

bool et::intersect::rayTriangle(const ray3d& r, const triangle& t, vec3* intersection_pt)
{
	float d = dot(r.direction, t.normalizedNormal());
	if (d >= 0.0f) return false;
	
	float a = dot(t.normalizedNormal(), (t.normalizedNormal() * dot(t.normalizedNormal(), t.v1())) - r.origin);
	if (a >= 0.0f) return false;
	
	vec3 ip = r.origin + (a / d) * r.direction;
	if (pointInsideTriangle(ip, t))
	{
		if (intersection_pt)
			*intersection_pt = ip;
		return true;
	}
	
	return false;
}

bool et::intersect::rayTriangleTwoSided(const ray3d& ray, const triangle& tri, vec3* intersection_pt)
{
	static const float epsilon = 0.00001f;
	
	vec3 h = cross(ray.direction, tri.edge3to1());
	float a = dot(tri.edge2to1(), h);
	
	if (std::abs(a) < epsilon)
		return false;
	
	vec3 s = ray.origin - tri.v1();
	
	float u = dot(s, h) / a;
	
	if ((u < 0.0) || (u > 1.0))
		return false;
	
	vec3 q = cross(s, tri.edge2to1());
	float v = dot(ray.direction, q) / a;
	
	if ((v < 0.0) || (u + v > 1.0))
		return false;
	
	float t = dot(tri.edge3to1(), q) / a;
	
	if (intersection_pt)
		*intersection_pt = ray.origin + t * ray.direction;
	
	return (t > epsilon);
}

bool et::intersect::rayTriangles(const ray3d& r, const triangle* triangles, size_t triangleCount,
	vec3* intersection_pt)
{
	for (size_t i = 0; i < triangleCount; ++i)
	{
		if (rayTriangle(r, triangles[i], intersection_pt))
			return true;
	}

	return false;
}

bool et::intersect::segmentPlane(const segment3d& s, const plane& p, vec3* intersection_pt)
{
	vec3 ds = s.end - s.start;
	float d = dot(ds, p.normal());
	if (d >= 0.0f) return false;

	float t = dot(p.normal(), p.planePoint() - s.start) / d;
	if ((t < 0.0f) || (t > 1.0f)) return false;

	if (intersection_pt)
		*intersection_pt = s.start + t * ds;

	return true;
}

bool et::intersect::segmentTriangle(const segment3d& s, const triangle& t, vec3* intersection_pt)
{
	vec3 ip;

	if (segmentPlane(s, plane(t), &ip))
	{
		if (pointInsideTriangle(ip, t))
		{
			if (intersection_pt)
				*intersection_pt = ip;
			return true;
		}
	}

	return false;
}

bool et::intersect::triangleTriangle(const et::triangle& t0, const et::triangle& t1)
{
#define ProjectOntoAxis(triangle, axis, fmin, fmax) \
	dot1 = dot(axis, triangle.v2());					\
	dot2 = dot(axis, triangle.v3());					\
	fmin = dot(axis, triangle.v1()), fmax = fmin;		\
	if (dot1 < fmin) fmin = dot1; else if (dot1 > fmax) fmax = dot1; \
	if (dot2 < fmin) fmin = dot2; else if (dot2 > fmax) fmax = dot2;

	float min0 = 0.0f;
	float min1 = 0.0f;
	float max0 = 0.0f;
	float max1 = 0.0f;
	float dot1 = 0.0f;
	float dot2 = 0.0f;

	vec3 E0[3] = 
	{
		 normalize(t0.edge2to1()),
		 normalize(t0.edge3to2()), 
		-normalize(t0.edge3to1())
	};

	vec3 N0 = cross(E0[0], E0[1]);
	float N0dT0V0 = dot(N0, t0.v1());
	ProjectOntoAxis(t1, N0, min1, max1);
	if ((N0dT0V0 < min1) || (N0dT0V0 > max1)) return false;

	vec3 E1[3] = 
	{
		 normalize(t1.edge2to1()), 
		 normalize(t1.edge3to2()), 
		-normalize(t1.edge3to1())
	};
	vec3 N1 = cross(E1[0], E1[1]);
	if (cross(N0, N1).dotSelf() >= 1.0e-5)
	{
		float N1dT1V0 = dot(N1, t1.v1());
		ProjectOntoAxis(t0, N1, min0, max0);
		if ((N1dT1V0 < min0) || (N1dT1V0 > max0)) return false;

		for (int i = 0; i < 3; ++i)
		{
			vec3 dir = cross(E0[0], E1[i]);
			ProjectOntoAxis(t0, dir, min0, max0);
			ProjectOntoAxis(t1, dir, min1, max1);
			if ((max0 < min1) || (max1 < min0)) return false;
			dir = cross(E0[1], E1[i]);
			ProjectOntoAxis(t0, dir, min0, max0);
			ProjectOntoAxis(t1, dir, min1, max1);
			if ((max0 < min1) || (max1 < min0)) return false;
			dir = cross(E0[2], E1[i]);
			ProjectOntoAxis(t0, dir, min0, max0);
			ProjectOntoAxis(t1, dir, min1, max1);
			if ((max0 < min1) || (max1 < min0)) return false;
		}
	}
	else 
	{
		for (int i = 0; i < 3; ++i)
		{
			vec3 dir = cross(N0, E0[i]);
			ProjectOntoAxis(t0, dir, min0, max0);
			ProjectOntoAxis(t1, dir, min1, max1);
			if ((max0 < min1) || (max1 < min0)) return false;
			dir = cross(N1, E1[i]);
			ProjectOntoAxis(t0, dir, min0, max0);
			ProjectOntoAxis(t1, dir, min1, max1);
			if ((max0 < min1) || (max1 < min0)) return false;
		}

	}

#undef ProjectOntoAxis
	return true;
}

bool et::intersect::sphereSphere(const Sphere& s1, const Sphere& s2, vec3* amount)
{
	vec3 dv = s2.center() - s1.center();

	float distance = dv.dotSelf();
	float radiusSum = s1.radius() + s2.radius();
	bool collised = (distance <= sqr(radiusSum));

	if (amount && collised)
		*amount = dv.normalized() * (radiusSum - std::sqrt(distance));

	return collised;
}

bool et::intersect::sphereBox(const vec3& sphereCenter, float sphereRadius, const vec3& boxCenter,
	const vec3& boxExtent)
{
	vec3 bMin = boxCenter - boxExtent;
	vec3 bMax = boxCenter + boxExtent;

	float d = 0; 

	if (sphereCenter.x < bMin.x)
		d += sqr(sphereCenter.x - bMin.x); 
	else if (sphereCenter.x > bMax.x)
		d += sqr(sphereCenter.x - bMax.x); 
	if (sphereCenter.y < bMin.y)
		d += sqr(sphereCenter.y - bMin.y); 
	else if (sphereCenter.y > bMax.y)
		d += sqr(sphereCenter.y - bMax.y); 
	if (sphereCenter.z < bMin.z)
		d += sqr(sphereCenter.z - bMin.z); 
	else if (sphereCenter.z > bMax.z)
		d += sqr(sphereCenter.z - bMax.z); 

	return d <= sqr(sphereRadius); 
}

bool et::intersect::sphereAABB(const Sphere& s, const AABB& b)
{
	return sphereBox(s.center(), s.radius(), b.center, b.halfDimension);
}

bool et::intersect::sphereOBB(const Sphere& s, const OBB& b)
{
	return sphereBox(b.center + b.transform.transposed() * (s.center() - b.center),
		s.radius(), b.center, b.dimension);
}

bool et::intersect::aabbAABB(const AABB& a1, const AABB& a2)
{
	vec3 dc = a1.center - a2.center;
	vec3 sd = a1.halfDimension + a2.halfDimension;

	if (std::abs(dc.x) > (sd.x)) return false;
	if (std::abs(dc.y) > (sd.y)) return false;
	if (std::abs(dc.z) > (sd.z)) return false;

	return true;
}

bool et::intersect::sphereTriangle(const vec3& sphereCenter, const float radius, const triangle& t, 
	vec3& point, vec3& normal, float& penetration)
{
	normal = t.normalizedNormal();
	float distanceFromPlane = dot(sphereCenter - t.v1(), normal);
	if (distanceFromPlane < 0.0f)
	{
		distanceFromPlane *= -1.0f;
		normal *= -1.0f;
	}

	if (distanceFromPlane >= radius) return false;

	float radiusSqr = sqr(radius);
	if (pointInsideTriangle(sphereCenter, t, normal)) 
	{
		point = sphereCenter - normal * distanceFromPlane;
	}
	else if (distanceSquareFromPointToLine(sphereCenter, t.v1(), t.v2(), point) > radiusSqr)
	{
		if (distanceSquareFromPointToLine(sphereCenter, t.v2(), t.v3(), point) > radiusSqr)
		{
			if (distanceSquareFromPointToLine(sphereCenter, t.v3(), t.v1(), point) > radiusSqr)
				return false;
		}
	}

	vec3 contactToCentre = sphereCenter - point;
	float distanceSqr = contactToCentre.dotSelf();
	
	if (distanceSqr > radiusSqr) return false;

	if (distanceSqr > std::numeric_limits<float>::epsilon())
	{
		normal = contactToCentre.normalized();
		penetration = radius - sqrtf(distanceSqr);
	} 
	else
	{
		penetration = radius;
	}

	return true;
}

bool et::intersect::sphereTriangle(const Sphere& s, const triangle& t, vec3& point, vec3& normal,
	float& penetration)
{
	return sphereTriangle(s.center(), s.radius(), t, point, normal, penetration);
}

bool et::intersect::sphereTriangle(const Sphere& s, const vec3& sphereVelocity, const triangle& t, 
	vec3& point, vec3& normal, float& penetration, float& intersectionTime)
{
	plane p(t);
	if (p.distanceToPoint(s.center()) <= s.radius())
		 return sphereTriangle(s, t, point, normal, penetration);

	vec3 triangleNormal = p.normal();
	float NdotV = dot(triangleNormal, sphereVelocity);
	if (NdotV >= 0.0f) return false;

	intersectionTime = (s.radius() + p.equation.w - dot(triangleNormal, s.center())) / NdotV;
	vec3 movedCenter = s.center() + intersectionTime * sphereVelocity;
	return sphereTriangle(movedCenter, s.radius(), t, point, normal, penetration);
}

bool et::intersect::sphereTriangles(const Sphere& s, const triangle* triangles,
	size_t triangleCount, vec3& point, vec3& normal, float& penetration)
{
	for (size_t i = 0; i < triangleCount; ++i)
	{
		if (sphereTriangle(s.center(), s.radius(), triangles[i], point, normal, penetration))
			return true;
	}
	return false;
}

bool et::intersect::sphereTriangles(const Sphere& s, const vec3& sphereVelocity,
	const triangle* triangles, size_t triangleCount, vec3& point, vec3& normal,
	float& penetration, float& intersectionTime)
{
	for (size_t i = 0; i < triangleCount; ++i)
	{
		if (sphereTriangle(s, sphereVelocity, triangles[i], point, normal,
			penetration, intersectionTime)) return true;
	}
	return false;
}

bool et::intersect::raySegment(const ray2d& ray, const segment2d& segment, vec2* intersectionPoint)
{
	vec2 lp;
	
	if (ray.line().intersects(segment.line(), &lp))
	{
		if (segment.containsPoint(lp))
		{
			bool rightDirection = dot(ray.direction, lp - ray.origin) >= 0.0f;
			
			if (rightDirection && (intersectionPoint != nullptr))
				*intersectionPoint = lp;
			
			return rightDirection;
		}
	}
	
	return false;
}

bool et::pointInsidePolygon(const vec2& p, const std::vector<vec2>& polygon)
{
	size_t intersectionCount = 0;
	
	ray2d ray(p, vec2(1.0f, 0.0f));
	for (size_t i = 0, e = polygon.size(); i != e; ++i)
	{
		size_t nextIndex = (i + 1) % polygon.size();
		
		vec2 v0 = polygon.at(i);
		vec2 v1 = polygon.at(nextIndex);
		
		vec2 direction = normalize(v1 - v0);
		v0 += std::numeric_limits<float>::epsilon() * direction;
		v1 += std::numeric_limits<float>::epsilon() * direction;

		vec2 ip;
		if (intersect::raySegment(ray, segment2d(v0, v1), &ip))
		{
			if (length(ip - v0) <= std::numeric_limits<float>::epsilon())
			{
				size_t prevIndex = ((i == 0) ? polygon.size() : i) - 1;
				float op = outerProduct(polygon.at(prevIndex) - v0, v1 - v0) ;
				intersectionCount += (op > 0.0f) ? 2 : 1;
			}
			else if (length(ip - v1) <= std::numeric_limits<float>::epsilon())
			{
				size_t nextNextIndex = (nextIndex + 1) % polygon.size();
				float op = outerProduct(v0 - v1, polygon.at(nextNextIndex) - v1);
				intersectionCount += (op > 0.0f) ? 2 : 1;
			}
			else
			{
				intersectionCount++;
			}
		}
	}
	
	return (intersectionCount % 2) == 1;
}

bool et::intersect::rayAABB(et::ray3d r, const et::AABB& box)
{
	r.direction.x = 1.0f / r.direction.x;
	r.direction.y = 1.0f / r.direction.y;
	r.direction.z = 1.0f / r.direction.z;
	
	int r_sign_x = (r.direction.x < 0.0f ? 1 : 0);
	int r_sign_y = (r.direction.y < 0.0f ? 1 : 0);
	
	vec3 parameters[2] = { box.minVertex(), box.maxVertex() };
	
	float txmin = (parameters[r_sign_x].x - r.origin.x) * r.direction.x;
	float tymin = (parameters[r_sign_y].y - r.origin.y) * r.direction.y;
	
	float txmax = (parameters[1 - r_sign_x].x - r.origin.x) * r.direction.x;
	float tymax = (parameters[1 - r_sign_y].y - r.origin.y) * r.direction.y;
	
	if ((txmin < tymax) && (tymin < txmax))
	{
		if (tymin > txmin) txmin = tymin;
		if (tymax < txmax) txmax = tymax;
		
		int r_sign_z = (r.direction.z < 0.0f ? 1 : 0);
		
		float tzmin = (parameters[r_sign_z].z - r.origin.z) * r.direction.z;
		float tzmax = (parameters[1 - r_sign_z].z - r.origin.z) * r.direction.z;
		
		if ((txmin < tzmax) && (tzmin < txmax))
			return true;
	}
	
	return false;
}

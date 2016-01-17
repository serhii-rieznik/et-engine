/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/vector4-simd.h>
#include <et/scene3d/scene3d.h>

namespace et
{
	namespace rt
	{
		using float4 = vec4simd;
		using index = uint32_t;
		
		enum : index
		{
			InvalidIndex = index(-1),
		};
		
		struct Constants
		{
			static const float epsilon;
			static const float minusEpsilon;
			static const float onePlusEpsilon;
			static const float epsilonSquared;
			static const float initialSplitValue;
		};

#		pragma pack(push, 16)
		struct ET_ALIGNED(16) Material
		{
			std::string name;
			float4 diffuse;
			float4 specular;
			float4 emissive;
			float roughness = 0.0f;
			float ior = 0.0f;
		};

		struct ET_ALIGNED(16) Triangle
		{
			float4 v[3];
			float4 n[3];
			float4 edge1to0;
			float4 edge2to0;
			float4 _invDenom;
			index materialIndex = 0;
			float _invDenomValue = 0.0f;
			float _dot00 = 0.0f;
			float _dot01 = 0.0f;
			float _dot11 = 0.0f;

			void computeSupportData()
			{
				edge1to0 = v[1] - v[0];
				edge2to0 = v[2] - v[0];
				_dot00 = edge1to0.dotSelf();
				_dot11 = edge2to0.dotSelf();
				_dot01 = edge1to0.dot(edge2to0);
				_invDenomValue = 1.0f / (_dot00 * _dot11 - _dot01 * _dot01);
				_invDenom = rt::float4(_invDenomValue);
			}

			float4 barycentric(const float4& inP) const
			{
				float4 p = inP - v[0];
				float dot20 = p.dot(edge1to0);
				float dot21 = p.dot(edge2to0);
				float y = _dot11 * dot20 - _dot01 * dot21;
				float z = _dot00 * dot21 - _dot01 * dot20;
				return float4(_invDenomValue - y - z, y, z, 0.0f) * _invDenom;
			}

			float4 interpolatedNormal(const float4& b) const
			{
				auto result =
					n[0] * b.shuffle<0, 0, 0, 3>() +
					n[1] * b.shuffle<1, 1, 1, 3>() +
					n[2] * b.shuffle<2, 2, 2, 3>();
				result.normalize();
				return result;
			}
			
			float4 geometricNormal() const
			{
				float4 c = edge1to0.crossXYZ(edge2to0);
				c.normalize();
				return c;
			}

			float4 averageNormal() const
			{
				float4 result = n[0] + n[1] + n[2];
				result.normalize();
				return result;
			}
			
			float4 minVertex() const
			{
				return v[0].minWith(v[1].minWith(v[2]));
			}
			
			float4 maxVertex() const
			{
				return v[0].maxWith(v[1].maxWith(v[2]));
			}
		};
		using TriangleList = std::vector<rt::Triangle, SharedBlockAllocatorSTDProxy<rt::Triangle>>;
		
		struct ET_ALIGNED(16) IntersectionData
		{
			float4 v0;
			float4 edge1to0;
			float4 edge2to0;
			
			IntersectionData(const float4& v, const float4& e1, const float4& e2) :
				v0(v), edge1to0(e1), edge2to0(e2) { }
		};
		using IntersectionDataList =
			std::vector<IntersectionData, SharedBlockAllocatorSTDProxy<rt::IntersectionData>>;

		struct ET_ALIGNED(16) BoundingBox
		{
			float4 center = float4(0.0f);
			float4 halfSize = float4(0.0f);
			
			BoundingBox()
				{ }
			
			BoundingBox(const float4& c, const float4& hs) :
				center(c), halfSize(hs) { }
			
			BoundingBox(const float4& minVertex, const float4& maxVertex, int)
			{
				center = (minVertex + maxVertex) * 0.5f;
				halfSize = (maxVertex - minVertex) * 0.5f;
			}
			
			float4 minVertex() const
				{ return center - halfSize; }
			
			float4 maxVertex() const
				{ return center + halfSize; }
			
			float square() const
			{
				float xy = halfSize.cX() * halfSize.cY();
				float yz = halfSize.cY() * halfSize.cZ();
				float xz = halfSize.cX() * halfSize.cZ();
				return 4.0f * (xy + yz + xz);
			}
			
			float volume() const
			{
				return 8.0f * halfSize.cX() * halfSize.cY() * halfSize.cZ();
			};
		};
		using BoundingBoxList = std::vector<BoundingBox, SharedBlockAllocatorSTDProxy<BoundingBox>>;
#		pragma pack(pop)

		struct Ray
		{
			float4 origin;
			float4 direction;

			Ray() { }

			Ray(const float4& o, const float4& d) : 
				origin(o), direction(d) { }

			Ray(const ray3d& r) : 
				origin(r.origin, 1.0f), direction(r.direction, 0.0f) { } 
		};

		struct Region
		{
			vec2i origin = vec2i(0);
			vec2i size = vec2i(0);
			size_t estimatedBounces = 0;
			bool sampled = false;
		};
		
		inline float fastRandomFloat()
		{
			union
			{
				float fres;
				unsigned int ires;
			};
			static unsigned int seed = 1;
			ires = (((seed *= 16807) >> 9) | 0x3f800000);
			return fres - 1.0f;
		}

		inline int floatIsNegative(float& a)
		{
			return reinterpret_cast<uint32_t&>(a) >> 31;
		}
		
		inline bool floatIsPositive(float& a)
		{
			return (reinterpret_cast<uint32_t&>(a) >> 31) == 0;
		}
		
		inline bool rayTriangle(const Ray& ray, const Triangle* tri, float& distance, float4& barycentric)
		{
			float4 pvec = ray.direction.crossXYZ(tri->edge2to0);
			float det = tri->edge1to0.dot(pvec);
			
			if (det * det < Constants::epsilonSquared)
				return false;
			
			float4 tvec = ray.origin - tri->v[0];
			float u = tvec.dot(pvec) / det;
			
			if ((u < Constants::minusEpsilon) || (u > Constants::onePlusEpsilon))
				return false;

			float4 qvec = tvec.crossXYZ(tri->edge1to0);
			float v = ray.direction.dot(qvec) / det;
			
			if ((v < Constants::minusEpsilon) || (u + v > Constants::onePlusEpsilon))
				return false;

			distance = tri->edge2to0.dot(qvec) / det;
			barycentric = float4(1.0f - u - v, u, v, 0.0f);
			
			return distance > Constants::epsilon;
		}

		inline float4 perpendicularVector(const float4& normal)
		{
			vec3 componentsLength = (normal * normal).xyz();

			if (componentsLength.x > 0.5f)
			{
				float scaleFactor = std::sqrt(componentsLength.z + componentsLength.x);
				return normal.shuffle<2, 3, 0, 1>() * float4(1.0f / scaleFactor, 0.0f, -1.0f / scaleFactor, 0.0f);
			}
			else if (componentsLength.y > 0.5f)
			{
				float scaleFactor = std::sqrt(componentsLength.y + componentsLength.x);
				return normal.shuffle<1, 0, 3, 3>() * float4(-1.0f / scaleFactor, 1.0f / scaleFactor, 0.0f, 0.0f);
			}
			float scaleFactor = std::sqrt(componentsLength.z + componentsLength.y);
			return normal.shuffle<3, 2, 1, 3>() * float4(0.0f, -1.0f / scaleFactor, 1.0f / scaleFactor, 0.0f);
		}

		inline float4 randomVectorOnHemisphere(const float4& normal, float distributionAngle)
		{
			float phi = fastRandomFloat() * DOUBLE_PI;
			float theta = std::sin(fastRandomFloat() * clamp(distributionAngle, 0.0f, HALF_PI));
			float4 u = perpendicularVector(normal);
			float4 result = (u * std::cos(phi) + u.crossXYZ(normal) * std::sin(phi)) * std::sqrt(theta) +
				normal * std::sqrt(1.0f - theta);
			result.normalize();
			return result;
		}

		inline float4 reflect(const float4& v, const float4& n)
		{
			const float4 two(2.0f);
			return v - two * n * v.dotVector(n);
		}
		
		inline bool pointInsideBoundingBox(const float4& p, const BoundingBox& box)
		{
			float4 lower = box.minVertex();
			
			if ((p.cX() < lower.cX() - Constants::epsilon) ||
				(p.cY() < lower.cY() - Constants::epsilon) ||
				(p.cZ() < lower.cZ() - Constants::epsilon)) return false;
			
			float4 upper = box.maxVertex();
			
			if ((p.cX() > upper.cX() + Constants::epsilon) ||
				(p.cY() > upper.cY() + Constants::epsilon) ||
				(p.cZ() > upper.cZ() + Constants::epsilon)) return false;
			
			return true;
		}
		
		inline bool rayToBoundingBox(const Ray& r, const BoundingBox& box, float& tNear, float& tFar)
		{
			float4 bounds[2] = { box.minVertex(), box.maxVertex() };
			
			float tmin, tmax, tymin, tymax, tzmin, tzmax;
			
			if (r.direction.cX() >= 0)
			{
				tmin = (bounds[0].cX() - r.origin.cX()) / r.direction.cX() - Constants::epsilon;
				tmax = (bounds[1].cX() - r.origin.cX()) / r.direction.cX() + Constants::epsilon;
			}
			else
			{
				tmin = (bounds[1].cX() - r.origin.cX()) / r.direction.cX() - Constants::epsilon;
				tmax = (bounds[0].cX() - r.origin.cX()) / r.direction.cX() + Constants::epsilon;
			}
			
			if (r.direction.cY() >= 0)
			{
				tymin = (bounds[0].cY() - r.origin.cY()) / r.direction.cY() - Constants::epsilon;
				tymax = (bounds[1].cY() - r.origin.cY()) / r.direction.cY() + Constants::epsilon;
			}
			else
			{
				tymin = (bounds[1].cY() - r.origin.cY()) / r.direction.cY() - Constants::epsilon;
				tymax = (bounds[0].cY() - r.origin.cY()) / r.direction.cY() + Constants::epsilon;
			}
			
			if ((tmin > tymax) || (tymin > tmax))
				return false;
			
			if (tymin > tmin)
				tmin = tymin;
			
			if (tymax < tmax)
				tmax = tymax;
			
			if (r.direction.cZ() >= 0)
			{
				tzmin = (bounds[0].cZ() - r.origin.cZ()) / r.direction.cZ() - Constants::epsilon;
				tzmax = (bounds[1].cZ() - r.origin.cZ()) / r.direction.cZ() + Constants::epsilon;
			}
			else
			{
				tzmin = (bounds[1].cZ() - r.origin.cZ()) / r.direction.cZ() - Constants::epsilon;
				tzmax = (bounds[0].cZ() - r.origin.cZ()) / r.direction.cZ() + Constants::epsilon;
			}
			
			if ( (tmin > tzmax) || (tzmin > tmax) )
				return false;
			
			if (tzmin > tmin)
				tmin = tzmin;
			
			if (tzmax < tmax)
				tmax = tzmax;
			
			tNear = tmin;
			tFar = tmax;
			
			return tmin <= tmax;
		}
		
		inline bool rayHitsBoundingBox(const Ray& r, const BoundingBox& box)
		{
			vec4 origin;
			vec4 invDirection;
			r.origin.loadToVec4(origin);
			r.direction.reciprocal().loadToVec4(invDirection);
			
			int r_sign_x = (invDirection.x < 0.0f ? 1 : 0);
			int r_sign_y = (invDirection.y < 0.0f ? 1 : 0);
			
			float4 parameters[2] = { box.minVertex(), box.maxVertex() };
			
			float txmin = (parameters[r_sign_x].cX() - origin.x) * invDirection.x;
			float tymin = (parameters[r_sign_y].cY() - origin.y) * invDirection.y;
			float txmax = (parameters[1 - r_sign_x].cX() - origin.x) * invDirection.x;
			float tymax = (parameters[1 - r_sign_y].cY() - origin.y) * invDirection.y;
			
			if ((txmin >= tymax) || (tymin >= txmax))
				return false;
			
			if (tymin > txmin)
				txmin = tymin;
			if (tymax < txmax)
				txmax = tymax;
			
			int r_sign_z = (invDirection.z < 0.0f ? 1 : 0);
			float tzmin = (parameters[r_sign_z].cZ() - origin.z) * invDirection.z;
			float tzmax = (parameters[1 - r_sign_z].cZ() - origin.z) * invDirection.z;
			
			return ((txmin < tzmax) && (tzmin < txmax));
		}
		
		inline float computeRefractiveCoefficient(const float4& incidence, const float4& normal, float eta)
		{
			float NdotI = normal.dot(incidence);
			return 1.0f - (eta * eta) * (1.0f - NdotI * NdotI);
		}
		
		inline float computeFresnelTerm(const float4& incidence, const float4& normal, float eta)
		{
			float cosTheta = std::abs(incidence.dot(normal));
			float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
			float etaCosTheta = eta * cosTheta;
			float v = std::sqrt(1.0f - sqr(eta * sinTheta));
			return sqr((etaCosTheta - v) / (etaCosTheta + v + 0.000001f));
		}

	}
}

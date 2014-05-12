/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

namespace et
{
	template <typename T>
	struct Quaternion
	{
		T scalar;
		vector3<T> vector;

		Quaternion() :
			scalar(static_cast<T>(1)), vector(static_cast<T>(0)) { }

		Quaternion(T s, T x, T y, T z) :
			scalar(s), vector(x, y, z) { }

		explicit Quaternion(const vector3<T>& axis) :
			scalar(0), vector(axis) { }
		
		Quaternion(T angle, const vector3<T>& axis)
		{
			T half = angle  / static_cast<T>(2);
			scalar = static_cast<T>(std::cos(half));
			vector = static_cast<T>(std::sin(half)) * axis;
		}

		T& operator[](int i) 
			{ return *(&scalar + i); }

		const T& operator[](int i) const
			{ return *(&scalar + i); }

		Quaternion operator !() const
			{ return Quaternion(scalar, -vector.x, -vector.y, -vector.z); }

		Quaternion operator - () const
			{ return Quaternion(-scalar, -vector.x, -vector.y, -vector.z); }
		
		Quaternion operator * (const Quaternion &q) const
		{
			Quaternion result;
			result.scalar = scalar * q.scalar - dot(vector, q.vector);
			result.vector = vector.cross(q.vector) + scalar * q.vector + q.scalar * vector;
			return result;
		};

		Quaternion operator * (T v) const
		{
			Quaternion result;
			result.scalar = scalar * v;
			result.vector = vector * v;
			return result;
		};

		Quaternion operator / (T v) const
		{
			Quaternion result;
			result.scalar = scalar / v;
			result.vector = vector / v;
			return result;
		};

		T length() const
			{ return std::sqrt(scalar*scalar + vector.dotSelf()); }

		void normalize() 
		{ 
			T len = this->length();
			if (len > 0)
			{
				len = sqrt(len);
				scalar /= len;
				vector /= len;
			}
		}

		vector3<T> transform(const vector3<T> &v) const
		{
			const Quaternion& thisOne = *this;
			return (thisOne * Quaternion(v) * (!thisOne)).vector;
		}

		vector3<T> invtransform(const vector3<T> &v) const
		{
			const Quaternion& thisOne = *this;
			return ((!thisOne) * Quaternion(v) * thisOne).vector;
		}

		Quaternion<T>& operator *= (const Quaternion &q) 
		{
			T s = scalar * q.scalar - dot(vector, q.vector);
			vector3<T> v = vector.cross(q.vector) + scalar * q.vector + q.scalar * vector;
			scalar = s;
			vector = v;
			return *this;
		}

		matrix4<T> toMatrix() const
		{
			T len = std::sqrt(vector.dotSelf() + scalar * scalar);
			T qx = vector.x / len;
			T qy = vector.y / len;
			T qz = vector.z / len;
			T qw = scalar / len;
			T one = static_cast<T>(1);
			T two = static_cast<T>(2);
			return matrix4<T>(
				vector4<T>(one - two*qy*qy - two*qz*qz,       two*qx*qy - two*qz*qw,       two*qx*qz + two*qy*qw, 0.0f),
				vector4<T>(      two*qx*qy + two*qz*qw, one - two*qx*qx - two*qz*qz,       two*qy*qz - two*qx*qw, 0.0f),
				vector4<T>(      two*qx*qz - two*qy*qw,       two*qy*qz + two*qx*qw, one - two*qx*qx - two*qy*qy, 0.0f),
				vector4<T>(                       0.0f,                        0.0f,                        0.0f, 1.0f));
		}
	};

	template <typename T>
	inline Quaternion<T> operator * (T value, const Quaternion<T>& q)
		{ return q * value; }
}

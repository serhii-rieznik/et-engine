/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/matrix3.h>

namespace et
{
template<typename T>
struct matrix4
{
public:
	enum : size_t
	{
		Rows = 4,
		ComponentSize = sizeof(T),
		RowSize = Rows * vector4<T>::ComponentSize
	};

	typedef vector4<T> RowType;

public:
	RowType mat[Rows];

public:
	matrix4()
	{
	}

	explicit matrix4(T s)
	{
		mat[0] = RowType(s, 0, 0, 0);
		mat[1] = RowType(0, s, 0, 0);
		mat[2] = RowType(0, 0, s, 0);
		mat[3] = RowType(0, 0, 0, s);
	}

	matrix4(const RowType& c0, const RowType& c1, const RowType& c2, const RowType& c3)
	{
		mat[0] = c0;
		mat[1] = c1;
		mat[2] = c2;
		mat[3] = c3;
	}

	matrix4(const matrix3<T>& transform, const vector3<T>& translation, bool explicitTranslation)
	{
		mat[0] = RowType(transform[0], 0);
		mat[1] = RowType(transform[1], 0);
		mat[2] = RowType(transform[2], 0);
		mat[3] = RowType(explicitTranslation ? translation : transform * translation, T(1));
	}

	explicit matrix4(const matrix3<T>& m)
	{
		mat[0] = RowType(m[0], 0.0f);
		mat[1] = RowType(m[1], 0.0f);
		mat[2] = RowType(m[2], 0.0f);
		mat[3] = RowType(0.0f, 0.0f, 0.0f, 1.0f);
	}

	matrix4& operator = (const matrix4& c)
	{
		mat[0] = c[0];
		mat[1] = c[1];
		mat[2] = c[2];
		mat[3] = c[3];
		return *this;
	}

	T* data()
	{
		return mat[0].data();
	}

	const T* data() const
	{
		return mat[0].data();
	}

	char* binary()
	{
		return mat[0].binary();
	}

	const char* binary() const
	{
		return mat[0].binary();
	}

	T& operator () (int32_t i)
	{
		return *(mat[0].data() + i);
	}

	const T& operator () (int32_t i) const
	{
		return *(mat[0].data() + i);
	}

	RowType& operator [] (int32_t i)
	{
		return mat[i];
	}

	const RowType& operator [] (int32_t i) const
	{
		return mat[i];
	}

	T& operator () (uint32_t i)
	{
		return *(mat[0].data() + i);
	}

	const T& operator () (uint32_t i) const
	{
		return *(mat[0].data() + i);
	}

	RowType& operator [] (uint32_t i)
	{
		return mat[i];
	}

	const RowType& operator [] (uint32_t i) const
	{
		return mat[i];
	}

	RowType column(int32_t c) const
	{
		return RowType(mat[0][c], mat[1][c], mat[2][c], mat[3][c]);
	}

	matrix4 operator * (T s) const
	{
		return matrix4(mat[0] * s, mat[1] * s, mat[2] * s, mat[3] * s);
	}

	matrix4 operator / (T s) const
	{
		return matrix4(mat[0] / s, mat[1] / s, mat[2] / s, mat[3] / s);
	}

	matrix4 operator + (const matrix4& m) const
	{
		return matrix4(mat[0] + m.mat[0], mat[1] + m.mat[1], mat[2] + m.mat[2], mat[3] + m.mat[3]);
	}

	matrix4 operator - (const matrix4& m) const
	{
		return matrix4(mat[0] - m.mat[0], mat[1] - m.mat[1], mat[2] - m.mat[2], mat[3] - m.mat[3]);
	}

	matrix4& operator /= (T m)
	{
		mat[0] /= m;
		mat[1] /= m;
		mat[2] /= m;
		mat[3] /= m;
		return *this;
	}

	vector3<T> operator * (const vector3<T>& v) const
	{
		vector3<T> r;
		r.x = mat[0].x * v.x + mat[1].x * v.y + mat[2].x * v.z + mat[3].x;
		r.y = mat[0].y * v.x + mat[1].y * v.y + mat[2].y * v.z + mat[3].y;
		r.z = mat[0].z * v.x + mat[1].z * v.y + mat[2].z * v.z + mat[3].z;
		T w = mat[0].w * v.x + mat[1].w * v.y + mat[2].w * v.z + mat[3].w;
		return (w*w > 0) ? r / w : r;
	}

	RowType operator * (const RowType& v) const
	{
		return RowType(
			mat[0].x * v.x + mat[1].x * v.y + mat[2].x * v.z + mat[3].x * v.w,
			mat[0].y * v.x + mat[1].y * v.y + mat[2].y * v.z + mat[3].y * v.w,
			mat[0].z * v.x + mat[1].z * v.y + mat[2].z * v.z + mat[3].z * v.w,
			mat[0].w * v.x + mat[1].w * v.y + mat[2].w * v.z + mat[3].w * v.w);
	}

	matrix4& operator += (const matrix4& m)
	{
		mat[0] += m.mat[0];
		mat[1] += m.mat[1];
		mat[2] += m.mat[2];
		mat[3] += m.mat[3];
		return *this;
	}

	matrix4& operator -= (const matrix4& m)
	{
		mat[0] -= m.mat[0];
		mat[1] -= m.mat[1];
		mat[2] -= m.mat[2];
		mat[3] -= m.mat[3];
		return *this;
	}

	matrix4& operator *= (const matrix4& m)
	{
		RowType r0, r1, r2, r3;

		r0.x = mat[0].x*m.mat[0].x + mat[0].y*m.mat[1].x + mat[0].z*m.mat[2].x + mat[0].w*m.mat[3].x;
		r0.y = mat[0].x*m.mat[0].y + mat[0].y*m.mat[1].y + mat[0].z*m.mat[2].y + mat[0].w*m.mat[3].y;
		r0.z = mat[0].x*m.mat[0].z + mat[0].y*m.mat[1].z + mat[0].z*m.mat[2].z + mat[0].w*m.mat[3].z;
		r0.w = mat[0].x*m.mat[0].w + mat[0].y*m.mat[1].w + mat[0].z*m.mat[2].w + mat[0].w*m.mat[3].w;
		r1.x = mat[1].x*m.mat[0].x + mat[1].y*m.mat[1].x + mat[1].z*m.mat[2].x + mat[1].w*m.mat[3].x;
		r1.y = mat[1].x*m.mat[0].y + mat[1].y*m.mat[1].y + mat[1].z*m.mat[2].y + mat[1].w*m.mat[3].y;
		r1.z = mat[1].x*m.mat[0].z + mat[1].y*m.mat[1].z + mat[1].z*m.mat[2].z + mat[1].w*m.mat[3].z;
		r1.w = mat[1].x*m.mat[0].w + mat[1].y*m.mat[1].w + mat[1].z*m.mat[2].w + mat[1].w*m.mat[3].w;
		r2.x = mat[2].x*m.mat[0].x + mat[2].y*m.mat[1].x + mat[2].z*m.mat[2].x + mat[2].w*m.mat[3].x;
		r2.y = mat[2].x*m.mat[0].y + mat[2].y*m.mat[1].y + mat[2].z*m.mat[2].y + mat[2].w*m.mat[3].y;
		r2.z = mat[2].x*m.mat[0].z + mat[2].y*m.mat[1].z + mat[2].z*m.mat[2].z + mat[2].w*m.mat[3].z;
		r2.w = mat[2].x*m.mat[0].w + mat[2].y*m.mat[1].w + mat[2].z*m.mat[2].w + mat[2].w*m.mat[3].w;
		r3.x = mat[3].x*m.mat[0].x + mat[3].y*m.mat[1].x + mat[3].z*m.mat[2].x + mat[3].w*m.mat[3].x;
		r3.y = mat[3].x*m.mat[0].y + mat[3].y*m.mat[1].y + mat[3].z*m.mat[2].y + mat[3].w*m.mat[3].y;
		r3.z = mat[3].x*m.mat[0].z + mat[3].y*m.mat[1].z + mat[3].z*m.mat[2].z + mat[3].w*m.mat[3].z;
		r3.w = mat[3].x*m.mat[0].w + mat[3].y*m.mat[1].w + mat[3].z*m.mat[2].w + mat[3].w*m.mat[3].w;

		mat[0] = r0;
		mat[1] = r1;
		mat[2] = r2;
		mat[3] = r3;

		return *this;
	}

	matrix4 operator * (const matrix4& m) const
	{
		RowType r0, r1, r2, r3;

		r0.x = mat[0].x*m.mat[0].x + mat[0].y*m.mat[1].x + mat[0].z*m.mat[2].x + mat[0].w*m.mat[3].x;
		r0.y = mat[0].x*m.mat[0].y + mat[0].y*m.mat[1].y + mat[0].z*m.mat[2].y + mat[0].w*m.mat[3].y;
		r0.z = mat[0].x*m.mat[0].z + mat[0].y*m.mat[1].z + mat[0].z*m.mat[2].z + mat[0].w*m.mat[3].z;
		r0.w = mat[0].x*m.mat[0].w + mat[0].y*m.mat[1].w + mat[0].z*m.mat[2].w + mat[0].w*m.mat[3].w;

		r1.x = mat[1].x*m.mat[0].x + mat[1].y*m.mat[1].x + mat[1].z*m.mat[2].x + mat[1].w*m.mat[3].x;
		r1.y = mat[1].x*m.mat[0].y + mat[1].y*m.mat[1].y + mat[1].z*m.mat[2].y + mat[1].w*m.mat[3].y;
		r1.z = mat[1].x*m.mat[0].z + mat[1].y*m.mat[1].z + mat[1].z*m.mat[2].z + mat[1].w*m.mat[3].z;
		r1.w = mat[1].x*m.mat[0].w + mat[1].y*m.mat[1].w + mat[1].z*m.mat[2].w + mat[1].w*m.mat[3].w;

		r2.x = mat[2].x*m.mat[0].x + mat[2].y*m.mat[1].x + mat[2].z*m.mat[2].x + mat[2].w*m.mat[3].x;
		r2.y = mat[2].x*m.mat[0].y + mat[2].y*m.mat[1].y + mat[2].z*m.mat[2].y + mat[2].w*m.mat[3].y;
		r2.z = mat[2].x*m.mat[0].z + mat[2].y*m.mat[1].z + mat[2].z*m.mat[2].z + mat[2].w*m.mat[3].z;
		r2.w = mat[2].x*m.mat[0].w + mat[2].y*m.mat[1].w + mat[2].z*m.mat[2].w + mat[2].w*m.mat[3].w;

		r3.x = mat[3].x*m.mat[0].x + mat[3].y*m.mat[1].x + mat[3].z*m.mat[2].x + mat[3].w*m.mat[3].x;
		r3.y = mat[3].x*m.mat[0].y + mat[3].y*m.mat[1].y + mat[3].z*m.mat[2].y + mat[3].w*m.mat[3].y;
		r3.z = mat[3].x*m.mat[0].z + mat[3].y*m.mat[1].z + mat[3].z*m.mat[2].z + mat[3].w*m.mat[3].z;
		r3.w = mat[3].x*m.mat[0].w + mat[3].y*m.mat[1].w + mat[3].z*m.mat[2].w + mat[3].w*m.mat[3].w;

		return matrix4(r0, r1, r2, r3);
	}

	bool operator == (const matrix4& m) const
	{
		return memcmp(mat, m.mat, sizeof(mat)) == 0;
	}

	bool operator != (const matrix4& m) const
	{
		return memcmp(mat, m.mat, sizeof(mat)) != 0;
	}

	T determinant() const
	{
		const T& a10 = mat[1].x; const T& a11 = mat[1].y; const T& a12 = mat[1].z; const T& a13 = mat[1].w;
		const T& a20 = mat[2].x; const T& a21 = mat[2].y; const T& a22 = mat[2].z; const T& a23 = mat[2].w;
		const T& a30 = mat[3].x; const T& a31 = mat[3].y; const T& a32 = mat[3].z; const T& a33 = mat[3].w;

		return mat[0].x * (a11 * (a22*a33 - a23*a32) + a12 * (a31*a23 - a21*a33) + a13 * (a21*a32 - a22*a31)) +
			mat[0].y * (a10 * (a23*a32 - a22*a33) + a20 * (a12*a33 - a13*a32) + a30 * (a13*a22 - a12*a23)) +
			mat[0].z * (a10 * (a21*a33 - a31*a23) + a11 * (a30*a23 - a20*a33) + a13 * (a20*a31 - a21*a30)) +
			mat[0].w * (a10 * (a22*a31 - a21*a32) + a11 * (a20*a32 - a30*a22) + a12 * (a21*a30 - a20*a31));
	}

	matrix3<T> mat3() const
	{
		return matrix3<T>(mat[0].xyz(), mat[1].xyz(), mat[2].xyz());
	}

	matrix3<T> subMatrix(int r, int c) const
	{
		matrix3<T> m;

		int i = 0;
		for (int y = 0; y < 4; ++y)
		{
			if (y != r)
			{
				int j = 0;
				if (0 != c) m[i][j++] = mat[y].x;
				if (1 != c) m[i][j++] = mat[y].y;
				if (2 != c) m[i][j++] = mat[y].z;
				if (3 != c) m[i][j++] = mat[y].w;
				++i;
			}
		}
		return m;
	}

	matrix4 adjugateMatrix() const
	{
		matrix4 m;

		m[0].x = subMatrix(0, 0).determinant();
		m[0].y = -subMatrix(1, 0).determinant();
		m[0].z = subMatrix(2, 0).determinant();
		m[0].w = -subMatrix(3, 0).determinant();

		m[1].x = -subMatrix(0, 1).determinant();
		m[1].y = subMatrix(1, 1).determinant();
		m[1].z = -subMatrix(2, 1).determinant();
		m[1].w = subMatrix(3, 1).determinant();

		m[2].x = subMatrix(0, 2).determinant();
		m[2].y = -subMatrix(1, 2).determinant();
		m[2].z = subMatrix(2, 2).determinant();
		m[2].w = -subMatrix(3, 2).determinant();

		m[3].x = -subMatrix(0, 3).determinant();
		m[3].y = subMatrix(1, 3).determinant();
		m[3].z = -subMatrix(2, 3).determinant();
		m[3].w = subMatrix(3, 3).determinant();

		return m;
	}

	vector3<T> rotationMultiply(const vector3<T>& v) const
	{
		const RowType& c0 = mat[0];
		const RowType& c1 = mat[1];
		const RowType& c2 = mat[2];
		return vector3<T>(c0.x * v.x + c1.x * v.y + c2.x * v.z, c0.y * v.x + c1.y * v.y + c2.y * v.z,
			c0.z * v.x + c1.z * v.y + c2.z * v.z);
	}

	matrix4 inverted() const
	{
		T det = determinant();
		return (det * det > 0) ? adjugateMatrix() / det : matrix4(0);
	}

	matrix4 transposed() const
	{
		matrix4 result;
		for (uint32_t r = 0; r < 4; ++r)
		{
			for (uint32_t c = 0; c < 4; ++c)
			{
				result.mat[r][c] = mat[c][r];
			}
		}
		return result;
	}

	bool isOrthonormal()
	{
		vector3<T>c1 = vector3<T>(mat[0][0], mat[1][0], mat[2][0]);
		vector3<T>c2 = vector3<T>(mat[0][1], mat[1][1], mat[2][1]);
		vector3<T>c3 = vector3<T>(mat[0][2], mat[1][2], mat[2][2]);
		vector3<T>c4 = vector3<T>(mat[0][3], mat[1][3], mat[2][3]);
		T l1 = c1.length();
		T l2 = c2.length();
		T l3 = c3.length();
		T l4 = c4.length();
		T c1dotc2 = c1.dot(c2);
		T c1dotc3 = c1.dot(c3);
		T c2dotc3 = c2.dot(c3);
		const T one = T(1);
		return (l1 == one) && (l2 = one) && (l3 = one) && (l4 = 0) && (c1dotc2 == 0) && (c1dotc3 == 0) && (c2dotc3 == 0);
	}

	matrix4 scaleWithPreservedPosition(const vector3<T>& p)
	{
		RowType r1 = RowType(mat[0].xyz() * p, mat[0].w);
		RowType r2 = RowType(mat[1].xyz() * p, mat[1].w);
		RowType r3 = RowType(mat[2].xyz() * p, mat[2].w);
		return matrix4(r1, r2, r3, mat[3]);
	}

	void clear()
	{
		mat[0].clear();
		mat[1].clear();
		mat[2].clear();
		mat[3].clear();
	}
};

template <typename T>
inline matrix4<T> translationMatrix(T x, T y, T z)
{
	matrix4<T> M(T(1));
	M[0][0] = M[1][1] = M[2][2] = static_cast<T>(1);
	M[3] = vector4<T>(x, y, z, static_cast<T>(1));
	return M;
}

template <typename T>
inline matrix4<T> translationScaleMatrix(T tx, T ty, T tz, T sx, T sy, T sz)
{
	matrix4<T> M(T(1));
	M[0][0] = sx;
	M[1][1] = sy;
	M[2][2] = sz;
	M[3] = vector4<T>(tx, ty, tz, static_cast<T>(1));
	return M;
}

template <typename T>
inline matrix4<T> scaleMatrix(T x, T y, T z)
{
	matrix4<T> M(T(1));
	M[0][0] = x;
	M[1][1] = y;
	M[2][2] = z;
	M[3][3] = static_cast<T>(1);
	return M;
}

template <typename T>
inline matrix4<T> rotationYXZMatrix(T x, T y, T z)
{
	matrix4<T> m(T(1));

	float sx = std::sin(x);
	float cx = std::cos(x);
	float sy = std::sin(y);
	float cy = std::cos(y);
	float sz = std::sin(z);
	float cz = std::cos(z);

	m[0][0] = cz*cy - sz*sx*sy; m[0][1] = -cx*sz; m[0][2] = cz*sy + sz*sx*cy;
	m[1][0] = sz*cy + cz*sx*sy; m[1][1] = cx*cz; m[1][2] = sz*sy - cz*sx*cy;
	m[2][0] = -cx*sy;            m[2][1] = sx;    m[2][2] = cx*cy;

	return m;
}

template <typename T>
inline matrix4<T> translationRotationYXZMatrix(T tx, T ty, T tz, T rx, T ry, T rz)
{
	matrix4<T> m(T(1));

	float sx = std::sin(rx);
	float cx = std::cos(rx);
	float sy = std::sin(ry);
	float cy = std::cos(ry);
	float sz = std::sin(rz);
	float cz = std::cos(rz);

	m[0][0] = cz*cy - sz*sx*sy; m[0][1] = -cx*sz; m[0][2] = cz*sy + sz*sx*cy;
	m[1][0] = sz*cy + cz*sx*sy; m[1][1] = cx*cz; m[1][2] = sz*sy - cz*sx*cy;
	m[2][0] = -cx*sy;            m[2][1] = sx;    m[2][2] = cx*cy;
	m[3][0] = tx;				 m[3][1] = ty;    m[3][2] = tz;
	m[3][3] = 1;

	return m;
}

template <typename T>
inline matrix4<T> rotationScaleMatrix(T rx, T ry, T rz, T scx, T scy, T scz)
{
	matrix4<T> m(T(1));

	float sx = std::sin(rx);
	float cx = std::cos(rx);
	float sy = std::sin(ry);
	float cy = std::cos(ry);
	float sz = std::sin(rz);
	float cz = std::cos(rz);

	m[0][0] = scx * (cz*cy - sz*sx*sy);
	m[0][1] = scy * (-cx*sz);
	m[0][2] = scz * (cz*sy + sz*sx*cy);

	m[1][0] = scx * (sz*cy + cz*sx*sy);
	m[1][1] = scy * (cx*cz);
	m[1][2] = scz * (sz*sy - cz*sx*cy);

	m[2][0] = scx * (-cx*sy);
	m[2][1] = scy * (sx);
	m[2][2] = scz * (cx*cy);
	m[3][3] = 1;

	return m;
}

template <typename T>
inline matrix4<T> transformYXZMatrix(T tx, T ty, T tz, T rx, T ry, T rz)
{
	matrix4<T> m(T(1));

	float sx = std::sin(rx);
	float cx = std::cos(rx);
	float sy = std::sin(ry);
	float cy = std::cos(ry);
	float sz = std::sin(rz);
	float cz = std::cos(rz);

	m[0][0] = cz*cy - sz*sx*sy; m[0][1] = -cx*sz; m[0][2] = cz*sy + sz*sx*cy;
	m[1][0] = sz*cy + cz*sx*sy; m[1][1] = cx*cz; m[1][2] = sz*sy - cz*sx*cy;
	m[2][0] = -cx*sy;            m[2][1] = sx;    m[2][2] = cx*cy;
	m[3][0] = tx;               m[3][1] = ty;    m[3][2] = tz;

	m[3][3] = 1;

	return m;
}

inline matrix4<float> translationMatrix(const vector3<float>& v)
{
	return translationMatrix<float>(v.x, v.y, v.z);
}

inline matrix4<float> translationScaleMatrix(const vector3<float>& t, const vector3<float>& s)
{
	return translationScaleMatrix<float>(t.x, t.y, t.z, s.x, s.y, s.z);
}

inline matrix4<float> scaleMatrix(const vector3<float>& v)
{
	return scaleMatrix<float>(v.x, v.y, v.z);
}

inline matrix4<float> rotationYXZMatrix(const vector3<float>& v)
{
	return rotationYXZMatrix<float>(v.x, v.y, v.z);
}

inline matrix4<float> rotationScaleMatrix(const vector3<float>& r, const vector3<float>& s)
{
	return rotationScaleMatrix<float>(r.x, r.y, r.z, s.x, s.y, s.z);
}

inline matrix4<float> translationRotationYXZMatrix(const vector3<float>& t, const vector3<float>& r)
{
	return translationRotationYXZMatrix<float>(t.x, t.y, t.z, r.x, r.y, r.z);
}

inline matrix4<float> transformYXZMatrix(vector3<float> translate, vector3<float> rotate)
{
	return transformYXZMatrix<float>(translate.x, translate.y, translate.z, rotate.x, rotate.y, rotate.z);
}
}

/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et
{

	template<typename T>
	struct matrix3
	{
		vector3<T> mat[3];

		matrix3()
			{ mat[0] = mat[1] = mat[2] = vector3<T>(0); }

		matrix3(const vector3<T>& c0, const vector3<T>& c1, const vector3<T>& c2) 
			{ mat[0] = c0; mat[1] = c1; mat[2] = c2; }

		matrix3(T s)
		{
			mat[0] = vector3<T>(s, 0, 0);
			mat[1] = vector3<T>(0, s, 0);
			mat[2] = vector3<T>(0, 0, s);
		}

		matrix3(T a00, T a01, T a02, T a10, T a11, T a12, T a20, T a21, T a22)
		{
			mat[0] = vector3<T>(a00, a01, a02);
			mat[1] = vector3<T>(a10, a11, a12);
			mat[2] = vector3<T>(a20, a21, a22);
		}

		T* data()
			{ return mat[0].data(); }
		
		const T* data() const
			{ return mat[0].data(); }
		
		char* binary()
			{ return mat[0].binary(); }
		
		const char* binary() const
			{ return mat[0].binary(); }
		
		vector3<T>& operator [] (int i)
			{ return mat[i]; }

		const vector3<T>& operator [] (int i) const
			{ return mat[i]; }

		T& operator () (int i)
			{ return *(&mat[0].x + i); }

		const T& operator () (int i) const
			{ return *(&mat[0].x + i); }

		vector3<T>& operator [] (size_t i)
			{ return mat[i]; }
		
		const vector3<T>& operator [] (size_t i) const
			{ return mat[i]; }
		
		T& operator () (size_t i)
			{ return *(&mat[0].x + i); }
		
		const T& operator() (size_t i) const
			{ return *(&mat[0].x + i); }
		
		matrix3<T>& operator += (const matrix3<T>& r)
		{
			mat[0] += r.mat[0];
			mat[1] += r.mat[1];
			mat[2] += r.mat[2];
			return *this;
		}
		
		T determinant()
		{
			T a10 = mat[1].x;
			T a11 = mat[1].y;
			T a12 = mat[1].z;
			T a20 = mat[2].x;
			T a21 = mat[2].y;
			T a22 = mat[2].z;
			return mat[0].x * (a11*a22 - a12*a21) +  mat[0].y * (a20*a12 - a10*a22) + mat[0].z * (a10*a21 - a11*a20);
		}

		vector3<T> operator * (const vector3<T>& r) const
		{
			return vector3<T>( 
				mat[0][0] * r.x + mat[1][0] * r.y + mat[2][0] * r.z, 
				mat[0][1] * r.x + mat[1][1] * r.y + mat[2][1] * r.z, 
				mat[0][2] * r.x + mat[1][2] * r.y + mat[2][2] * r.z );
		}

		matrix3& operator *= (const matrix3& m)
		{
			vector3<T> r0, r1, r2;
			r0.x = mat[0][0]*m.mat[0][0] + mat[0][1]*m.mat[1][0] + mat[0][2]*m.mat[2][0];
			r0.y = mat[0][0]*m.mat[0][1] + mat[0][1]*m.mat[1][1] + mat[0][2]*m.mat[2][1];
			r0.z = mat[0][0]*m.mat[0][2] + mat[0][1]*m.mat[1][2] + mat[0][2]*m.mat[2][2];
			r1.x = mat[1][0]*m.mat[0][0] + mat[1][1]*m.mat[1][0] + mat[1][2]*m.mat[2][0];
			r1.y = mat[1][0]*m.mat[0][1] + mat[1][1]*m.mat[1][1] + mat[1][2]*m.mat[2][1];
			r1.z = mat[1][0]*m.mat[0][2] + mat[1][1]*m.mat[1][2] + mat[1][2]*m.mat[2][2];
			r2.x = mat[2][0]*m.mat[0][0] + mat[2][1]*m.mat[1][0] + mat[2][2]*m.mat[2][0];
			r2.y = mat[2][0]*m.mat[0][1] + mat[2][1]*m.mat[1][1] + mat[2][2]*m.mat[2][1];
			r2.z = mat[2][0]*m.mat[0][2] + mat[2][1]*m.mat[1][2] + mat[2][2]*m.mat[2][2];
			mat[0] = r0;
			mat[1] = r1;
			mat[2] = r2;
			return *this;
		}

		matrix3 operator * (const matrix3& m) const
		{
			vector3<T> r0, r1, r2;
			r0.x = mat[0][0]*m.mat[0][0] + mat[0][1]*m.mat[1][0] + mat[0][2]*m.mat[2][0];
			r0.y = mat[0][0]*m.mat[0][1] + mat[0][1]*m.mat[1][1] + mat[0][2]*m.mat[2][1];
			r0.z = mat[0][0]*m.mat[0][2] + mat[0][1]*m.mat[1][2] + mat[0][2]*m.mat[2][2];
			r1.x = mat[1][0]*m.mat[0][0] + mat[1][1]*m.mat[1][0] + mat[1][2]*m.mat[2][0];
			r1.y = mat[1][0]*m.mat[0][1] + mat[1][1]*m.mat[1][1] + mat[1][2]*m.mat[2][1];
			r1.z = mat[1][0]*m.mat[0][2] + mat[1][1]*m.mat[1][2] + mat[1][2]*m.mat[2][2];
			r2.x = mat[2][0]*m.mat[0][0] + mat[2][1]*m.mat[1][0] + mat[2][2]*m.mat[2][0];
			r2.y = mat[2][0]*m.mat[0][1] + mat[2][1]*m.mat[1][1] + mat[2][2]*m.mat[2][1];
			r2.z = mat[2][0]*m.mat[0][2] + mat[2][1]*m.mat[1][2] + mat[2][2]*m.mat[2][2];
			return matrix3(r0, r1, r2);
		}

		matrix3<T> transposed() const
		{
			return matrix3<T>(mat[0][0], mat[1][0], mat[2][0], mat[0][1], mat[1][1], mat[2][1],
				mat[0][2], mat[1][2], mat[2][2]);
		}

		vector3<T> column(int i) const
			{ return vector3<T>(mat[0][i], mat[1][i], mat[2][i]); }
		
		bool operator == (const matrix3<T>& m) const
			{ return (mat[0] == m.mat[0]) && (mat[1] == m.mat[1]) && (mat[2] == m.mat[2]); }
		
		bool operator != (const matrix3<T>& m) const
			{ return (mat[0] != m.mat[0]) || (mat[1] != m.mat[1]) || (mat[2] != m.mat[2]); }

		void clear()
		{
			mat[0].clear();
			mat[1].clear();
			mat[2].clear();
		}
	};

}

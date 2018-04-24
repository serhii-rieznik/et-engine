/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

namespace et {

template <typename T>
union vector2 {
	struct { T x, y; };
	T c[2];

	vector2() :
		x(static_cast<T>(0)), y(static_cast<T>(0)) {}

	explicit vector2(T value) :
		x(value), y(value) {}

	vector2(T _x, T _y) :
		x(_x), y(_y) {}

	T& operator [](unsigned long i) {
		ET_ASSERT(i < 2); return c[i];
	}

	const T& operator [](unsigned long i) const {
		ET_ASSERT(i < 2); return c[i];
	}

	T* data() {
		return c;
	}

	const T* data() const {
		return c;
	}

	char* binary() {
		return reinterpret_cast<char*>(c);
	}

	const char* binary() const {
		return reinterpret_cast<const char*>(c);
	}

	bool operator == (const vector2& value) const {
		return (value.x == x) && (value.y == y);
	}

	bool operator != (const vector2& value) const {
		return (value.x != x) || (value.y != y);
	}

	vector2 operator - () const {
		return vector2(-x, -y);
	}

	vector2 operator + (const vector2& value) const {
		return vector2(x + value.x, y + value.y);
	};

	vector2 operator - (const vector2& value) const {
		return vector2(x - value.x, y - value.y);
	};

	vector2 operator * (const vector2& value) const {
		return vector2(x * value.x, y * value.y);
	};

	vector2 operator / (const vector2& value) const {
		return vector2(x / value.x, y / value.y);
	};

	vector2 operator * (const T& value) const {
		return vector2(x * value, y * value);
	};

	vector2 operator / (const T& value) const {
		return vector2(x / value, y / value);
	};

	vector2& operator += (const vector2 &value) {
		x += value.x; y += value.y; return *this;
	}

	vector2& operator -= (const vector2 &value) {
		x -= value.x; y -= value.y; return *this;
	}

	vector2& operator *= (const vector2 &value) {
		x *= value.x; y *= value.y; return *this;
	}

	vector2& operator /= (const vector2 &value) {
		x /= value.x; y /= value.y; return *this;
	}

	vector2& operator *= (T value) {
		x *= value; y *= value; return *this;
	}

	vector2& operator /= (T value) {
		x /= value; y /= value; return *this;
	}

	T dotSelf() const {
		return x * x + y * y;
	}

	T dot(const vector2<T>& v) const {
		return x * v.x + y * v.y;
	}

	T length() const {
		return std::sqrt(dotSelf());
	}

	T square() const {
		return x * y;
	}

	T aspect() const {
		return x / y;
	}

	void normalize() {
		T lenSquare = dotSelf();
		if (lenSquare > 0)
		{
			lenSquare = std::sqrt(lenSquare);
			x /= lenSquare;
			y /= lenSquare;
		}
	}

	vector2 normalized() const {
		T lenSquare = dotSelf();
		if (lenSquare > 0)
		{
			lenSquare = std::sqrt(lenSquare);
			return vector2(x / lenSquare, y / lenSquare);
		}
		return vector2();
	}

	vector2 yx() const {
		return vector2(y, x);
	}
};

template <typename T>
vector2<T> operator * (const vector2<T>& vec, T value) {
	return vector2<T>(vec.x * value, vec.y * value);
}

template <typename T>
vector2<T> operator * (T value, const vector2<T>& vec) {
	return vector2<T>(vec.x * value, vec.y * value);
}

template<typename T>
inline vector2<T> minv(const vector2<T>& v1, const vector2<T>& v2) {
	return vector2<T>(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
}

template<typename T>
inline vector2<T> maxv(const vector2<T>& v1, const vector2<T>& v2) {
	return vector2<T>(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
}

template<typename T>
inline vector2<T> absv(const vector2<T>& value) {
	return vector2<T>(std::abs(value.x), std::abs(value.y));
}

template <typename T>
inline vector2<T> floorv(const vector2<T>& v) {
	return vector2<T>(std::floor(v.x), std::floor(v.y));
}

template <typename T>
inline vector2<T> ceilv(const vector2<T>& v) {
	return vector2<T>(std::ceil(v.x), std::ceil(v.y));
}

template <typename T>
inline vector2<T> sqrtv(const vector2<T>& v) {
	return vector2<T>(std::sqrt(v.x), std::sqrt(v.y));
}

template <typename T>
inline T length(const vector2<T>& v) {
	return v.length();
}

template<typename T>
inline vector2<T> mix(const vector2<T>& v1, const vector2<T>& v2, const T& t) {
	return v1 * (static_cast<T>(1) - t) + v2 * t;
}

template<typename T>
inline vector2<T> lerp(const vector2<T>& v1, const vector2<T>& v2, const T& t) {
	return v1 * (static_cast<T>(1) - t) + v2 * t;
}

template <typename T>
inline vector2<T> normalize(const vector2<T>& v) {
	return v.normalized();
}

template <typename T>
inline T dot(const vector2<T>& v1, const vector2<T>& v2) {
	return v1.dot(v2);
}

template <typename T>
inline T outerProduct(const vector2<T>& v1, const vector2<T>& v2) {
	return v1.x * v2.y - v1.y * v2.x;
}

template <typename T>
inline vector2<float> vector2ToFloat(const vector2<T>& v) {
	return vector2<float>(static_cast<float>(v.x), static_cast<float>(v.y));
}

}

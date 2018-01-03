#pragma once

#if !defined(PBR_BASIC_INCLUDES)
#	error This file should not be included directly
#endif

namespace et {

namespace pbr {

class Camera
{
public:
	float ev() const;

private:
	float _aperture = 16.0f;
	float _shutterTime = 1.0f / 100.0f;
	float _iso = 100.0f;
};

inline float Camera::ev() const {
	return std::log2(_aperture * _aperture / _shutterTime * (_iso / 100.0f));
}

}

}
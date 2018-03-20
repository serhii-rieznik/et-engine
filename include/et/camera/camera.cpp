/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/camera/camera.h>

namespace et {

float Camera::renderingOriginTransform = 1.0f;
bool Camera::zeroClipRange = false;

Camera::Camera() :
	_viewMatrix(1.0f),
	_inverseViewMatrix(1.0f),
	_projectionMatrix(1.0f),
	_inverseProjectionMatrix(1.0f),
	_viewProjectionMatrix(1.0f),
	_inverseViewProjectionMatrix(1.0f) {

}

void Camera::lookAt(const vec3& pos, const vec3& point, const vec3& up) {
	vec3 d = (point - pos).normalized();
	vec3 s = d.cross(up).normalized();
	vec3 u = s.cross(d).normalized();
	vec3 e(-dot(s, pos), -dot(u, pos), dot(d, pos));

	_viewMatrix[0] = vec4(s.x, u.x, -d.x, 0.0f);
	_viewMatrix[1] = vec4(s.y, u.y, -d.y, 0.0f);
	_viewMatrix[2] = vec4(s.z, u.z, -d.z, 0.0f);
	_viewMatrix[3] = vec4(e.x, e.y, e.z, 1.0f);

	viewUpdated();
}

const mat4& Camera::perspectiveProjection(float fov, float aspect, float zNear, float zFar, bool reverseZ) {
	ET_ASSERT((reverseZ == false) || (reverseZ && zeroClipRange));

	_zFar = zFar;
	_zNear = zNear;
	_perspecitveAspect = aspect;
	_fov = fov;

	_projectionMatrix = identityMatrix;

	float halfFovTangent = std::tan(0.5f * _fov);

	_projectionMatrix[0][0] = 1.0f / (halfFovTangent * aspect);
	_projectionMatrix[1][1] = renderingOriginTransform / halfFovTangent;
	_projectionMatrix[2][3] = -1.0f;
	_projectionMatrix[3][3] = 0.0f;

	if (zeroClipRange)
	{
		if (reverseZ)
		{
			_projectionMatrix[2][2] = 0.0f; //  -1.0f / (_zFar - _zNear);
			_projectionMatrix[3][2] = zNear; // _zFar / (_zFar - _zNear);
		}
		else
		{
			_projectionMatrix[2][2] = -zFar / (_zFar - _zNear);
			_projectionMatrix[3][2] = -(_zFar * _zNear) / (_zFar - _zNear);
		}
	}
	else
	{
		_projectionMatrix[2][2] = -(_zFar + _zNear) / (_zFar - _zNear);
		_projectionMatrix[3][2] = -2.0f * (_zFar * _zNear) / (_zFar - _zNear);
	}

	projectionUpdated();
	return _projectionMatrix;
}

const mat4& Camera::customPerspectiveProjection(const vec2& fullFov, float zNear, float zFar) {
	_zFar = zFar;
	_zNear = zNear;
	_perspecitveAspect = 0.0f;
	_projectionMatrix = identityMatrix;

	vec2 fov = 0.5f * fullFov;
	float cotanX = std::cos(fov.x) / std::sin(fov.x);
	float cotanY = std::cos(fov.y) / std::sin(fov.y);
	float dz = zFar - zNear;

	_projectionMatrix[0][0] = cotanX;
	_projectionMatrix[1][1] = cotanY;
	_projectionMatrix[2][2] = -(zFar + zNear) / dz;
	_projectionMatrix[3][3] = 0.0f;
	_projectionMatrix[2][3] = -1.0f;
	_projectionMatrix[3][2] = -2.0f * zNear * zFar / dz;

	projectionUpdated();
	return _projectionMatrix;
}

const mat4& Camera::orthogonalProjection(float left, float right, float bottom, float top, float zNear, float zFar) {
	_projectionMatrix = identityMatrix;

	_projectionMatrix[0][0] = 2.0f / (right - left);
	_projectionMatrix[3][0] = (left + right) / (left - right);

	_projectionMatrix[1][1] = -2.0f / (top - bottom) * renderingOriginTransform;
	_projectionMatrix[3][1] = -(top + bottom) / (bottom - top) * renderingOriginTransform;

	_projectionMatrix[2][2] = (zeroClipRange ? 1.0f : 2.0f) / (zNear - zFar);
	_projectionMatrix[3][2] = (zNear + (zeroClipRange ? 0.0f : zFar)) / (zNear - zFar);

	projectionUpdated();
	return _projectionMatrix;
}

const mat4& Camera::windowProjection(const vec2& windowSize) {
	_projectionMatrix = identityMatrix;

	_projectionMatrix[0][0] = 2.0f / windowSize.x;
	_projectionMatrix[1][1] = -2.0f / windowSize.y;

	_projectionMatrix[3][0] = -1.0;
	_projectionMatrix[3][1] = 1.0f;
	_projectionMatrix[3][2] = 0.0f;

	projectionUpdated();
	return _projectionMatrix;
}

void Camera::setPosition(const vec3& p) {
	_viewMatrix[3] = vec4(-_viewMatrix.rotationMultiply(p), _viewMatrix[3][3]);
	viewUpdated();
}

void Camera::setDirection(const vec3& d) {
	vec3 u = up();
	vec3 s = cross(u, d).normalized();
	vec3 p = position();
	vec3 e(-dot(s, p), -dot(u, p), -dot(d, p));
	_viewMatrix[0] = vec4(s.x, u.x, d.x, 0.0f);
	_viewMatrix[1] = vec4(s.y, u.y, d.y, 0.0f);
	_viewMatrix[2] = vec4(s.z, u.z, d.z, 0.0f);
	_viewMatrix[3] = vec4(e.x, e.y, e.z, 1.0f);
	viewUpdated();
}

void Camera::setSide(const vec3& s) {
	vec3 u = up();
	vec3 d = normalize(s.cross(u));
	vec3 p = position();
	vec3 e(-dot(s, p), -dot(u, p), dot(d, p));
	_viewMatrix[0] = vec4(s.x, u.x, -d.x, 0.0f);
	_viewMatrix[1] = vec4(s.y, u.y, -d.y, 0.0f);
	_viewMatrix[2] = vec4(s.z, u.z, -d.z, 0.0f);
	_viewMatrix[3] = vec4(e.x, e.y, e.z, 1.0f);

	viewUpdated();
}

vec3 Camera::direction() const {
	return _viewMatrix.column(2).xyz();
}

vec3 Camera::up() const {
	return _viewMatrix.column(1).xyz();
}

vec3 Camera::side() const {
	return _viewMatrix.column(0).xyz();
}

vec3 Camera::invDirection() const {
	return _inverseViewMatrix.column(2).xyz();
}

vec3 Camera::invUp() const {
	return _inverseViewMatrix.column(1).xyz();
}

vec3 Camera::invSide() const {
	return _inverseViewMatrix.column(0).xyz();
}

float Camera::heading() const {
	return -std::asin(_viewMatrix[1][2]);
}

void Camera::move(const vec3& dp) {
	_viewMatrix[3] += vec4(_viewMatrix.rotationMultiply(dp), 0.0f);
	viewUpdated();
}

void Camera::rotate(const vec3& axis) {
	_viewMatrix *= rotationYXZMatrix(axis);
	viewUpdated();
}

void Camera::rotate(const quaternion& q) {
	_viewMatrix *= q.toMatrix();
	viewUpdated();
}

ray3d Camera::castRay(const vec2& pt) const {
	const vec3& pos = position();
	vec3 dir = unproject(vec3(pt, 1.0f));
	dir -= pos;
	dir.normalize();
	return ray3d(pos, dir);
}

void Camera::viewUpdated() {
	_inverseViewMatrix = _viewMatrix.inverted();

	if (_lockUpVector)
	{
		vec3 p = position();
		vec3 d = -direction();
		vec3 s = normalize(d.cross(_upLocked));
		vec3 u = normalize(s.cross(d));
		vec3 e(-dot(s, p), -dot(u, p), dot(d, p));
		_viewMatrix[0] = vec4(s.x, u.x, -d.x, 0.0f);
		_viewMatrix[1] = vec4(s.y, u.y, -d.y, 0.0f);
		_viewMatrix[2] = vec4(s.z, u.z, -d.z, 0.0f);
		_viewMatrix[3] = vec4(e.x, e.y, e.z, 1.0f);
		_inverseViewMatrix = _viewMatrix.inverted();
	}

	updateViewProjectionMatrix();
}

void Camera::projectionUpdated() {
	_inverseProjectionMatrix = _projectionMatrix.inverted();
	updateViewProjectionMatrix();
}

void Camera::updateViewProjectionMatrix() {
	_viewProjectionMatrix = _viewMatrix * _projectionMatrix;
	_inverseViewProjectionMatrix = _viewProjectionMatrix.inverted();
	_frustum.build(_inverseViewProjectionMatrix);
}

void Camera::lockUpVector(const vec3& u) {
	_upLocked = u;
	_lockUpVector = true;
	viewUpdated();
}

void Camera::unlockUpVector() {
	_lockUpVector = false;
}

void Camera::reflected(const plane& pl, Camera& result) const {
	vec3 s = normalize(reflect(side(), pl.normal()));
	vec3 u = normalize(reflect(up(), pl.normal()));
	vec3 d = normalize(reflect(direction(), pl.normal()));
	vec3 p = pl.reflect(position());

	mat4 mv = _viewMatrix;
	mv[0][0] = s.x; mv[0][1] = u.x; mv[0][2] = d.x;
	mv[1][0] = s.y; mv[1][1] = u.y; mv[1][2] = d.y;
	mv[2][0] = s.z; mv[2][1] = u.z; mv[2][2] = d.z;
	mv[3] = vec4(-mv.rotationMultiply(p), mv[3][3]);

	result.unlockUpVector();
	result.setViewMatrix(mv);
	result.setProjectionMatrix(_projectionMatrix);
}

vec3 Camera::project(const vec3& v) const {
	return viewProjectionMatrix() * v;
}

vec3 Camera::unproject(const vec3& v) const {
	return inverseViewProjectionMatrix() * v;
}

vec4 Camera::project(const vec4& v) const {
	return viewProjectionMatrix() * v;
}

vec4 Camera::unproject(const vec4& v) const {
	return inverseViewProjectionMatrix() * v;
}

void Camera::getValuesFromCamera(const Camera& r) {
#define COPY_PROP(PROP) PROP = r.PROP
	COPY_PROP(_viewMatrix);
	COPY_PROP(_inverseViewMatrix);
	COPY_PROP(_projectionMatrix);
	COPY_PROP(_inverseProjectionMatrix);
	COPY_PROP(_viewProjectionMatrix);
	COPY_PROP(_inverseViewProjectionMatrix);
	COPY_PROP(_frustum);
	COPY_PROP(_upLocked);
	COPY_PROP(_zNear);
	COPY_PROP(_zFar);
	COPY_PROP(_perspecitveAspect);
	COPY_PROP(_fov);
	COPY_PROP(_lockUpVector);
#undef COPY_PROP
}

BoundingBox::Corners Camera::frustumCornersWithDistance(float nearDistance, float farDistance) {
	BoundingBox::Corners result;

	vec3 s = side();
	vec3 u = up();
	vec3 d = direction();
	vec3 o = position();
	float sideScale = 1.0f / _projectionMatrix[0][0];
	float upScale = renderingOriginTransform / _projectionMatrix[1][1];

	result[0] = o + nearDistance * (d - s * sideScale - u * upScale);
	result[1] = o + nearDistance * (d - s * sideScale + u * upScale);
	result[2] = o + nearDistance * (d + s * sideScale + u * upScale);
	result[3] = o + nearDistance * (d + s * sideScale - u * upScale);
	result[4] = o + farDistance * (d - s * sideScale - u * upScale);
	result[5] = o + farDistance * (d - s * sideScale + u * upScale);
	result[6] = o + farDistance * (d + s * sideScale + u * upScale);
	result[7] = o + farDistance * (d + s * sideScale - u * upScale);

	return result;
}

CubemapProjectionMatrixArray cubemapMatrixProjectionArray(const mat4& proj, const vec3& point) {
	CubemapProjectionMatrixArray result;

	const vec4& rX = proj[0];
	const vec4& rY = proj[1] * Camera::renderingOriginTransform;
	const vec4& rZ = proj[2];
	const vec4& rW = proj[3];

	mat4 translation = translationMatrix(-point);
	result[0] = translation * mat4(-rZ, -rY, -rX, rW);
	result[1] = translation * mat4(rZ, -rY, rX, rW);
	result[2] = translation * mat4(rX, -rZ, rY, rW);
	result[3] = translation * mat4(rX, rZ, -rY, rW);
	result[4] = translation * mat4(rX, -rY, -rZ, rW);
	result[5] = translation * mat4(-rX, -rY, rZ, rW);

	return result;
}

}

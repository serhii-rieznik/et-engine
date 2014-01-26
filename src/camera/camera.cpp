/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/camera/camera.h>

using namespace et;

Camera::Camera() : _modelViewMatrix(1.0f), _projectionMatrix(1.0f), _mvpMatrix(1.0f), _inverseModelViewMatrix(1.0f), 
	_inverseProjectionMatrix(1.0f),	_inverseMVPMatrix(1.0f), _lockUpVector(false)
{

}

void Camera::lookAt(const vec3& pos, const vec3& point, const vec3& up)
{
	vec3 d = (point - pos).normalized();
	vec3 s = d.cross(up).normalized();
	vec3 u = s.cross(d).normalized();
	vec3 e(-dot(s, pos), -dot(u, pos), dot(d, pos));

	_modelViewMatrix[0] = vec4(s.x, u.x, -d.x, 0.0f);
	_modelViewMatrix[1] = vec4(s.y, u.y, -d.y, 0.0f);
	_modelViewMatrix[2] = vec4(s.z, u.z, -d.z, 0.0f);
	_modelViewMatrix[3] = vec4(e.x, e.y,  e.z, 1.0f);

	modelViewUpdated();
}

const mat4& Camera::perspectiveProjection(float fov, float aspect, float zNear, float zFar)
{
	_projectionMatrix = identityMatrix;

	float fHalfFOV = 0.5f * fov;
	float cotan = std::cos(fHalfFOV) / std::sin(fHalfFOV);
	float dz = zFar - zNear;

	_projectionMatrix[0][0] = cotan / aspect;
	_projectionMatrix[1][1] = cotan;
	_projectionMatrix[2][2] = -(zFar + zNear) / dz;
	_projectionMatrix[3][3] =  0.0f;
	_projectionMatrix[2][3] = -1.0f;
	_projectionMatrix[3][2] = -2.0f * zNear * zFar / dz;

	projectionUpdated();
	return _projectionMatrix;
}

const mat4& Camera::customPerspectiveProjection(const vec2& fullFov, float zNear, float zFar)
{
	_projectionMatrix = identityMatrix;
	
	vec2 fov = 0.5f * fullFov;
	float cotanX = std::cos(fov.x) / std::sin(fov.x);
	float cotanY = std::cos(fov.y) / std::sin(fov.y);
	float dz = zFar - zNear;
	
	_projectionMatrix[0][0] = cotanX;
	_projectionMatrix[1][1] = cotanY;
	_projectionMatrix[2][2] = -(zFar + zNear) / dz;
	_projectionMatrix[3][3] =  0.0f;
	_projectionMatrix[2][3] = -1.0f;
	_projectionMatrix[3][2] = -2.0f * zNear * zFar / dz;
	
	projectionUpdated();
	return _projectionMatrix;
}

const mat4& Camera::orthogonalProjection(float left, float right, float top, float bottom, float zNear, float zFar)
{
	_projectionMatrix = identityMatrix;

	_projectionMatrix[0][0] = 2.0f / (right - left);
	_projectionMatrix[1][1] = 2.0f / (top - bottom);
	_projectionMatrix[2][2] = -2.0f / (zFar - zNear);

	_projectionMatrix[3][0] = -(right + left) / (right - left);
	_projectionMatrix[3][1] = -(top + bottom) / (top - bottom);
	_projectionMatrix[3][2] = -(zFar + zNear) / (zFar - zNear);

	projectionUpdated();
	return _projectionMatrix;
}

const mat4& Camera::windowProjection(const vec2& windowSize)
{
	_projectionMatrix = identityMatrix;
	
	_projectionMatrix[0][0] = 2.0f / windowSize.x;
	_projectionMatrix[1][1] = -2.0f / windowSize.y;
	
	_projectionMatrix[3][0] = -1.0;
	_projectionMatrix[3][1] = 1.0f;
	_projectionMatrix[3][2] = 0.0f;
	
	projectionUpdated();
	return _projectionMatrix;
}

void Camera::setPosition(const vec3& p)
{
	_modelViewMatrix[3] = vec4(-_modelViewMatrix.rotationMultiply(p), _modelViewMatrix[3][3]);
	modelViewUpdated();
}

void Camera::setDirection(const vec3& d)
{
	vec3 u = up();
	vec3 s = normalize(d.cross(u));
	vec3 p = position();
	vec3 e(-dot(s, p), -dot(u, p), dot(d, p));
	_modelViewMatrix[0] = vec4(s.x, u.x, -d.x, 0.0f);
	_modelViewMatrix[1] = vec4(s.y, u.y, -d.y, 0.0f);
	_modelViewMatrix[2] = vec4(s.z, u.z, -d.z, 0.0f);
	_modelViewMatrix[3] = vec4(e.x, e.y,  e.z, 1.0f);
	modelViewUpdated();
}

void Camera::setSide(const vec3& s)
{
	vec3 u = up();
	vec3 d = normalize(s.cross(u));
	vec3 p = position();
	vec3 e(-dot(s, p), -dot(u, p), dot(d, p));
	_modelViewMatrix[0] = vec4(s.x, u.x, -d.x, 0.0f);
	_modelViewMatrix[1] = vec4(s.y, u.y, -d.y, 0.0f);
	_modelViewMatrix[2] = vec4(s.z, u.z, -d.z, 0.0f);
	_modelViewMatrix[3] = vec4(e.x, e.y,  e.z, 1.0f);
	modelViewUpdated();
}

vec3 Camera::direction() const
{
	return _modelViewMatrix.column(2).xyz();
}

vec3 Camera::up() const
{
	return _modelViewMatrix.column(1).xyz();
}

vec3 Camera::side() const
{
	return _modelViewMatrix.column(0).xyz();
}

vec3 Camera::invDirection() const
{
	return _inverseModelViewMatrix.column(2).xyz();
}

vec3 Camera::invUp() const
{
	return _inverseModelViewMatrix.column(1).xyz();
}

vec3 Camera::invSide() const
{
	return _inverseModelViewMatrix.column(0).xyz();
}

float Camera::heading() const
{
	return -std::asin(_modelViewMatrix[1][2]);
}

void Camera::move(const vec3& dp)
{
	_modelViewMatrix[3] += vec4(_modelViewMatrix.rotationMultiply(dp), 0.0f);
	modelViewUpdated();
}

void Camera::rotate(const vec3& axis)
{
	_modelViewMatrix *= rotationYXZMatrix(axis);
	modelViewUpdated();
}

void Camera::rotate(const quaternion& q)
{
	_modelViewMatrix *= q.toMatrix();
	modelViewUpdated();
}

ray3d Camera::castRay(const vec2& pt) const
{
	vec3 pos = position();
	return ray3d(pos, normalize(unproject(vec3(pt, 1.0f)) - pos));
}

void Camera::modelViewUpdated()
{
	_inverseModelViewMatrix = _modelViewMatrix.inverse();
	if (_lockUpVector)
	{
		vec3 p = position();
		vec3 d = -direction();
		vec3 s = normalize(d.cross(_upLocked));
		vec3 u = normalize(s.cross(d));
		vec3 e(-dot(s, p), -dot(u, p), dot(d, p));
		_modelViewMatrix[0] = vec4(s.x, u.x, -d.x, 0.0f);
		_modelViewMatrix[1] = vec4(s.y, u.y, -d.y, 0.0f);
		_modelViewMatrix[2] = vec4(s.z, u.z, -d.z, 0.0f);
		_modelViewMatrix[3] = vec4(e.x, e.y,  e.z, 1.0f);
		_inverseModelViewMatrix = _modelViewMatrix.inverse();

	}
	updateMVP();
}

void Camera::projectionUpdated()
{
	_inverseProjectionMatrix = _projectionMatrix.inverse();
	updateMVP();
}

void Camera::updateMVP()
{
	_mvpMatrix = _modelViewMatrix * _projectionMatrix;
	_inverseMVPMatrix = _mvpMatrix.inverse();
	_frustum = Frustum(_mvpMatrix);
}

void Camera::lockUpVector(const vec3& u)
{
	_upLocked = u;
	_lockUpVector = true;
	modelViewUpdated();
}

void Camera::unlockUpVector()
{
	_lockUpVector = false;
}

Camera Camera::reflected(const plane& pl) const
{
	Camera result(*this);
	
	vec3 s = normalize(reflect(side(), pl.normal()));
	vec3 u = normalize(reflect(up(), pl.normal()));
	vec3 d = normalize(reflect(direction(), pl.normal()));
	vec3 p = pl.reflect(position());
	
	mat4 mv = _modelViewMatrix;
	mv[0][0] = s.x; mv[0][1] = u.x; mv[0][2] = d.x;
	mv[1][0] = s.y; mv[1][1] = u.y; mv[1][2] = d.y;
	mv[2][0] = s.z; mv[2][1] = u.z; mv[2][2] = d.z;
	mv[3] = vec4(-mv.rotationMultiply(p), mv[3][3]);
	
	result.unlockUpVector();
 	result.setModelViewMatrix(mv);
	
	return result;
}

vec3 Camera::project(const vec3& v) const
{
	return modelViewProjectionMatrix() * v;
}

vec3 Camera::unproject(const vec3& v) const
{
	return inverseModelViewProjectionMatrix() * v;
}

vec4 Camera::project(const vec4& v) const
{
	return modelViewProjectionMatrix() * v;
}

vec4 Camera::unproject(const vec4& v) const
{
	return inverseModelViewProjectionMatrix() * v;
}

CubemapProjectionMatrixArray et::cubemapMatrixProjectionArray(const mat4& proj, const vec3& point)
{
	CubemapProjectionMatrixArray result;

	const vec4& rX = proj[0]; 
	const vec4& rY = proj[1];
	const vec4& rZ = proj[2];
	const vec4& rW = proj[3];

	mat4 translation = translationMatrix(-point);
	result[0] = translation * mat4( -rZ, -rY, -rX, rW );
	result[1] = translation * mat4(  rZ, -rY,  rX, rW );
	result[2] = translation * mat4(  rX, -rZ,  rY, rW );
	result[3] = translation * mat4(  rX,  rZ, -rY, rW );
	result[4] = translation * mat4(  rX, -rY, -rZ, rW );
	result[5] = translation * mat4( -rX, -rY,  rZ, rW );

	return result;
}

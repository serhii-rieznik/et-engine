/*
* This file is part of `et engine`
* Copyright 2009-2013 by Sergey Reznik
* Please, do not modify content without approval.
*
*/

#include <et/geometry/geometry.h>
#include <et/core/transformable.h>

using namespace et;

ComponentTransformable::ComponentTransformable() :
	_cachedTransform(identityMatrix), _translation(0.0f), _scale(1.0f), _orientation(), _flags(0)
{
}

mat4 ComponentTransformable::transform()
{
	if (!transformValid())
		buildTransform();

	return _cachedTransform;
}

void ComponentTransformable::buildTransform()
{
	if (!shouldDecompose())
	{
		_cachedTransform = _orientation.toMatrix() * scaleMatrix(_scale);
		_cachedTransform[3] = vec4(_translation, 1.0f);
	}

	_flags |= Flag_Valid;
}

void ComponentTransformable::invalidateTransform()
{
	_flags &= ~Flag_Valid;
}

void ComponentTransformable::setTranslation(const vec3& t)
{
	if (shouldDecompose())
		setTransform(_cachedTransform);

	_translation = t;
	invalidateTransform();
}

void ComponentTransformable::applyTranslation(const vec3& t)
{
	if (shouldDecompose())
		setTransform(_cachedTransform);

	_translation += t;
	invalidateTransform();
}

void ComponentTransformable::setScale(const vec3& s)
{
	if (shouldDecompose())
		setTransform(_cachedTransform);

	_scale = s;
	invalidateTransform();
}

void ComponentTransformable::applyScale(const vec3& s)
{
	if (shouldDecompose())
		setTransform(_cachedTransform);

	_scale *= s;
	invalidateTransform();
}

void ComponentTransformable::setOrientation(const quaternion& q)
{
	if (shouldDecompose())
		setTransform(_cachedTransform);

	_orientation = q;
	invalidateTransform();
}

void ComponentTransformable::applyOrientation(const quaternion& q)
{
	if (shouldDecompose())
		setTransform(_cachedTransform);

	_orientation *= q;
	invalidateTransform();
}

const mat4& ComponentTransformable::cachedTransform() const
{
	assert(transformValid());
	return _cachedTransform; 
}

void ComponentTransformable::setTransform(const mat4& originalMatrix)
{
	_flags &= ~Flag_ShouldDecompose;
	decomposeMatrix(originalMatrix, _translation, _orientation, _scale);
	invalidateTransform();

#if (ET_DEBUG)
	buildTransform();

	float deviation = 0.0f;
	for (size_t v = 0; v < 4; ++v)
		for (size_t u = 0; u < 4; ++u)
			deviation += sqr(originalMatrix[v][u] - _cachedTransform[v][u]);

	if (deviation > 0.01f)
	{
		log::warning("Failed to decompose matrix\n{\n"
			"\tscale: (%f %f %f)\n"
			"\torientation: (%f %f %f %f)\n"
			"\ttranslation: (%f %f %f)\n"
			"\tdeviation: %f\n"
			"\toriginal matrix and result matrices:\n"
			 "\t\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\n"
			 "\t\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\n"
			 "\t\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\n"
			 "\t\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\t(%3.5f\t%3.5f\t%3.5f\t%3.5f)\n"
			"}", _scale.x, _scale.y, _scale.z, _orientation.scalar,
			_orientation.vector.x, _orientation.vector.y, _orientation.vector.z,
			_translation.x, _translation.y, _translation.z, deviation,
			 originalMatrix[0][0], originalMatrix[0][1], originalMatrix[0][2], originalMatrix[0][3],
			 _cachedTransform[0][0], _cachedTransform[0][1], _cachedTransform[0][2], _cachedTransform[0][3],
			 originalMatrix[1][0], originalMatrix[1][1], originalMatrix[1][2], originalMatrix[1][3],
			 _cachedTransform[1][0], _cachedTransform[1][1], _cachedTransform[1][2], _cachedTransform[1][3],
			 originalMatrix[2][0], originalMatrix[2][1], originalMatrix[2][2], originalMatrix[2][3],
			 _cachedTransform[2][0], _cachedTransform[2][1], _cachedTransform[2][2], _cachedTransform[2][3],
			 originalMatrix[3][0], originalMatrix[3][1], originalMatrix[3][2], originalMatrix[3][3],
			 _cachedTransform[3][0], _cachedTransform[3][1], _cachedTransform[3][2], _cachedTransform[3][3]);
	}
#endif
}

void ComponentTransformable::setTransformDirectly(const mat4& m)
{
	_cachedTransform = m;
	_flags |= Flag_ShouldDecompose;
	invalidateTransform();
}

const vec3& ComponentTransformable::translation() const
{ 
	return shouldDecompose() ? _cachedTransform[3].xyz() : _translation;
}

const vec3& ComponentTransformable::scale()
{ 
	if (shouldDecompose())
		setTransform(_cachedTransform);

	return _scale; 
}

const quaternion& ComponentTransformable::orientation()
{
	if (shouldDecompose())
		setTransform(_cachedTransform);

	return _orientation; 
}
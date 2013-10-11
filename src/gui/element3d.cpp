/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/gui/guirenderer.h>
#include <et/gui/element3d.h>

using namespace et;
using namespace et::gui;

Element3D::Element3D(const Camera& camera, Element* parent, const std::string& name) : Element(parent, name),
	_camera(camera), _transform(identityMatrix), _finalTransform(identityMatrix), 
	_inverseFinalTransform(identityMatrix), _finalTransformValid(true), _inverseFinalTransformValid(true)
{

}

const mat4& Element3D::finalTransform()
{
	if (!_finalTransformValid)
		buildFinalTransform();

	return _finalTransform;
}

const mat4& Element3D::inverseFinalTransform()
{
	if (!_inverseFinalTransformValid)
	{
		_inverseFinalTransform = finalTransform().inverse();
		_inverseFinalTransformValid = true;
	}

	return _inverseFinalTransform;
}

void Element3D::buildFinalTransform()
{
	_finalTransform = parent() ? _transform * parent()->finalTransform() : _transform;
	_finalTransformValid = false;
	_inverseFinalTransformValid = false;
}

void Element3D::setTransform(const mat4& t)
{
	_transform = t;
	_finalTransformValid = false;
	_inverseFinalTransformValid = false;
}

void Element3D::applyTransform(const mat4& t)
{
	_transform *= t;
	_finalTransformValid = false;
	_inverseFinalTransformValid = false;
}

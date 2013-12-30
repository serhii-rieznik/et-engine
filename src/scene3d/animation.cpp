/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#include <et/scene3d/animation.h>

using namespace et;
using namespace et::s3d;

Animation::Animation()
{
	
}

Animation::Animation(const std::string& n) :
	_name(n)
{
}

void Animation::addKeyFrame(float, const mat4&)
{
}

const mat4& Animation::transformation(float)
{
	return identityMatrix;
}

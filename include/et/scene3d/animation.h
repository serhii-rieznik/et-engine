/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/geometry/geometry.h>
#include <et/core/transformable.h>

namespace et
{
	namespace s3d
	{
		class Animation
		{
		public:
			Animation();
			Animation(const std::string&);
			
			void addKeyFrame(float, const ComponentTransformable&);
			
			mat4 transformation(float);
			
			void transformation(float, vec3&, quaternion&, vec3&);
			
			void setTimeRange(float, float);
			
			void setFrameRate(float);
			
			float duration() const
				{ return _stopTime - _startTime; }
			
		private:
			std::string _name;
			
			std::vector<float> _times;
			std::vector<ComponentTransformable> _transformations;
			
			float _startTime = 0.0f;
			float _stopTime = 0.0f;
			float _frameRate = 0.0f;
			
			bool _loop = false;
		};
	}
}
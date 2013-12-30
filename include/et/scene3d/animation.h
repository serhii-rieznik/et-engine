/*
 * This file is part of `et engine`
 * Copyright 2009-2013 by Sergey Reznik
 * Please, do not modify content without approval.
 *
 */

#pragma once

#include <et/geometry/geometry.h>

namespace et
{
	namespace s3d
	{
		class Animation
		{
		public:
			Animation();
			Animation(const std::string&);
			
			void addKeyFrame(float, const mat4&);
			
			const mat4& transformation(float);
			
			float duration() const
				{ return _stopTime - _startTime; }
			
		private:
			std::string _name;
			float _startTime = 0.0f;
			float _stopTime = 0.0f;
			bool _loop = 0.0f;
		};
	}
}
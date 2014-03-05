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
			struct Frame
			{
				float time = 0.0f;
				
				vec3 translation;
				quaternion orientation;
				vec3 scale;
				
				Frame() { }
				
				Frame(float t, const vec3& tr, const quaternion& o, const vec3& s) :
					time(t), translation(tr), orientation(o), scale(s) { }
			};
			
			enum OutOfRangeMode
			{
				OutOfRangeMode_Loop,
				OutOfRangeMode_Once
			};
			
		public:
			Animation();
			Animation(std::istream&);
			
			void addKeyFrame(float, const vec3&, const quaternion&, const vec3&);
			
			mat4 transformation(float) const;
			
			void transformation(float, vec3&, quaternion&, vec3&) const;
						
			void setTimeRange(float, float);
			
			void setFrameRate(float);
			
			void setOutOfRangeMode(OutOfRangeMode);
			
			OutOfRangeMode outOfRangeMode() const
				{ return _outOfRangeMode; }
			
			float startTime() const
				{ return _startTime; }

			float stopTime() const
				{ return _stopTime; }
			
			float duration() const
				{ return _stopTime - _startTime; }
			
			void serialize(std::ostream&) const;
			void deserialize(std::istream&);
			
		private:
			std::vector<Frame> _frames;
			
			float _startTime = 0.0f;
			float _stopTime = 0.0f;
			float _frameRate = 0.0f;
			
			OutOfRangeMode _outOfRangeMode = OutOfRangeMode_Loop;
		};
	}
}
//
//  RaytraceThread.h
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/threading/thread.h>
#include "Raytracer.h"

namespace rt
{
	class RaytraceThreadDelegate
	{
	public:
		virtual ~RaytraceThreadDelegate() { }
		
		virtual bool fetchNewRenderRect(et::vec2i& origin, et::vec2i& size, bool& preview) = 0;
		
		virtual OutputFunction outputFunction() = 0;
		
		virtual et::vec2i imageSize() = 0;
		
		virtual void renderFinished(uint64_t) = 0;
		
		virtual const RaytraceScene& scene() = 0;
	};
	
	class RaytraceThread : public et::Thread
	{
	public:
		RaytraceThread(RaytraceThreadDelegate*);
		
		et::ThreadResult main() override;
		
		bool rendering() const
			{ return _rendering; }
		
	private:
		RaytraceThread(const RaytraceThread&) = delete;
		
	private:
		RaytraceThreadDelegate* _delegate = nullptr;
		et::AtomicBool _rendering;
	};
}

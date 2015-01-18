//
//  RaytraceThread.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/core/tools.h>
#include "RaytraceThread.h"

using namespace et;
using namespace rt;

RaytraceThread::RaytraceThread(RaytraceThreadDelegate* d) :
	Thread(true), _delegate(d)
{
	
}

et::ThreadResult RaytraceThread::main()
{
	while (running())
	{
		vec2i size;
		vec2i origin;
		
		bool preview = true;
		if (_delegate->fetchNewRenderRect(origin, size, preview))
		{
			auto currentTime = queryContiniousTimeInMilliSeconds();

			_rendering = true;
			
			if (preview)
				raytracePreview(_delegate->scene(), _delegate->imageSize(), origin, size, _delegate->outputFunction());
			else
				raytrace(_delegate->scene(), _delegate->imageSize(), origin, size, _delegate->outputFunction());
			
			_rendering = false;

			auto elapsedTime = queryContiniousTimeInMilliSeconds() - currentTime;
			
			_delegate->renderFinished(elapsedTime);
		}
		else
		{
			Thread::sleepMSec(25);
		}
	}
	
	return 0;
}

//
//  RaytraceThread.cpp
//  Raytracer
//
//  Created by Sergey Reznik on 27/6/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

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
		
		if (_delegate->fetchNewRenderRect(origin, size))
		{
			_rendering = true;
			
			raytrace(_delegate->scene(), _delegate->imageSize(), origin, size,
				_delegate->shouldAntialias(), _delegate->outputFunction());
			
			_rendering = false;
			
			_delegate->renderFinished();
		}
		else
		{
			Thread::sleepMSec(25);
		}
	}
	
	return 0;
}

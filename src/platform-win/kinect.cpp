/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et/core/et.h>

#if (ET_PLATFORM_WIN && ET_HAVE_KINECT_SDK)

#include <iostream>
#include <Windows.h>
#include <MSR_NuiApi.h>
#include <et/platform-win/kinect.h>

#pragma comment(lib, "MSRKinectNUI.lib")

enum KinectEvent
{
	KinectEvent_Shutdown,
	KinectEvent_NextImageFrame,
	KinectEvent_NextDepthFrame,
	KinectEvent_NextSkeletonFrame,
	KinectEvent_max
};

class et::KinectPrivate
{
public:
	KinectPrivate(Kinect* k);
	~KinectPrivate();

	static DWORD WINAPI threadFunc(void* param);

	void didGetNewDepthFrame();
	void didGetNewImageFrame();
	void didGetNewSkeletonFrame();

	void parseDepthData(KinectDepthData data, const vec2i& dimension);

public:
	Kinect* _kinect;
	HANDLE _thread;
	HANDLE _threadStopEvent;
	HANDLE _kinectImageNextFrameEvent;
	HANDLE _kinectDepthNextFrameEvent;
	HANDLE _kinectSkeletonNextFrameEvent;

	HANDLE _kinectImageStream;
	HANDLE _kinectDepthStream;

	bool _firstPlayerVisible;
	bool _secondPlayerVisible;
};

using namespace et;

/*
* Kinect
*/

size_t Kinect::PlayerIndexMask = 0x7;
unsigned short Kinect::DepthValueShift = 3;

bool Kinect::deviceAvailable()
{
	int deviceCount = 0;
	HRESULT result = MSR_NUIGetDeviceCount(&deviceCount);
	return (result == S_OK) && (deviceCount > 0);
}

vec2i Kinect::imageFrameSize()
{
	return vec2i(640, 480);
}

vec2i Kinect::depthFrameSize()
{
	return vec2i(320, 240);
}

Kinect::Kinect(KinectDelegate* delegate) :
	_delegate(delegate)
{
	_deviceAvailable = Kinect::deviceAvailable();
	
	if (_deviceAvailable)
	{
		ET_PIMPL_INIT(Kinect, this);
	}
}

Kinect::~Kinect()
{
	ET_PIMPL_FINALIZE(Kinect)
}

/*
 * Kinect private
 */

KinectPrivate::KinectPrivate(Kinect* k) : _kinect(k), _thread(0), _threadStopEvent(0), 
	_kinectImageNextFrameEvent(0), _kinectDepthNextFrameEvent(0), _kinectSkeletonNextFrameEvent(0),
	_kinectImageStream(0), _kinectDepthStream(0), _firstPlayerVisible(false), _secondPlayerVisible(false)
{
	HRESULT result = NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON | NUI_INITIALIZE_FLAG_USES_COLOR);
	if (FAILED(result))
	{
		std::cout << "Failed to initialize kinect. Error code: " << result << std::endl;
		return;
	}

	_threadStopEvent = CreateEvent(0, false, false, 0);
	_kinectImageNextFrameEvent = CreateEvent(0, true, false, 0);
	_kinectDepthNextFrameEvent = CreateEvent(0, true, false, 0);
	_kinectSkeletonNextFrameEvent = CreateEvent(0, true, false, 0);

	result = NuiImageStreamOpen(NUI_IMAGE_TYPE_COLOR, NUI_IMAGE_RESOLUTION_640x480, 0, 2, 
		_kinectImageNextFrameEvent, &_kinectImageStream);

	if (FAILED(result))
		std::cout << "Failed to open kinect image stream. Error code: " << result << std::endl;

	result = NuiImageStreamOpen(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, NUI_IMAGE_RESOLUTION_320x240, 0, 2, 
		_kinectDepthNextFrameEvent, &_kinectDepthStream);

	if (FAILED(result))
		std::cout << "Failed to open kinect depth stream. Error code: " << result << std::endl;

	result = NuiSkeletonTrackingEnable(_kinectSkeletonNextFrameEvent, 0);
	if (FAILED(result))
		std::cout << "Failed to enable kinect skeleton tracking. Error code:" << result << std::endl;

	_thread = CreateThread(0, 0, threadFunc, this, 0, 0);
}

KinectPrivate::~KinectPrivate()
{
	if (_threadStopEvent)
	{
		SetEvent(_threadStopEvent);
		if (_thread)
		{
			WaitForSingleObject(_thread, INFINITE);
			CloseHandle(_thread);
		}

		CloseHandle(_threadStopEvent);
	}

	NuiShutdown();

	if (_kinectSkeletonNextFrameEvent)
		CloseHandle(_kinectSkeletonNextFrameEvent);

	if (_kinectDepthNextFrameEvent)
		CloseHandle(_kinectDepthNextFrameEvent);

	if (_kinectImageNextFrameEvent)
		CloseHandle(_kinectImageNextFrameEvent);
}

DWORD WINAPI KinectPrivate::threadFunc(void* param)
{
	std::cout << "Running kinect thread..." << std::endl;

	KinectPrivate* kp = reinterpret_cast<KinectPrivate*>(param);

	HANDLE eventList[KinectEvent_max] = 
	{
		kp->_threadStopEvent,
		kp->_kinectImageNextFrameEvent,
		kp->_kinectDepthNextFrameEvent,
		kp->_kinectSkeletonNextFrameEvent,
	};

	DWORD eventIndex = static_cast<DWORD>(-1);
	DWORD numEvents = sizeof(eventList) / sizeof(HANDLE);

	bool running = true;
	while (running)
	{
		eventIndex = WaitForMultipleObjects(numEvents, eventList, false, 100);
		if (eventIndex == KinectEvent_Shutdown)
		{
			running = false;
		}
		else if (eventIndex == KinectEvent_NextImageFrame)
		{
			kp->didGetNewImageFrame();
		}
		else if (eventIndex == KinectEvent_NextDepthFrame)
		{
			kp->didGetNewDepthFrame();
		}
		else if (eventIndex == KinectEvent_NextSkeletonFrame)
		{
			kp->didGetNewSkeletonFrame();
		}
	}

	std::cout << "Kinect thread has finished." << std::endl;
	return 0;
}

void KinectPrivate::didGetNewDepthFrame()
{
	const NUI_IMAGE_FRAME* frame = 0;
	HRESULT result = NuiImageStreamGetNextFrame(_kinectDepthStream, 0, &frame);
	if (FAILED(result)) return;

	KINECT_SURFACE_DESC surfaceDescription = { };
	if (!FAILED(frame->pFrameTexture->GetLevelDesc(0, &surfaceDescription)))
	{
		KINECT_LOCKED_RECT lockedRect = { };
		if (!FAILED(frame->pFrameTexture->LockRect(0, &lockedRect, 0, 0)))
		{
			KinectDepthData depthData = static_cast<KinectDepthData>(lockedRect.pBits);
			vec2i dimensions(surfaceDescription.Width, surfaceDescription.Height);
			parseDepthData(depthData, dimensions);
			_kinect->_delegate->kinectDidGetDepthFrame(dimensions, depthData);
			frame->pFrameTexture->UnlockRect(0);
		}
	}

	NuiImageStreamReleaseFrame(_kinectDepthStream, frame);
}

void KinectPrivate::didGetNewImageFrame()
{
	const NUI_IMAGE_FRAME* frame = 0;
	HRESULT result = NuiImageStreamGetNextFrame(_kinectImageStream, 0, &frame);
	if (FAILED(result)) return;

	KINECT_SURFACE_DESC surfaceDescription = { };
	if (!FAILED(frame->pFrameTexture->GetLevelDesc(0, &surfaceDescription)))
	{
		KINECT_LOCKED_RECT lockedRect = { };
		if (!FAILED(frame->pFrameTexture->LockRect(0, &lockedRect, 0, 0)))
		{
			_kinect->_delegate->kinectDidGetImageFrame(vec2i(surfaceDescription.Width, surfaceDescription.Height), 
				static_cast<KinectColorData>(lockedRect.pBits));

			frame->pFrameTexture->UnlockRect(0);
		}
	}

	NuiImageStreamReleaseFrame(_kinectImageStream, frame);
}

void KinectPrivate::didGetNewSkeletonFrame()
{
	NUI_SKELETON_FRAME frame = { };
	if (FAILED(NuiSkeletonGetNextFrame(0, &frame))) return;

	NuiTransformSmooth(&frame, 0);

	KinectSkeletonFrameDescription desc;
	desc.floorClipPlane = vec4(frame.vFloorClipPlane.x, frame.vFloorClipPlane.y, frame.vFloorClipPlane.z, frame.vFloorClipPlane.w);
	desc.gravityVector = vec4(frame.vNormalToGravity.x, frame.vNormalToGravity.y, frame.vNormalToGravity.z, frame.vNormalToGravity.w);
	for (int i = 0; i < NUI_SKELETON_COUNT; ++i)
	{
		NUI_SKELETON_DATA& data = frame.SkeletonData[i];
		if (data.eTrackingState == NUI_SKELETON_TRACKED)
		{
			KinectSkeletonData skeleton = { };
			skeleton.enrollmentIndex = data.dwEnrollmentIndex;
			skeleton.trackingId = data.dwTrackingID;
			skeleton.skeletonClipped[KinectSkeletonClipping_Left] = (data.dwQualityFlags & NUI_SKELETON_QUALITY_CLIPPED_LEFT) != 0;
			skeleton.skeletonClipped[KinectSkeletonClipping_Right] = (data.dwQualityFlags & NUI_SKELETON_QUALITY_CLIPPED_RIGHT) != 0;
			skeleton.skeletonClipped[KinectSkeletonClipping_Top] = (data.dwQualityFlags & NUI_SKELETON_QUALITY_CLIPPED_TOP) != 0;
			skeleton.skeletonClipped[KinectSkeletonClipping_Bottom] = (data.dwQualityFlags & NUI_SKELETON_QUALITY_CLIPPED_BOTTOM) != 0;
			skeleton.overallSkeletonPosition = vec4(data.Position.x, data.Position.y, data.Position.z, data.Position.w);
			for (int i = 0; i < KinectSkeletonJoint_max; ++i)
			{
				skeleton.jointTracked[i] = data.eSkeletonPositionTrackingState[i] != NUI_SKELETON_POSITION_NOT_TRACKED;
				skeleton.joint[i] = vec4(data.SkeletonPositions[i].x, data.SkeletonPositions[i].y, 
					data.SkeletonPositions[i].z, data.SkeletonPositions[i].w);
			}
			_kinect->_delegate->kinectDidGetSkeleton(desc, skeleton);
		}
	}
}

void KinectPrivate::parseDepthData(KinectDepthData data, const vec2i& dimensions)
{
	bool firstPlayerWasVisible = _firstPlayerVisible;
	bool secondPlayerWasVisible = _secondPlayerVisible;
	_firstPlayerVisible = false;
	_secondPlayerVisible = false;

	int startIndex = dimensions.square() / 2;
	int numIndexes = startIndex - 1;
	for (int i = 0; i < numIndexes; ++i)
	{
		if (((data[startIndex + i] & Kinect::PlayerIndexMask) == 1) || ((data[startIndex - i] & Kinect::PlayerIndexMask) == 1))
		{
			_firstPlayerVisible = true;
			break;
		}
	}
	for (int i = 0; i < numIndexes; ++i)
	{
		if (((data[startIndex + i] & Kinect::PlayerIndexMask) == 2) || ((data[startIndex - i] & Kinect::PlayerIndexMask) == 2))
		{
			_secondPlayerVisible = true;
			break;
		}
	}

	if (_firstPlayerVisible != firstPlayerWasVisible)
	{
		if (_firstPlayerVisible)
			_kinect->_delegate->kinectDidFindPlayer(1);
		else
			_kinect->_delegate->kinectDidLosePlayer(1);
	}

	if (_secondPlayerVisible != secondPlayerWasVisible)
	{
		if (_secondPlayerVisible)
			_kinect->_delegate->kinectDidFindPlayer(2);
		else
			_kinect->_delegate->kinectDidLosePlayer(2);
	}
}

#else 
#	if (ET_PLATFORM_WIN)
#		pragma message("Define ET_HAVE_KINECT_SDK to compile")
#	endif
#endif

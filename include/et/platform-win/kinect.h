/*
 * This file is part of `et engine`
 * Copyright 2009-2012 by Sergey Reznik
 * Please, do not modify contents without approval.
 *
 */

#pragma once

#include <et/geometry/geometry.h>

namespace et
{

	typedef unsigned short* KinectDepthData;
	typedef vec4ub* KinectColorData;

	enum KinectSkeletonJoint 
	{
		KinectSkeletonJoint_HipCenter = 0,
		KinectSkeletonJoint_Spine,
		KinectSkeletonJoint_ShoulderCenter,
		KinectSkeletonJoint_Head,
		KinectSkeletonJoint_ShoulderLeft,
		KinectSkeletonJoint_ElbowLeft,
		KinectSkeletonJoint_WristLeft,
		KinectSkeletonJoint_HandLeft,
		KinectSkeletonJoint_ShoulderRight,
		KinectSkeletonJoint_ElbowRight,
		KinectSkeletonJoint_WristRight,
		KinectSkeletonJoint_HandRight,
		KinectSkeletonJoint_HipLeft,
		KinectSkeletonJoint_KneeLeft,
		KinectSkeletonJoint_AnkleLeft,
		KinectSkeletonJoint_FootLeft,
		KinectSkeletonJoint_HipRight,
		KinectSkeletonJoint_KneeRight,
		KinectSkeletonJoint_AnkleRight,
		KinectSkeletonJoint_FootRight,
		KinectSkeletonJoint_max
	};

	enum KinectSkeletonClipping
	{
		KinectSkeletonClipping_Left,
		KinectSkeletonClipping_Right,
		KinectSkeletonClipping_Top,
		KinectSkeletonClipping_Bottom,
		KinectSkeletonClipping_max
	};

	struct KinectSkeletonFrameDescription
	{
		vec4 floorClipPlane;
		vec4 gravityVector;
	};

	struct KinectSkeletonData
	{
		size_t trackingId;
		size_t enrollmentIndex;
		vec4 overallSkeletonPosition;
		vec4 joint[KinectSkeletonJoint_max];
		bool jointTracked[KinectSkeletonJoint_max];
		bool skeletonClipped[KinectSkeletonClipping_max];
	};

	class KinectDelegate
	{
	public:
		virtual void kinectDidGetImageFrame(const vec2i& size, KinectColorData data) = 0;
		virtual void kinectDidGetDepthFrame(const vec2i& size, KinectDepthData data) = 0;

		virtual void kinectDidFindPlayer(int playerIndex) = 0;
		virtual void kinectDidLosePlayer(int playerIndex) = 0;

		virtual void kinectDidGetSkeleton(const KinectSkeletonFrameDescription& desc, const KinectSkeletonData& skeleton) = 0;
	};

	class KinectPrivate;
	class Kinect
	{
	public:
		static bool deviceAvailable();
		static vec2i imageFrameSize();
		static vec2i depthFrameSize();

		static size_t PlayerIndexMask;
		static unsigned short DepthValueShift;

	public:
		Kinect(KinectDelegate*);
		~Kinect();

	private:
		friend class KinectPrivate;
		ET_DECLARE_PIMPL(Kinect, 32)

		KinectDelegate* _delegate = nullptr;
		bool _deviceAvailable = false;
	};

}
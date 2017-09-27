static const float MomentsDepthBias = 0.035955884801f;
static const float MomentBias = 3e-5;

#define Use16BitMomentsEncoding 1

float4 encodeMoments(in float FragmentDepth)
{
	float Square = FragmentDepth * FragmentDepth;
	float4 Moments = float4(FragmentDepth, Square, FragmentDepth * Square, Square * Square);

#if (Use16BitMomentsEncoding)
	
	static const float4x4 MomentsTransformMatrix = float4x4(
		-2.07224649f, 13.7948857237f, 0.105877704f, 9.7924062118f,
		32.23703778f, -59.4683975703f, -1.9077466311f, -33.7652110555f,
		-68.571074599f, 82.0359750338f, 9.3496555107f, 47.9456096605f,
		39.3703274134f,-35.364903257f, -6.6543490743f, -23.9728048165f);
	float4 Out4MomentsOptimized = mul(Moments, MomentsTransformMatrix);
	Out4MomentsOptimized.x += MomentsDepthBias;
	return Out4MomentsOptimized;

#else

	return Moments;

#endif
}

float4 decodeMoments(in float4 moments)
{
#if (Use16BitMomentsEncoding)
	
	static const float4x4 MomentsInverseTransformMatrix = float4x4(
		0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f,
		0.0771972861f, 0.1394629426f, 0.2120202157f, 0.2591432266f,
		0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f,
		0.0319417555f, -0.1722823173f, -0.2758014811f, -0.3376131734f);

	moments.x -= MomentsDepthBias;
	return mul(moments, MomentsInverseTransformMatrix);

#else
	
	return moments;

#endif
}
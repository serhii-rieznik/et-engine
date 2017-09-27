Texture2D<float4> shadowTexture : DECL_TEXTURE(Shadow);
SamplerComparisonState shadowSampler : DECL_SAMPLER(Shadow);

Texture2D<float4> aoTexture : DECL_TEXTURE(Ao);
SamplerState aoSampler : DECL_SAMPLER(Ao);

static const float2 PoissonDistribution[8] = {
	float2( 0.8528466f,  0.0213828f),
	float2( 0.1141956f,  0.2880972f),
	float2( 0.5853493f, -0.6930891f),
	float2( 0.6362274f,  0.7029839f),
	float2(-0.1640182f, -0.4143998f),
	float2(-0.8862001f, -0.3506839f),
	float2(-0.2186042f,  0.8690619f),
	float2(-0.8200445f,  0.4156708f)
};

static const float4x4 MomentsTransformMatrix = float4x4(
	-2.07224649f, 13.7948857237f, 0.105877704f, 9.7924062118f,
	32.23703778f, -59.4683975703f, -1.9077466311f, -33.7652110555f,
	-68.571074599f, 82.0359750338f, 9.3496555107f, 47.9456096605f,
	39.3703274134f,-35.364903257f, -6.6543490743f, -23.9728048165f);

static const float4x4 MomentsInverseTransformMatrix = float4x4(
	0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f,
	0.0771972861f, 0.1394629426f, 0.2120202157f, 0.2591432266f,
	0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f,
	0.0319417555f, -0.1722823173f, -0.2758014811f, -0.3376131734f);

static const float ShadowMapBias = 0.0001;
static const float PCFShadowRadius = PI / 2.0;
static const float MomentsDepthBias = 0.035955884801f;
static const float MomentBias = 3e-5;

#define Use16BitEncoding 1
#define UseSmoothStep 1

float4 encodeMoments(in float FragmentDepth)
{
	float Square = FragmentDepth * FragmentDepth;
	float4 Moments = float4(FragmentDepth, Square, FragmentDepth * Square, Square * Square);

#if (Use16BitEncoding)
	float4 Out4MomentsOptimized = mul(Moments, MomentsTransformMatrix);
	Out4MomentsOptimized.x += MomentsDepthBias;
	return Out4MomentsOptimized;
#else
	return Moments;
#endif
}

float sampleMomentsShadow(in float3 shadowTexCoord, in float2 shadowmapSize)
{
	float4 moments = aoTexture.Sample(aoSampler, shadowTexCoord.xy * 0.5 + 0.5);

#if (Use16BitEncoding)
	moments.x -= MomentsDepthBias;
	moments = mul(moments, MomentsInverseTransformMatrix);
#endif

	// Bias input data to avoid artifacts
	float4 b = lerp(moments, float4(0.5f, 0.5f, 0.5f, 0.5f), MomentBias);
	float3 z = shadowTexCoord.z - ShadowMapBias;

	// Compute a Cholesky factorization of the Hankel matrix B storing only non-
	// trivial entries or related products
	float L32D22 = -b.x * b.y + b.z;
	float D22 = -b.x * b.x + b.y;
	float SquaredDepthVariance = -b.y * b.y + b.w;
	float D33D22 = dot(float2(SquaredDepthVariance, -L32D22), float2(D22, L32D22));
	float InvD22 = 1.0f / D22;
	float L32 = L32D22 * InvD22;

	// Obtain a scaled inverse image of bz = (1, z.x, z.x * z.x)^T
	float3 c = float3(1.0f, z.x, z.x * z.x);
	// Forward substitution to solve L*c1=bz
	c.y -= b.x;
	c.z -= b.y + L32 * c.y;
	// Scaling to solve D*c2=c1
	c.y *= InvD22;
	c.z *= D22 / D33D22;
	// Backward substitution to solve L^T*c3=c2
	c.y -= L32 * c.z;
	c.x -= dot(c.yz, b.xy);
	
	// Solve the quadratic equation c.x + c.y * z + c.z * z^2 to obtain solutions  z.y and z.z
	float p = c.y / c.z;
	float q = c.x / c.z;
	float r = sqrt(max(0.0, (p * p * 0.25f) - q));
	z.y = -p * 0.5f - r;
	z.z = -p * 0.5f + r;

	// Compute the shadow intensity by summing the appropriate weights
	float4 Switch = (z.z < z.x) ? float4(z.y, z.x, 1.0f, 1.0f) : 
		((z.y < z.x) ? float4(z.x, z.y, 0.0f, 1.0f) : float4(0.0f, 0.0f, 0.0f, 0.0f));

	float Quotient = (Switch.x * z.z - b.x * (Switch.x + z.z) + b.y) / ((z.z - Switch.y) * (z.x - z.y));
	float result = 1.0 - saturate(Switch.z + Switch.w * Quotient);

#if (UseSmoothStep)
	return smoothstep(0.2, 1.0, result);
#else
	return result;
#endif
}

float sampleShadow(in float3 shadowTexCoord, in float rotationKernel, in float2 shadowmapSize)
{
#if (UseMomentsShadowmap)
	
	return sampleMomentsShadow(shadowTexCoord, shadowmapSize);

#else
	
	float biasedZ = shadowTexCoord.z - ShadowMapBias;
	float2 scaledUV = shadowTexCoord.xy * 0.5 + 0.5;
	float shadow = shadowTexture.SampleCmpLevelZero(shadowSampler, scaledUV, biasedZ);

#if (EnablePCFShadow)
	float angle = 2.0 * PI * (rotationKernel + 2.0 * PI * continuousTime);
	float sn = sin(angle);
	float cs = cos(angle);
	for (uint i = 0; i < 8; ++i)
	{
		float2 o = PoissonDistribution[i] / shadowmapSize;
		float2 r = PCFShadowRadius * float2(dot(o, float2(cs, -sn)), dot(o, float2(sn,  cs)));
		shadow += shadowTexture.SampleCmpLevelZero(shadowSampler, scaledUV + r, biasedZ);
	}
	shadow /= 9.0;
#endif

	return shadow;
#endif
}

Texture2D<float4> shadowTexture : DECL_TEXTURE(Shadow);

#if (ShadowMapping == ShadowMappingMoments)
	SamplerState shadowSampler : DECL_SAMPLER(Shadow);
#else
	SamplerComparisonState shadowSampler : DECL_SAMPLER(Shadow);
#endif

static const float PCFShadowRadius = 2.0;
static const float ShadowMapBias = 0.003 * PCFShadowRadius;

float sampleMomentsShadow(in float2 shadowTexCoord, in float fragmentDepth, in float2 shadowmapSize)
{
	float4 moments = decodeMoments(shadowTexture.Sample(shadowSampler, shadowTexCoord));

	// Bias input data to avoid artifacts
	float4 b = lerp(moments, float4(0.5f, 0.5f, 0.5f, 0.5f), MomentBias);
	float3 z = fragmentDepth;

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

	return smoothstep(0.25, 1.0, result);
}

float sampleShadow(in float3 shadowTexCoord, in float rotationKernel, in float2 shadowmapSize)
{
	float2 scaledUV = shadowTexCoord.xy * 0.5 + 0.5;
	float biasedZ = shadowTexCoord.z - ShadowMapBias;

#if (ShadowMapping == ShadowMappingMoments)
	
	return sampleMomentsShadow(scaledUV, biasedZ, shadowmapSize);

#elif (ShadowMapping == ShadowMappingPCF)

	static const float2 Distribution[9] = {
		float2(-0.5, -0.5), float2( 0.0, -0.5), float2( 0.5, -0.5),
		float2(-0.5,  0.0), float2( 0.0,  0.0), float2( 0.5,  0.0),
		float2(-0.5,  0.5), float2( 0.0,  0.5), float2( 0.5,  0.5),
	};

	float shadow = 0.0;
	for (uint i = 0; i < 9; ++i)
	{
		float2 o = PCFShadowRadius * (Distribution[i] / shadowmapSize);
		shadow += shadowTexture.SampleCmpLevelZero(shadowSampler, scaledUV + o, biasedZ);
	}

	return shadow / 9.0;

#elif (ShadowMapping == ShadowMappingPoissonPCF)

	static const float2 PoissonDistribution[9] = {
		float2( 0.5853493, -0.6930891),
		float2(-0.1640182, -0.4143998),
		float2(-0.8862001, -0.3506839),
		float2( 0.0000000,  0.0000000),
		float2( 0.8528466,  0.0213828),
		float2(-0.8200445,  0.4156708),
		float2( 0.1141956,  0.2880972),
		float2( 0.6362274,  0.7029839),
		float2(-0.2186042,  0.8690619),
	};

	float angle = 2.0 * PI * (rotationKernel + 2.0 * PI * continuousTime);
	float sn = sin(angle);
	float cs = cos(angle);
	
	float shadow = 0.0;
	for (uint i = 0; i < 9; ++i)
	{
		float2 o = PoissonDistribution[i] / shadowmapSize;
		float2 r = PCFShadowRadius * float2(dot(o, float2(cs, -sn)), dot(o, float2(sn,  cs)));
		shadow += shadowTexture.SampleCmpLevelZero(shadowSampler, scaledUV + r, biasedZ);
	}
	return (shadow / 9.0);

#else

	return shadowTexture.SampleCmpLevelZero(shadowSampler, scaledUV, biasedZ);

#endif	
}

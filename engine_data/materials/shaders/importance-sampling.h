#define DIFFUSE_SAMPLES 	1
#define SPECULAR_SAMPLES 	256

float3 sampleEnvironment(in float3 dir, in float level);
float ggxDistribution(in float NdotH, in float roughnessSquared);
float ggxMasking(in float VdotN, in float LdotN, in float roughnessSquared);
float diffuseBurley(in float LdotN, in float VdotN, in float LdotH, in float roughness);

void buildBasis(in float3 n, out float3 t, out float3 b)
{
	t = normalize(cross(n, (abs(n.z) < 0.999) ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0)));
	b = cross(n, t);
}

float bitfieldInverse(in uint bits)
{
	bits = (bits << 16) | (bits >> 16);
	bits = ((bits & 0x55555555) << 1) | ((bits & 0xAAAAAAAA) >> 1);
	bits = ((bits & 0x33333333) << 2) | ((bits & 0xCCCCCCCC) >> 2);
	bits = ((bits & 0x0F0F0F0F) << 4) | ((bits & 0xF0F0F0F0) >> 4);
	bits = ((bits & 0x00FF00FF) << 8) | ((bits & 0xFF00FF00) >> 8);
	return (float)(bits * 2.3283064365386963e-10);
}

float3 importanceSampleCosine(in float2 Xi)
{
	float t = sqrt(Xi.x);	
	return float3(cos(Xi.y) * t, sin(Xi.y) * t, sqrt(max(0.0, 1.0 - Xi.x)));
}

float3 importanceSampleGGX(in float2 Xi, in float r)
{                                       
	float t = sqrt((1.0f - Xi.x) / ((r * r - 1.0) * Xi.x + 1.0f));
	float s = sqrt(max(0.0, 1.0 - t * t));

	return float3(cos(Xi.y) * s, sin(Xi.y) * s, t);
}

float3 importanceSampledDiffuse(in float3 n, in float3 v, in Surface surface)
{
	uint numSamples = DIFFUSE_SAMPLES;

	float3 t, b;
	buildBasis(n, t, b);
	
	float3 result = 0.0;
	for (uint i = 0; i < DIFFUSE_SAMPLES; ++i)
	{
		float2 Xi;
		Xi.x = bitfieldInverse(i);
		Xi.y = 2.0 * PI * (i / (float)(DIFFUSE_SAMPLES));
		float3 h = importanceSampleCosine(Xi);
		float3 l = t * h.x + b * h.y + n * h.z;

		float LdotN = saturate(dot(l, n));
		float VdotN = saturate(dot(v, n));
		float LdotH = saturate(dot(l, normalize(l + v)));
		float diffuse = diffuseBurley(LdotN, VdotN, LdotH, surface.roughness);
	
		result += diffuse * sampleEnvironment(l, 0.0);
	}
	
	return surface.baseColor * result / float(DIFFUSE_SAMPLES);
}

float3 importanceSampledSpecular(in float3 n, in float3 v, in Surface surface)
{
	float3 t, b;
	buildBasis(n, t, b);

	float3 result = 0.0;
	for (uint i = 0; i < SPECULAR_SAMPLES; ++i)
	{
		float2 Xi;
		Xi.x = bitfieldInverse(i);
		Xi.y = 2.0 * PI * (i / (float)(SPECULAR_SAMPLES));

		float3 h = importanceSampleGGX(Xi, surface.roughness);
		h = t * h.x + b * h.y + n * h.z;
		float3 l = normalize(2.0 * dot(v, h) * h - v);

		float LdotN = dot(l, n);
		if (LdotN > 0.0)
		{
			float NdotH = saturate(dot(n, h));
			float VdotN = saturate(dot(v, n));
			float LdotH = saturate(dot(l, h));

			float d = ggxDistribution(NdotH, surface.roughnessSquared);
			float g = ggxMaskingCombined(VdotN, LdotN, surface.roughnessSquared);

			float3 f = lerp(surface.f0, surface.f90, pow(1.0 - LdotH, 5.0));
			float pdf = d * NdotH / (4.0 * LdotH);
			float3 brdf = d * g * f;
			result += (brdf / pdf) * sampleEnvironment(l, 0.0);
		}
	}
	
	return result / float(SPECULAR_SAMPLES);
}

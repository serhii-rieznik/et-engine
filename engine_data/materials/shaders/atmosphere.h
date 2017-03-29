#define inScatteringSamples 16
#define outScatteringSamples 16
#define atmosphereHeight 50e+3
#define Re 6371e+3
#define Ra (Re + atmosphereHeight) 
#define H0r (7994.0)
#define H0m (1200.0)
#define betaR float3(6.554e-6, 1.428e-5, 2.853e-5)
#define betaM float3(6.894e-6, 1.018e-5, 1.438e-5)

float atmosphereIntersection(in float3 origin, in float3 direction)
{
	float b = dot(direction, origin);
	float d = b * b - dot(origin, origin) + Ra*Ra;
	return (d < 0.0) ? -1.0 : (sqrt(d) - b);
}

float phaseFunctionRayleigh(in float cosTheta)
{
	return (1.0 + cosTheta * cosTheta);
}

float phaseFunctionMie(in float cosTheta, in float g)
{
	return (3.0 * (1.0 - g*g)) / (2.0 * (2.0 * g*g)) *
		(1.0 - g*g) / pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 3.0 / 2.0) / (4.0 * PI);
}

float2 outScattering(in float3 origin, in float3 dir0, in float dist0, in float3 dir1, in float dist1)
{
	float stepSize0 = dist0 / outScatteringSamples;
	float3 dp0 = dir0 * stepSize0;
	
	float stepSize1 = dist1 / outScatteringSamples;
	float3 dp1 = dir1 * stepSize1;
	
	float2 result = 0.0;
	float3 pos0 = origin + 0.5 * dp0;
	float3 pos1 = origin + 0.5 * dp1;
	for (uint i = 0; i < outScatteringSamples; ++i)
	{
		float h0 = max(0.0, length(pos0) - Re);
		float h1 = max(0.0, length(pos1) - Re);
		result.x += exp(-h0 / H0r) * stepSize0 + exp(-h1 / H0r) * stepSize1;
		result.y += exp(-h0 / H0m) * stepSize0 + exp(-h1 / H0m) * stepSize1;
		pos0 += dp0;
		pos1 += dp1;
	}
	return result / (4.0 * PI);
}

float3 inScattering(in float3 origin, in float3 dir, in float dist, in float h0, in float3 light, in float3 beta, in float2 phase)
{
	float stepSize = dist / inScatteringSamples;
	float3 dp = dir * stepSize;

	float3 resultR = 0.0;
	float3 resultM = 0.0;
	float3 pos = origin + 0.5 * dp;
	for (uint i = 0; i < inScatteringSamples; ++i)
	{
		float tA = atmosphereIntersection(pos, light);
		float h = max(0.0, length(origin) - Re);

		float2 expOut = outScattering(pos, light, tA, -dir, length(pos - origin));
		float3 expOutR = exp(-betaR * expOut.x);
		float3 expOutM = exp(-betaM * expOut.y);
		resultR += exp(-h / H0r) * expOutR;
		resultM += exp(-h / H0m) * expOutM;

		pos += dp;
	}
	return (resultR * betaR * phase.x + resultM * betaM * phase.y) * stepSize;
}

float3 sampleAtmosphere(float3 dir, in float3 light, in float3 lightColor)
{
	/*
	float theta = 90.0 * PI / 180.0;
	light = float3(cos(theta), sin(theta), 0.0);
	// */

	const float g = 0.785;
	float cosTheta = dot(dir, light);
	float2 phase = float2(phaseFunctionRayleigh(cosTheta), phaseFunctionMie(cosTheta, g));

	float3 origin = float3(0.0, Re, 0.0);
	float targetDistance = atmosphereIntersection(origin, dir);
	return lightColor * inScattering(origin, dir, targetDistance, H0m, light, betaM, phase);
}

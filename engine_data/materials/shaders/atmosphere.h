float athmosphereIntersection(in float3 origin, in float3 direction, in float radius)
{
	float b = dot(direction, origin);
	float d = b * b - dot(origin, origin) + radius * radius;
	return (d < 0.0) ? -1.0 : (sqrt(d) - b);
}

float planetIntersection(in float3 origin, in float3 direction, in float radius)
{
	float b = dot(direction, origin);
	float d = b * b - dot(origin, origin) + radius * radius;
	return (d < 0.0) ? -1.0 : -(b + sqrt(d));
}

float phaseFunctionMie(in float cosTheta, in float g)
{
	return (3.0 / (8.0 * PI)) *
		((1.0 - g*g) * (1.0 + cosTheta * cosTheta) /
		((2.0 + g*g) * pow(abs(1.0 + g*g - 2.0 * g * cosTheta), 1.5)));
}

float phaseFunctionRayleigh(in float cosTheta)
{
	return (3.0 / 16.0) * (1.0 + cosTheta * cosTheta);
}

float3 sampleAtmosphere(float3 dir, in float3 light)
{
	const uint steps = 24;
	const uint subSteps = 12;
	const float3 sunColor = float3(10.0, 10.0, 10.0);
	const float Re = 6371.0e+3;
	const float Ra = 6421.0e+3;
	const float Hr = 7994.0;
	const float Hm = 1200.0;
	const float g = 0.785;
	const float3 betaR = float3(6.554e-6, 1.428e-5, 2.853e-5);
	const float3 betaM = float3(7.619e-6, 1.125e-5, 1.59e-5);

//	const float sunPhi = 0.0;
//	const float sunTheta = 0.0 * PI / 180.0;
//	const float3 light = float3(cos(sunTheta) * cos(sunPhi), sin(sunTheta), cos(sunTheta) * sin(sunPhi));

	const float height = 5.0;
	float3 origin = normalize(float3(0.0, 1.0, 0.0)) * (Re + height);

	float tE = planetIntersection(origin, dir, Re);
	if (tE >= 0.0)
	{
		return dir * 0.5 + 0.5;
	}

	float tA = athmosphereIntersection(origin, dir, Ra);
	float cosTheta = dot(dir, light);

	float2 phase = float2(phaseFunctionRayleigh(cosTheta), phaseFunctionMie(cosTheta, g));
	float2 opticalDepth = 0.0;
	float3 scatteringR = 0.0;
	float3 scatteringM = 0.0;

	float stepSize = tA / float(steps);

	for (uint i = 0; i < steps; ++i)
	{
		float3 pos = origin + dir * ((float(i) + 0.5) * stepSize);
		float height = length(pos) - Re;

		float2 opticalDepthAtStep = float2(exp(-height / Hr), exp(-height / Hm)) * stepSize;

		float substepSize = athmosphereIntersection(pos, light, Ra) / float(subSteps);
		pos += light * (0.5 * substepSize);

		float2 secondaryOpticalDepth = 0.0;
		for (uint j = 0; j < subSteps; ++j)
		{
			height = length(pos) - Re;
			secondaryOpticalDepth.x += exp(-height / Hr) * substepSize;
			secondaryOpticalDepth.y += exp(-height / Hm) * substepSize;
			pos += light * substepSize;
		}
		float3 attenuation = exp(-betaR * (opticalDepth.x + secondaryOpticalDepth.x) - betaM * (opticalDepth.y + secondaryOpticalDepth.y));
		scatteringR += attenuation * opticalDepthAtStep.x;
		scatteringM += attenuation * opticalDepthAtStep.y;
		opticalDepth += opticalDepthAtStep;
	}

	return sunColor * (scatteringR * betaR * phase.x + scatteringM * betaM * phase.y);
}

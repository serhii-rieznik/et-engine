	const float3 sunColor = float3(1.0, 1.0, 1.0);
	const float Re = 6370.0e+3;
	const float Ra = 6420.0e+3;
	const float Hr = 7994.0;
	const float Hm = 1200.0;
	const float height = 10.0;
	const float sunPhi = 0.0;
	const float sunTheta = 0.0 * PI / 180.0;
	const float3 light = float3(cos(sunTheta) * cos(sunPhi), sin(sunTheta), cos(sunTheta) * sin(sunPhi));
	const float3 betaR = float3(6.554e-6, 1.428e-5, 2.853e-5);
	const float3 betaM = float3(7.6e-6, 1.125e-5, 1.59e-6);
	const float g = 0.99;
	const uint samples = 10;

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
		((2.0 + g*g) * pow(1.0 + g*g - 2.0 * g * cosTheta, 1.5)));
}

float phaseFunctionRayleigh(in float cosTheta)
{
	return (3.0 / 4.0) * (1.0 + cosTheta * cosTheta);
}

float3 sampleAtmosphere(float3 dir)
{
	float3 origin = normalize(float3(0.0, 1.0, 0.0)) * (Re + height);

	float3 betaEx = betaR + betaM;

	float cosTheta = dot(dir, light);
	float phaseR = phaseFunctionRayleigh(cosTheta);
	float phaseM = phaseFunctionMie(cosTheta, g);

	float tE = planetIntersection(origin, dir, Re);
	if (tE > 0.0)
	{
		float3 betaEx = betaR + betaM;
		float3 outScattering = exp(-tE * betaEx);
		float3 inScattering = (betaR * phaseR + betaM * phaseM) / (betaR + betaM) * (1.0 - outScattering);
		return float3(0.5, 0.5, 0.5) * outScattering + sunColor * inScattering;
	}

	float tA = athmosphereIntersection(origin, dir, Ra);
	float3 result = 0.0;

	float sampleLength = tA / float(samples);
	float distanceToOrigin = 0.5 * sampleLength;
	float3 samplePoint = origin + distanceToOrigin * dir;

	for (uint i = 0; i < samples; ++i)
	{
		float samplePointHeight = length(samplePoint) - Re;

		float expR = exp(-samplePointHeight / Hr);
		float expM = exp(-samplePointHeight / Hm);
		float3 betaSc = betaR * expR * phaseR + betaM * expM * phaseM;
		float3 betaEx = betaR * expR + betaM * expM;
		float3 inScattering = 1.0 / betaEx * betaSc * (1.0 - exp(-distanceToOrigin * betaEx));
		result += 0.1 * inScattering * exp(-distanceToOrigin * betaEx);

		samplePoint += dir * sampleLength;
		distanceToOrigin += sampleLength;

/*
		float3 betaExp = exp(-betaEx * (float(i) + 0.5) * sampleLength);
		float3 betaSc = betaR * expR + betaM * expM;
		float3 pointInScattering = betaSc * betaExp;

		float3 resultOutScattering = betaExp;
		float3 pointColor = pointInScattering * resultOutScattering;

		result += pointColor;
		
/*
		float3 pSample = pStart + (sampleLength * (float(i) + 0.5)) * dir;
		float sampleHeight = length(pSample) - Re;
		float sampleHr = exp(-sampleHeight / Hr) * sampleLength;
		float sampleHm = exp(-sampleHeight / Hm) * sampleLength;

		float opticalDepthR = 0.0;
		float opticalDepthM = 0.0;
		float lA = athmosphereIntersection(pSample, light, Ra);
		float lightSampleLength = lA / float(lightSamples);
		for (uint j = 0; j < lightSamples; ++j)
		{
			float3 lSample = pSample + lightSampleLength * light * (float(j) + 0.5);
			float lightSampleHeight = length(lSample) - Re;
			opticalDepthR += exp(-lightSampleHeight / Hr) * lightSampleLength;
			opticalDepthM += exp(-lightSampleHeight / Hm) * lightSampleLength;
		}

		float3 t = exp(-(opticalDepthR + opticalDepthM) * (betaR + betaM));
		inScatteringR += t * sampleHr;
		inScatteringM += t * sampleHm;
*/
	}

	return sunColor * result;
}

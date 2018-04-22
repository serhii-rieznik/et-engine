#define scatteringSamples 			32
#define EARTH_RADIUS				6371e+3
#define ATMOSPHERE_HEIGHT 			80e+3
#define ATMOSPHERE_RADIUS			(EARTH_RADIUS + ATMOSPHERE_HEIGHT) 
#define H0 							float2(7994.0, 1200.0)
#define MIE_SCATTERING_ANISOTROPY	0.65
#define RAYLEIGH_EXTINCTION			float3(6.554e-6, 1.428e-5, 2.853e-5)
#define MIE_SCATTERING 				2.0e-6 // float3(6.894e-6, 1.018e-5, 1.438e-5)
#define RAYLEIGH_SCATTERING			RAYLEIGH_EXTINCTION
#define MIE_EXTINCTION 				(MIE_SCATTERING / 0.9)
#define OZONE_ABSORPTION			(float3(3.426, 8.298, 0.356) * 6.0e-7)
#define SUN_ILLUMINANCE 			120000.0
#define SUN_ANGULAR_SIZE			(9.35e-3)
#define SUN_SOLID_ANGLE				(6.87e-5) // (2.0 * PI * (1.0 - cos(SUN_ANGULAR_SIZE)))
#define SUN_ANGULAR_SIZE_COSINE		(cos(SUN_ANGULAR_SIZE))
#define SUN_LIMB_DARKENING			float3(0.397, 0.503, 0.652)
#define HEIGHT_ABOVE_GROUND			50.0

struct LookupParameters
{
	float u;
	float v;
	float w;
};

struct AtmosphereParameters
{
	float heightAboveGround;
	float viewZenithAngle;
	float lightZenithAngle;
};

Texture2D<float4> precomputedOpticalDepth : DECLARE_TEXTURE;

LookupParameters atmosphereParametersToLookup(in AtmosphereParameters p)
{
	LookupParameters result;
	result.u = p.viewZenithAngle * 0.5 + 0.5;
	result.v = p.heightAboveGround / ATMOSPHERE_HEIGHT;
	result.w = p.lightZenithAngle * 0.5 + 0.5; 
	return result;
}

AtmosphereParameters lookupParametersToAtmosphere(in LookupParameters p)
{
	AtmosphereParameters result;
	result.heightAboveGround = p.v * ATMOSPHERE_HEIGHT;
	result.viewZenithAngle = p.u * 2.0 - 1.0;
	result.lightZenithAngle = p.w * 2.0 - 1.0;
	return result;
} 

void atmosphereParametersToValues(in AtmosphereParameters p, out float3 position, out float3 view, out float3 light)
{
	position = float3(0.0, EARTH_RADIUS + p.heightAboveGround, 0.0);
	
	float viewCos = sqrt(1.0 - saturate(p.viewZenithAngle * p.viewZenithAngle));
	view = float3(viewCos, p.viewZenithAngle, 0.0); 
	
	float lightCos = sqrt(1.0 - saturate(p.lightZenithAngle * p.lightZenithAngle));
	light = float3(lightCos, p.lightZenithAngle, 0.0); 
}

void valuesToAtmosphereParameters(in float3 position, in float3 view, in float3 light, out AtmosphereParameters p)
{
	p.heightAboveGround = length(position) - EARTH_RADIUS;
	p.viewZenithAngle = normalize(view).y;
	p.lightZenithAngle = normalize(light).y;
}

int sphereIntersection(in float3 origin, in float3 direction, in float radius, out float2 result)
{
	result = 2.0e+127;

	int intersectionsCount = 0;

	float b = dot(direction, origin);
	float d = b * b - dot(origin, origin) + radius * radius;
	if (d >= 0.0f)
	{ 
		d = sqrt(d);
		float t1 = -b - d;
		float t2 = -b + d;
		float minT = min(t1, t2);
		float maxT = max(t1, t2);
		if ((minT >= 0.0) && (maxT >= 0.0))
		{
			result = float2(minT, maxT);
			intersectionsCount = 2;
		}
		else if ((minT <= 0.0) && (maxT >= 0.0))
		{
			result = float2(0.0, maxT);
			intersectionsCount = 1;
		}
	}
	
	return intersectionsCount;	
}

float3 atmosphereIntersection(in float3 origin, in float3 direction)
{
    float b = dot(direction, origin);
    float d = b * b - dot(origin, origin) + ATMOSPHERE_RADIUS * ATMOSPHERE_RADIUS;
	
	float3 result = 0.0;
	if (d >= 0.0)
	{
		result.x = -b - sqrt(d);
		result.y = -b + sqrt(d);
		result.z = max(result.x, result.y);
	}
	return result;
}

float3 densityFunction(in float heightAboveGround)
{
	const float2 rmDensityScale = float2(7994.0, 1200.0);
	float2 rmDensity = exp(-heightAboveGround / rmDensityScale);
	float oDensity = rmDensity.x; // 7.0 - 6.0 * rmDensity.x;
	return float3(rmDensity, oDensity);
}

float3 evaluateOpticalLengthBetweenPoints(in float3 p0, in float3 p1, in int samples)
{
	float3 result = 0.0;
	float3 pStep = (p1 - p0) / float(samples - 1);
	
	float3 samplePosition = p0;
	for (int i = 0; i < samples; ++i)
	{
		float heightAboveGround = max(0.0, length(samplePosition) - EARTH_RADIUS);
		result += densityFunction(heightAboveGround);
		samplePosition += pStep;
	}
	return result * length(pStep);
}

float3 evaluateTransmittanceFromOpticalLength(in float3 opticalLength)
{
	return exp(-opticalLength.x * RAYLEIGH_EXTINCTION - opticalLength.y * MIE_EXTINCTION - opticalLength.z * OZONE_ABSORPTION);
}

float3 evaluateTransmittanceBetweenPoints(in float3 p0, in float3 p1, in int samples)
{
	float3 opticalLength = evaluateOpticalLengthBetweenPoints(p0, p1, samples);	
	return evaluateTransmittanceFromOpticalLength(opticalLength);
}

float3 evaluateTransmittanceToAtmosphereBounds(in AtmosphereParameters p)
{
	float3 p0;
	float3 view;
	float3 light;
	atmosphereParametersToValues(p, p0, view, light);
	
	float2 atmosphereIntersection;
	sphereIntersection(p0, light, ATMOSPHERE_RADIUS, atmosphereIntersection);

	float3 p1 = p0 + light * atmosphereIntersection.y; 
	return evaluateTransmittanceBetweenPoints(p0, p1, 128);
}

float3 evaluateTransmittanceToAtmosphereBounds(in float3 p0, in float3 light)
{
	float2 atmosphereIntersection;
	sphereIntersection(p0, light, ATMOSPHERE_RADIUS, atmosphereIntersection);

	float3 p1 = p0 + light * atmosphereIntersection.y; 
	return evaluateTransmittanceBetweenPoints(p0, p1, 128);
}

float3 sampleTransmittanceToAtmosphereBounds(in AtmosphereParameters p)
{
	LookupParameters lookup = atmosphereParametersToLookup(p);
	return precomputedOpticalDepth.Sample(LinearClamp, float2(lookup.w, lookup.v));
}

float phaseFunctionRayleigh(in float cosTheta)
{
    return (8.0 / 10.0) * (7.0 / 5.0 + 1.0 / 2.0 * cosTheta);
		// (3.0 / 4.0) * (1.0 + cosTheta * cosTheta);
}

float phaseFunctionMie(in float cosTheta, in float g)
{
	float t1 = 3.0 * (1.0 - g * g);
	float t2 = 2.0 * (2.0 + g * g);
	float u1 = 1.0 + cosTheta * cosTheta;
	float u2 = pow(1.0 + g * g - 2.0 * g * cosTheta, 3.0 / 2.0);
    return (t1 / t2) * (u1 / u2);
}

float3 approximateMieScatteringFromRayleigh(in float3 integralR, in float mieR)
{
	return integralR * (mieR / integralR.x) * (RAYLEIGH_EXTINCTION.x / MIE_EXTINCTION.x) * (MIE_EXTINCTION / RAYLEIGH_EXTINCTION); 
}

float4 integrateInScattering(in float3 origin, float3 target, float3 light, in int samples)
{
	float3 pStep = (target - origin) / float(samples);
	float3 viewDirection = normalize(pStep);
	float pStepSize = length(pStep);

	float3 samplePosition = origin;

	float3 integralR = 0.0;
	float3 integralM = 0.0;

	float3 opticalLengthToOrigin = 0.0;
	for (int i = 0; i < samples; ++i)
	{
		float h = max(0.0, length(samplePosition) - EARTH_RADIUS);
		float3 densityAtSample = densityFunction(h) * pStepSize;
		
		AtmosphereParameters params;
		valuesToAtmosphereParameters(samplePosition, viewDirection, light, params);

		float3 t0 = evaluateTransmittanceFromOpticalLength(opticalLengthToOrigin);
		float3 t1 = sampleTransmittanceToAtmosphereBounds(params);

		float3 transmittance = t0 * t1;

		integralR += densityAtSample.x * transmittance;
		integralM += densityAtSample.y * transmittance;
		
        opticalLengthToOrigin += densityAtSample;
		samplePosition += pStep;	
	}

	return float4(integralR * RAYLEIGH_SCATTERING, (integralM * MIE_SCATTERING).x);

	integralM = approximateMieScatteringFromRayleigh(integralR, integralM.x);
	integralR *= 0.0;
}

float3 evaluateSingleScattering(in float3 view, in float3 light, in float3 integralR, in float3 integralM, in float3 lightIntensity)
{
	float cosTheta = dot(light, view);
	float phaseR = phaseFunctionRayleigh(cosTheta);
	float phaseM = phaseFunctionMie(cosTheta, MIE_SCATTERING_ANISOTROPY);
	return lightIntensity * (integralR * phaseR + integralM * phaseM);
}

/******************************************************
 *
 * Old stuff
 *
 ******************************************************/

float3 planetIntersection(in float3 origin, in float3 direction)
{
	float3 result = 0.0;

    float b = dot(direction, origin);
    float d = b * b - dot(origin, origin) + EARTH_RADIUS * EARTH_RADIUS;
	
	if (d >= 0.0)
	{
		result.x = -b - sqrt(d);
		result.y = -b + sqrt(d);
		result.z = max(result.x, result.y);
	}
	
	return result;
}

float2 density(in float3 pos)
{
    return exp(-(length(pos) - EARTH_RADIUS) / H0);
}

float2 evaluateOpticalLength(in float3 from, in float3 to, in int samplesCount)
{
    float3 dp = (to - from) / (samplesCount - 1);
    
    float2 result = 0.0;
    for (uint i = 0; i < samplesCount; ++i)
    {
        result += density(from);
        from += dp;
    }
    return result * length(dp);
}

float3 zenithDirectionFromAngle(in float cosTheta)
{
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return float3(0.0, cosTheta, sinTheta);
}

float3 positionFromNormalizedHeight(in float he)
{
	return float3(0.0, EARTH_RADIUS + ATMOSPHERE_HEIGHT * he, 0.0);
}

float3 evaluateTransmittance(in float h, in float sinTheta)
{
	float3 pos = positionFromNormalizedHeight(h);
	float3 light = zenithDirectionFromAngle(sinTheta);
	float a = atmosphereIntersection(pos, light);
	float2 opticalLength = evaluateOpticalLength(pos, pos + light * a, 256);
	return exp(-opticalLength.x * (OZONE_ABSORPTION + RAYLEIGH_EXTINCTION) - opticalLength.y * MIE_EXTINCTION);;
}                           

float3 evaluateTransmittance(in float3 p0, in float3 p1)
{
    float2 opticalLength = evaluateOpticalLength(p0, p1, 32);
	return exp(-opticalLength.x * (OZONE_ABSORPTION + RAYLEIGH_EXTINCTION) - opticalLength.y * MIE_EXTINCTION);
}                           

float3 samplePrecomputedTransmittance(in float h, in float sinTheta)
{
	return precomputedOpticalDepth.Sample(LinearClamp, float2(sinTheta * 0.5 + 0.5, saturate(h / ATMOSPHERE_HEIGHT))).xyz;
}

float3 inScattering(in float3 origin, in float3 target, in float3 light, in float2 phase)
{
    float3 dp = (target - origin) / (scatteringSamples - 1);
    float ds = length(dp);
    float lightSinTheta = -light.y;

    float3 integralR = 0.0;
    float3 integralM = 0.0;
    float2 opticalLengthToOrigin = 0.0;

    for (uint i = 0; i < scatteringSamples; ++i)
    {
    	float h = max(0.0, length(origin) - EARTH_RADIUS);
    	float2 d = density(origin) * ds;
		float p = planetIntersection(origin, light);
		if (p < 0.0)
		{
	    	float3 t0 = exp(-(opticalLengthToOrigin.x * (OZONE_ABSORPTION + RAYLEIGH_EXTINCTION) + opticalLengthToOrigin.y * MIE_EXTINCTION));
	    	float3 t1 = samplePrecomputedTransmittance(h, lightSinTheta);
	    	float3 transmittance = t0 * t1;
			float3 inScatteringR = d.x * transmittance;
			float3 inScatteringM = d.y * transmittance;
			integralR += inScatteringR;
			integralM += inScatteringM;
		}
    	opticalLengthToOrigin += d;
    	origin += dp;
    }

    float3 result = integralR * RAYLEIGH_EXTINCTION * phase.x + integralM * MIE_EXTINCTION * phase.y;

    return result / (4.0 * PI);
}

float3 lightColor(in float3 light)
{
	return SUN_ILLUMINANCE * samplePrecomputedTransmittance(0.0, light.y);
}

float3 sunLuminance()
{
	float3 zenithLuminance = SUN_ILLUMINANCE / SUN_SOLID_ANGLE;
	return zenithLuminance / samplePrecomputedTransmittance(0.0, 1.0);
}

float3 sunColor(in float3 view, in float3 light)
{
	float cosTheta = dot(view, light);
	if (cosTheta <= SUN_ANGULAR_SIZE_COSINE)
		return 0.0;

	float3 position = float3(0.0, EARTH_RADIUS + HEIGHT_ABOVE_GROUND, 0.0);

	float2 planetIntersection = 0.0;
	if (sphereIntersection(position, view, EARTH_RADIUS, planetIntersection) > 0)
		return 0.0;
	
	float distanceToCenter = (1.0 - cosTheta) / (1.0 - SUN_ANGULAR_SIZE_COSINE);
	float mu = sqrt(1.0 - distanceToCenter * distanceToCenter);
	float3 darkening = pow(mu, SUN_LIMB_DARKENING);
	float3 transmittance = samplePrecomputedTransmittance(HEIGHT_ABOVE_GROUND / ATMOSPHERE_HEIGHT, view.y);
	
	return sunLuminance() * darkening * transmittance;
}

float3 sampleAtmosphere(in float3 view, in float3 light)
{    
	float3 result = 0.0;
	float3 lightIntensity = SUN_ILLUMINANCE / samplePrecomputedTransmittance(0.0, 1.0); 

	float3 position = float3(0.0, EARTH_RADIUS + HEIGHT_ABOVE_GROUND, 0.0);

	float2 atmosphereIntersection = 0.0;
	int atmosphereIntersections = sphereIntersection(position, view, ATMOSPHERE_RADIUS, atmosphereIntersection);

	float2 planetIntersection = 0.0;
	int planetIntersections = sphereIntersection(position, view, EARTH_RADIUS, planetIntersection);

	float t0 = atmosphereIntersection.x;
	float t1 = min(atmosphereIntersection.y, planetIntersection.x);

	float3 origin = position + t0 * view;
	float3 target = position + t1 * view;

	float4 integral = integrateInScattering(origin, target, light, 32);
	float3 mieScattering = approximateMieScatteringFromRayleigh(integral.xyz, integral.w);

	return evaluateSingleScattering(view, light, integral.xyz, mieScattering, lightIntensity);
}

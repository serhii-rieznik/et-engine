#define scatteringSamples 			32
#define EARTH_RADIUS				6371e+3
#define ATMOSPHERE_HEIGHT 			60e+3
#define ATMOSPHERE_RADIUS			(EARTH_RADIUS + ATMOSPHERE_HEIGHT) 
#define H0 							float2(7994.0, 1200.0)
#define MIE_EXTINCTION_ANISOTROPY	0.75
#define RAYLEIGH_EXTINCTION			float3(6.554e-6, 1.428e-5, 2.853e-5)
#define MIE_EXTINCTION 				float3(6.894e-6, 1.018e-5, 1.438e-5)
#define OZONE_ABSORPTION			float3(2.0556e-6, 4.9788e-6, 2.136e-7)
#define SUN_ILLUMINANCE 			120000.0
#define SUN_ANGULAR_SIZE			(0.009512 * 2.0)
#define SUN_SOLID_ANGLE				(PI * SUN_ANGULAR_SIZE * SUN_ANGULAR_SIZE) // (2.0 * PI * (1.0 - cos(SUN_ANGULAR_SIZE)))
#define SUN_ANGULAR_SIZE_COSINE		(cos(SUN_ANGULAR_SIZE))
#define SUN_LIMB_DARKENING			float3(0.397, 0.503, 0.652)

Texture2D<float4> precomputedOpticalDepth : DECLARE_TEXTURE;

float atmosphereIntersection(in float3 origin, in float3 direction)
{
    float b = dot(direction, origin);
    float d = b * b - dot(origin, origin) + ATMOSPHERE_RADIUS * ATMOSPHERE_RADIUS;
    return (d < 0.0) ? -1.0 : (-b + sqrt(d));
}

float planetIntersection(in float3 origin, in float3 direction)
{
    float b = dot(direction, origin);
    float d = b * b - dot(origin, origin) + EARTH_RADIUS * EARTH_RADIUS;
    return (d < 0.0) ? -1.0 : (-b - sqrt(d));
}

float phaseFunctionRayleigh(in float cosTheta)
{
    return (3.0 / 4.0) * (1.0 + cosTheta * cosTheta);
}

float phaseFunctionMie(in float cosTheta, in float g)
{
    return (3.0 * (1.0 - g * g) * (1.0 + cosTheta * cosTheta)) /
    	((2.0 * (2.0 + g * g) * pow(1.0 + g * g - 2.0 * g * cosTheta, 3.0 / 2.0)));
}

float2 density(in float3 pos)
{
    float h = max(0.0, length(pos) - EARTH_RADIUS);
    return exp(-h / H0);
}

float2 evaluateOpticalLength(in float3 from, in float3 to)
{
    float3 dp = (to - from) / (scatteringSamples - 1);
    
    float2 result = 0.0;
    for (uint i = 0; i < scatteringSamples; ++i)
    {
        result += density(from);
        from += dp;
    }
    return result * length(dp);
}

float3 zenithDirectionFromAngle(in float sinTheta)
{
	float cosTheta = sqrt(1.0 - sinTheta * sinTheta);
	return float3(0.0, sinTheta, cosTheta);
}

float3 positionFromNormalizedHeight(in float he)
{
	return float3(0.0, EARTH_RADIUS + ATMOSPHERE_HEIGHT * he, 0.0);
}

float3 evaluateTransmittance(in float h, in float sinTheta)
{
	float3 pos = positionFromNormalizedHeight(h);
	float3 light = zenithDirectionFromAngle(sinTheta);
    float2 opticalLength = evaluateOpticalLength(pos, pos + light * atmosphereIntersection(pos, light));
	return exp(-opticalLength.x * (OZONE_ABSORPTION + RAYLEIGH_EXTINCTION) - opticalLength.y * MIE_EXTINCTION);
}                           

float3 evaluateTransmittance(in float3 p0, in float3 p1)
{
    float2 opticalLength = evaluateOpticalLength(p0, p1);
	return exp(-opticalLength.x * (OZONE_ABSORPTION + RAYLEIGH_EXTINCTION) - opticalLength.y * MIE_EXTINCTION);
}                           

float3 samplePrecomputedTransmittance(in float h, in float sinTheta)
{
	return precomputedOpticalDepth.Sample(LinearClamp, float2(sinTheta * 0.5 + 0.5, h / ATMOSPHERE_HEIGHT)).xyz;
}

float3 inScattering(in float3 origin, in float3 target, in float3 light, in float2 phase)
{
    float3 dp = (target - origin) / (scatteringSamples - 1);
    float ds = length(dp);
    float lightSinTheta = light.y;

    float3 rr = 0.0;
    float3 rm = 0.0;
    float2 opticalLengthToOrigin = 0.0;

    for (uint i = 0; i < scatteringSamples; ++i)
    {
    	float h = length(origin) - EARTH_RADIUS;
    	float2 d = density(origin);

    	float3 t0 = exp(-(opticalLengthToOrigin.x * (OZONE_ABSORPTION + RAYLEIGH_EXTINCTION) + opticalLengthToOrigin.y * MIE_EXTINCTION));
    	float3 t1 = samplePrecomputedTransmittance(h, lightSinTheta);

    	float3 transmittance = t0 * t1;

		rr += d.x * transmittance;
		rm += d.y * transmittance;

    	opticalLengthToOrigin += d * ds;
    	origin += dp;
    }

    float3 result = rr * RAYLEIGH_EXTINCTION * phase.x + rm * MIE_EXTINCTION * phase.y;

    return result * (ds / (4.0 * PI));
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
	float3 result = 0.0;

	float cosTheta = dot(view, light);
	if (cosTheta > SUN_ANGULAR_SIZE_COSINE)
	{
		float distanceToCenter = (1.0 - cosTheta) / (1.0 - SUN_ANGULAR_SIZE_COSINE);
		float mu = sqrt(1.0 - distanceToCenter * distanceToCenter);
		float3 darkening = pow(mu, SUN_LIMB_DARKENING);

		float3 transmittance = samplePrecomputedTransmittance(0.0, view.y);
		result = sunLuminance() * darkening * transmittance;
	}

	return result;
}

float3 sampleAtmosphere(float3 view, in float3 light)
{                                                  
    float cosTheta = dot(view, light);
    float2 phase = float2(phaseFunctionRayleigh(cosTheta), phaseFunctionMie(cosTheta, MIE_EXTINCTION_ANISOTROPY));
    float3 pos = positionFromNormalizedHeight(0.0);
    float t = min(8.0 * ATMOSPHERE_HEIGHT, atmosphereIntersection(pos, view));
    return SUN_ILLUMINANCE * inScattering(pos, pos + t * view, light, phase) / samplePrecomputedTransmittance(0.0, 1.0);
}

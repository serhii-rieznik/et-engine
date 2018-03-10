#define scatteringSamples 32
#define atmosphereHeight 60e+3
#define height 2.0
#define Re 6371e+3
#define Ra (Re + atmosphereHeight) 
#define H0 float2(7994.0, 1200.0)
#define mieG 0.75
#define betaR float3(6.554e-6, 1.428e-5, 2.853e-5)
#define betaM float3(6.894e-6, 1.018e-5, 1.438e-5)
#define positionOnPlanet float3(0.0, Re + height, 0.0)

float atmosphereIntersection(in float3 origin, in float3 direction)
{
    float b = dot(direction, origin);
    float d = b * b - dot(origin, origin) + Ra * Ra;
    return (d < 0.0) ? -1.0 : (-b + sqrt(d));
}

float planetIntersection(in float3 origin, in float3 direction)
{
    float b = dot(direction, origin);
    float d = b * b - dot(origin, origin) + Re * Re;
    return (d < 0.0) ? -1.0 : (-b - sqrt(d));
}

float phaseFunctionRayleigh(in float cosTheta)
{
    return (3.0 / 4.0) * (1.0 + cosTheta * cosTheta) / (4.0 * PI);
}

float phaseFunctionMie(in float cosTheta, in float g)
{
    return (3.0 * (1.0 - g * g)) / (2.0 * (2.0 + g * g)) * 
        (1.0 + cosTheta * cosTheta) / pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 3.0 / 2.0) / (4.0 * PI);
}

float2 densityAtHeight(in float h)
{
    return exp(-h / H0);
}

float2 density(in float3 pos)
{
    float h = max(0.0, length(pos) - Re);
    return densityAtHeight(h);
}

float2 opticalDensity(in float3 from, in float3 to)
{
    float3 dp = (to - from) / scatteringSamples;
    float3 origin = from;
    
    float2 result = 0.0;
    for (uint i = 0; i < scatteringSamples; ++i)
    {
        result += density(origin);
        origin += dp;
    }
    return result * length(dp);
}

float3 inScattering(in float3 origin, in float3 target, in float3 light, in float2 phase)
{
    float3 step = (target - origin) / scatteringSamples;

    float3 resultR = 0.0;
    float3 resultM = 0.0;
    float3 pos = origin + 0.5 * step;

    for (uint i = 0; i < scatteringSamples; ++i)
    {
        float2 d = density(pos);

        float3 outerIntersection = pos + light * atmosphereIntersection(pos, light);
        float2 opticalDepth = opticalDensity(outerIntersection, pos) + opticalDensity(pos, origin);
        float3 scattering = exp(-betaR * opticalDepth.x - betaM * opticalDepth.y);

        resultR += d.x * scattering;
        resultM += d.y * scattering;

        pos += step;
    }

    return (resultR * betaR * phase.x + resultM * betaM * phase.y) * length(step);
}

float3 outScattering(in float3 origin, in float3 target)
{
    float2 scatter = opticalDensity(origin, target - origin);
    return exp(-(scatter.x * betaR + scatter.y * betaM));   
}

float3 sampleAtmosphere(float3 dir, in float3 light, in float3 lightColor)
{
    float cosTheta = dot(dir, light);
    float2 phase = float2(phaseFunctionRayleigh(cosTheta), phaseFunctionMie(cosTheta, mieG));

    float maxDistance = atmosphereHeight * 4.0;
    float a = min(maxDistance, atmosphereIntersection(positionOnPlanet, dir));
    return lightColor * inScattering(positionOnPlanet, positionOnPlanet + a * dir, light, phase);
}

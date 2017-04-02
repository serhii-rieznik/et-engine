#define inScatteringSamples 16
#define outScatteringSamples 16
#define atmosphereHeight 122e+3
#define height 2.0
#define Re 6371e+3
#define Ra (Re + atmosphereHeight) 
#define H0r (7994.0)
#define H0m (1200.0)
#define mieG 0.85
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
    return (3.0 * (1.0 - g * g)) / (2.0 * (2.0 + g * g) * 
        (1.0 + cosTheta * cosTheta) ) / pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 3.0 / 2.0) / (4.0 * PI);
}

float2 density(in float3 pos)
{
    float h = max(0.0, length(pos) - Re);
    return float2(exp(-h / H0r), exp(-h / H0m));
}

float2 opticalDensity(in float3 origin, in float3 dir)
{
    float3 dp = dir / outScatteringSamples;
    origin += 0.5 * dp;
    
    float2 result = 0.0;
    for (uint i = 0; i < outScatteringSamples; ++i)
    {
        result += density(origin);
        origin += dp;
    }
    return result * length(dp);
}

float3 inScattering(in float3 origin, in float3 target, in float3 light, in float2 phase)
{
    float3 step = (target - origin) / inScatteringSamples;

    float3 resultR = 0.0;
    float3 resultM = 0.0;
    float3 pos = origin + 0.5 * step;
    float2 totalDensity = 0.0;
    for (uint i = 0; i < inScatteringSamples; ++i)
    {
        float2 d = density(pos);

        float3 toLight = light * atmosphereIntersection(pos, light);
        float2 opticalDepth = opticalDensity(pos, toLight) + opticalDensity(pos, origin - pos);
        float3 scattering = exp(-betaR * (totalDensity.x + opticalDepth.x) - betaM * (totalDensity.y + opticalDepth.y));

        resultR += d.x * scattering;
        resultM += d.y * scattering;

        totalDensity += d;
        pos += step;
    }
    return (resultR * betaR * phase.x + resultM * betaM * phase.y) * length(step);
}

float3 outScattering(in float3 origin, in float3 target)
{
    float2 scatter = opticalDensity(origin, target - origin);
    return exp(-(scatter.x * betaR + scatter.y * betaM));   
}

float2 opticalDensityAtConstantHeight(in float3 origin, in float3 dir)
{
    return density(origin + 0.5 * dir) * length(dir);
}

float3 inScatteringAtConstantHeight(in float3 origin, in float3 target, in float3 light, in float2 phase)
{
    float2 constantDensity = density(origin);
    float2 opticalDepthToLight = opticalDensity(origin, light * atmosphereIntersection(origin, light));
    float2 opticalDepthToOrigin = opticalDensity(target, origin - target);
    float2 opticalDepth = opticalDepthToLight + opticalDepthToOrigin;
    float3 result = exp(-betaR * opticalDepth.x - betaM * opticalDepth.y);
    return result * (betaR * constantDensity.x * phase.x + constantDensity.y * betaM * phase.y) * length(target - origin);
}

float3 outScatteringAtConstantHeight(in float3 origin, in float3 target)
{
    float2 scatter = density(target) * length(target - origin);
    return exp(-betaR * scatter.x - betaM * scatter.y);
}

float3 sampleAtmosphere(float3 dir, in float3 light, in float3 lightColor)
{
    float cosTheta = dot(dir, light);
    float2 phase = float2(phaseFunctionRayleigh(cosTheta), phaseFunctionMie(cosTheta, mieG));

    float a = atmosphereIntersection(positionOnPlanet, dir);
    return lightColor * inScattering(positionOnPlanet, positionOnPlanet + a * dir, light, phase);
}

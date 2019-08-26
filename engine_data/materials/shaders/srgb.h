static const float aperture = 16.0;
static const float iso = 100.0;
static const float shutterSpeed = 1.0 / 100.0;
static const float expectedEv = log2(aperture * aperture / shutterSpeed * (100.0 / iso));
static const float exposureRange = 5.0;
static const float2 dynamicRange = float2(exposureRange, exposureRange);
static const float2 adaptationRange = float2(5.0, 5.0);

float3 srgbToLinear(in float3 c)
{
#if (SRGBConversion == SRGBConversionApproximate)

	return pow(c, 2.2);

#elif (SRGBConversion == SRGBConversionAccurate)

	float3 linearRGBLo = c / 12.92;
	float3 linearRGBHi = pow((c + 0.055) / 1.055, 2.4);
	return lerp(linearRGBLo, linearRGBHi, step(0.04045, c));

#else

	return c * c;

#endif
}

float3 linearToSRGB(in float3 c)
{
#if (SRGBConversion == SRGBConversionApproximate)

	return pow(c, 1.0 / 2.2);

#elif (SRGBConversion == SRGBConversionAccurate)

	float3 sRGBLo = c * 12.92;
	float3 sRGBHi = (pow(c, 1.0 / 2.4) * 1.055) - 0.055;
	return lerp(sRGBLo, sRGBHi, step(0.0031308, c));
#else

	return sqrt(c);

#endif
}

float luminanceToEv(in float lum) {
	return log2(max(0.001, lum * LUMINANCE_SCALE)) + 3.0;
}

float evToLuminance(in float ev) {
	return exp2(ev - 3.0) / LUMINANCE_SCALE;
}

float3 toneMapping(float3 color, float averageLuminance, in float unused)
{
	float lowerBound = expectedEv - dynamicRange.x;
	float upperBound = expectedEv + dynamicRange.y;

	float maxLuminance = 78.0 / (iso * 0.65) * exp2(expectedEv) / LUMINANCE_SCALE;

	float exposure = 1.0 / maxLuminance;
	color = clamp(color, evToLuminance(lowerBound), evToLuminance(upperBound)) * exposure;

#if (ToneMapping == ToneMappingACES)

	static const float3x3 ACESInputMat = {
	    {0.59719, 0.35458, 0.04823},
    	{0.07600, 0.90834, 0.01566},
	    {0.02840, 0.13383, 0.83777}};

	static const float3x3 ACESOutputMat = {
	    { 1.60475, -0.53108, -0.07367},
    	{-0.10208,  1.10813, -0.00605},
	    {-0.00327, -0.07276, 1.07602}};

	static const float a = 0.0245786;
	static const float b = 0.000090537;
	static const float c = 0.983729;
	static const float d = 0.4329510;
	static const float e = 0.238081;
	color = linearToSRGB(color);
	color = mul(ACESInputMat, color);
    color = (color * (color + a) - b) / (color * (c * color + d) + e);
    return saturate(mul(ACESOutputMat, color));

#elif (ToneMapping == ToneMappingUncharted)

	static const float A = 0.15;
	static const float B = 0.50;
	static const float C = 0.10;
	static const float D = 0.20;
	static const float E = 0.02;
	static const float F = 0.30;
	static const float W = 11.2;
	static const float whiteScale = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color *= 3.0;
	color = (((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F) / whiteScale;
	return linearToSRGB(saturate(color));

#else
	
	color = saturate(1.0 - exp(-color));
	return linearToSRGB(color);

#endif
}                                        

float3 RGBToYCoCg(float3 c)
{
	return float3(c.x / 4.0 + c.y / 2.0 + c.z / 4.0, c.x / 2.0 - c.z / 2.0, -c.x / 4.0 + c.y / 2.0 - c.z / 4.0);
}

float3 YCoCgToRGB(float3 c)
{
	return saturate(float3(c.x + c.y - c.z, c.x + c.z, c.x - c.y - c.z));
}

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

	return pow(c, 1.0/2.2);

#elif (SRGBConversion == SRGBConversionAccurate)

	float3 sRGBLo = c * 12.92;
	float3 sRGBHi = (pow(c, 1.0/2.4) * 1.055) - 0.055;
	return lerp(sRGBLo, sRGBHi, step(0.0031308, c));
#else

	return sqrt(c);

#endif
}

float3 toneMapping(float3 color, float exposure)
{
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
	color = linearToSRGB(color * exposure);
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
	color *= exposure;
	color = whiteScale * (((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F);
	return linearToSRGB(saturate(color));

#else

	return linearToSRGB(saturate(exposure * color));

#endif
	
}
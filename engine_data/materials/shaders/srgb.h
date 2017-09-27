#define GAMMA_CORRECTION_IN  2.2
#define GAMMA_CORRECTION_OUT 1.0 / 2.2

#define ToneMappingDisabled 0
#define ToneMappingACES 1
#define ToneMappingUncharted 2
#define ToneMapping ToneMappingACES

float srgbToLinear(in float l)
{
	return pow(l, GAMMA_CORRECTION_IN);
}

float3 srgbToLinear(in float3 l)
{
	return pow(l, float3(GAMMA_CORRECTION_IN, GAMMA_CORRECTION_IN, GAMMA_CORRECTION_IN));
}

float4 srgbToLinear(in float4 l)
{
	return pow(l, float4(GAMMA_CORRECTION_IN, GAMMA_CORRECTION_IN, GAMMA_CORRECTION_IN, GAMMA_CORRECTION_IN));
}

float3 linearToSRGB(in float3 s)
{
	return pow(s, float3(GAMMA_CORRECTION_OUT, GAMMA_CORRECTION_OUT, GAMMA_CORRECTION_OUT));
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

	const float a = 0.0245786;
	const float b = 0.000090537;
	const float c = 0.983729;
	const float d = 0.4329510;
	const float e = 0.238081;
	color = linearToSRGB(color * exposure);
	color = mul(ACESInputMat, color);
    color = (color * (color + a) - b) / (color * (c * color + d) + e);
    return saturate(mul(ACESOutputMat, color));

#elif (ToneMapping == ToneMappingUncharted)

	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	color *= exposure;
	float whiteScale = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color = whiteScale * (((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F);
	return linearToSRGB(saturate(color));

#else

	return linearToSRGB(saturate(color * exposure));

#endif
	
}
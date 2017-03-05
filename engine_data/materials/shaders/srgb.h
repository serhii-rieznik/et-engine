#define GAMMA_CORRECTION_IN  2.2
#define GAMMA_CORRECTION_OUT 1.0/2.2

float3 srgbToLinear(in float3 l)
{
	return pow(abs(l), float3(GAMMA_CORRECTION_IN, GAMMA_CORRECTION_IN, GAMMA_CORRECTION_IN));
}

float3 linearToSRGB(in float3 s)
{
	return pow(abs(s), float3(GAMMA_CORRECTION_OUT, GAMMA_CORRECTION_OUT, GAMMA_CORRECTION_OUT));
}
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

float3 toneMapping(float3 color, float exposure)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	return pow(abs(color / white), float3(GAMMA_CORRECTION_OUT, GAMMA_CORRECTION_OUT, GAMMA_CORRECTION_OUT));
}
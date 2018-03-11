TextureCube<float4> convolvedSpecular : DECLARE_TEXTURE;

float3 sampleEnvironment(float3 i, in float3 l, float roughness)
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	convolvedSpecular.GetDimensions(0, w, h, levels);
	float sampledLod = roughness * (levels - 1.0);
	return convolvedSpecular.SampleLevel(LinearWrap, i, sampledLod).xyz;
}

float3 sampleSpecularConvolution(float3 i, float roughness)
{
	float w = 0.0;
	float h = 0.0;
	float levels = 0.0;
	convolvedSpecular.GetDimensions(0, w, h, levels);
	float sampledLod = roughness * (levels - 1.0);
	return convolvedSpecular.SampleLevel(LinearWrap, i, sampledLod).xyz;
}

float3 specularDominantDirection(in float3 n, in float3 v, in float roughness)
{
	float3 r = -reflect(v, n);
	float smoothness = saturate(1.0 - roughness);
	float factor = smoothness * (sqrt(smoothness) + roughness);
	return normalize(lerp(n, r, factor));
}

float3 diffuseDominantDirection(in float3 n, in float3 v, in float roughness)
{
	float a = 1.02341 * roughness - 1.51174;
	float b = -0.511705 * roughness + 0.755868;
	float factor = saturate((dot(n, v) * a + b) * roughness);
	return normalize(lerp(n, v, factor));
}

float texelSolidAngle(float u, float v, float invSize)
{
    float x0 = u - invSize;
    float x1 = u + invSize;
    float y0 = v - invSize;
    float y1 = v + invSize;
    float x00sq = x0 * x0;
    float x11sq = x1 * x1;
    float y00sq = y0 * y0;
    float y11sq = y1 * y1;
    return atan2(x0 * y0, sqrt(x00sq + y00sq + 1.0)) - 
    	   atan2(x0 * y1, sqrt(x00sq + y11sq + 1.0)) - 
		   atan2(x1 * y0, sqrt(x11sq + y00sq + 1.0)) + 
		   atan2(x1 * y1, sqrt(x11sq + y11sq + 1.0)) ;
}

float texelSolidAngle(in float3 d, float invSize)
{
	float u = 0.0;
	float v = 0.0;
	float3 ad = abs(d);
	float maxComponent = max(ad.x, max(ad.y, ad.z));
	if (maxComponent == ad.x)
	{
		u = d.z / maxComponent;
		v = d.y / maxComponent;
	}
	if (maxComponent == ad.y)
	{
		u = d.z / maxComponent;
		v = d.x / maxComponent;
	}
	if (maxComponent == ad.z)
	{
		u = d.y / maxComponent;
		v = d.x / maxComponent;
	}
	return texelSolidAngle(u, v, invSize);
}

float4 getExitRadianceFromSphericalHarmonics(in float4 sh[9], in float3 n)
{
	float4 shResult = sh[0] * (0.282095);
	shResult += sh[1] * (0.488603 * n.y);
	shResult += sh[2] * (0.488603 * n.z);
	shResult += sh[3] * (0.488603 * n.x);
	shResult += sh[4] * (1.092548 * n.x * n.y);
	shResult += sh[5] * (1.092548 * n.y * n.z);
	shResult += sh[6] * (0.315392 * (3.0 * n.z * n.z - 1.0));
	shResult += sh[7] * (1.092548 * n.x * n.z);
	shResult += sh[8] * (0.546274 * (n.x * n.x - n.y * n.y));
	return shResult;
}

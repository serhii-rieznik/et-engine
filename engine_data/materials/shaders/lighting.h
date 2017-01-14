struct PBSLightEnvironment
{
	float LdotN;
	float VdotN;
	float LdotH;
	float VdotH;
	float NdotH;

	float alpha;
	float metallness;

	float viewFresnel;
	float brdfFresnel;
};

float fresnelShlick(float fN, float cosTheta);
float normalizedLambert(PBSLightEnvironment env);
float burleyDiffuse(PBSLightEnvironment env);
float ggxD(float rSq, float ct);
float ggxG(float t, float rSq);
float microfacetSpecular(PBSLightEnvironment env);

#define NORMALIZATION_SCALE INV_PI

/*
 *
 * Implementation
 *
 */

float fresnelShlick(float fN, float cosTheta)
{
	return fN + (1.0 - fN) * pow(1.0 - cosTheta, 5.0);
}

float normalizedLambert(PBSLightEnvironment env)
{
	return NORMALIZATION_SCALE * max(0.0, env.LdotN);
}

float burleyDiffuse(PBSLightEnvironment env)
{
	// TODO
	return 1.0;
}

float ggxG(float t, float rSq)
{
	t *= t;
	float x = rSq * (1.0f - t) / t;
	return 1.0f / (1.0f + sqrt(1.0 + x * x));
}

float ggxD(float rSq, float ct)
{
	float x = ct * ct * (rSq - 1.0f) + 1.0f;
	return rSq / (PI * x * x + 0.000001);
}

float microfacetSpecular(PBSLightEnvironment env)
{
	float rSq = env.alpha * env.alpha;
	float ndf = ggxD(rSq, env.NdotH);
	float g1 = ggxG(env.VdotN, rSq) * float(env.VdotH / env.VdotN > 0.0f);
	float g2 = ggxG(env.LdotN, rSq) * float(env.LdotH / env.LdotN > 0.0f);
	return (ndf * g1 * g2 * env.brdfFresnel) / (env.VdotN * env.LdotN + 0.0000001) * NORMALIZATION_SCALE;
}

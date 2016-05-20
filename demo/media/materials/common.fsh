const float pi = 3.1415926536;
const float invPi = 0.3183098862;

float fresnelShlickApproximation(in float f0, in float cosTheta)
{
    return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

float burleyDiffuse(in float LdotN, in float VdotN, in float LdotH, in float alpha)
{
	alpha = 1.0 - pow(alpha, 4.0);
    float fl = pow(1.0 - LdotN, 5.0);
    float fv = pow(1.0 - VdotN, 5.0);
    float fd90 = 0.5 + alpha * (LdotH * LdotH);
    return (1.0 - fd90 * fl) * (1.0 + fd90 * fv);
}

float smithGGX(in float a2, in float cosTheta)
{
    cosTheta *= cosTheta;
    return 2.0 / (1.0 + sqrt(1.0 + a2 * (1.0 - cosTheta) / cosTheta));
}

float microfacetSpecular(in float NdotL, in float NdotV, in float NdotH, in float alpha)
{
	float a2 = alpha * alpha;
    float denom = 1.0 + NdotH * NdotH * (a2 - 1.0);
    float ggxD = a2 / (denom * denom);
    float ggxF = fresnelShlickApproximation(0.2f, NdotV);
    float ggxV = smithGGX(a2, NdotV) * smithGGX(a2, NdotL);
    return (ggxF * ggxD * ggxV) / max(0.00001, 4.0 * NdotL * NdotV);
}

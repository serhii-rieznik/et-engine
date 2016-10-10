float burleyDiffuse(float LdotN, float VdotN, float LdotH, float alpha); // shut up Metal compiler

float burleyDiffuse(float LdotN, float VdotN, float LdotH, float alpha)
{
	alpha = 1.0 - pow(alpha, 4.0);
	float fl = pow(1.0 - LdotN, 5.0);
	float fv = pow(1.0 - VdotN, 5.0);
	float fd90 = 0.5 + alpha * (LdotH * LdotH);
	return (1.0 - fd90 * fl) * (1.0 + fd90 * fv);
}

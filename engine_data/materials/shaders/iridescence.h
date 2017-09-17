/*
 * A Practical Extension to Microfacet Theory for the Modeling of Varying Iridescence
 * Laurent Belcour, Pascal Barla
 * https://belcour.github.io/blog/research/2017/05/01/brdf-thin-film.html
 */

const float Dinc = 1.5;// 10.0 0.5
const float eta2 = 1.3333;// 5.0 2.0
const float eta3 = 1.0; // 1.0 5.0 3.0
const float kappa3 = 0.75; // 0.0 5.0 0.0

 // Square functions for cleaner code
float sqr(float x) {
	return x * x;
}

float2 sqr(float2 x) {
	return x*x;
}

// Depolarization functions for natural light
float depol(float2 polV) {
	return 0.5 * (polV.x + polV.y);
}

float3 depolColor(float3 colS, float3 colP) {
	return 0.5 * (colS + colP);
}

// Fresnel equations for dielectric/dielectric interfaces.
void fresnelDielectric(in float ct1, in float n1, in float n2, out float2 R, out float2 phi)
{
	float st1 = (1.0 - ct1 * ct1); // Sinus theta1 'squared'
	float nr = n1 / n2;

	if (sqr(nr) * st1 > 1)
	{
		// Total reflection
		float2 R = float2(1, 1);
		phi = 2.0 * atan2(-sqr(nr) * sqrt(st1 - 1.0 / sqr(nr)) / ct1, -sqrt(st1 - 1.0 / sqr(nr)) / ct1);
	}
	else
	{ 
		// Transmission & Reflection
		float ct2 = sqrt(1 - sqr(nr) * st1);
		float2 r = float2((n2 * ct1 - n1 * ct2) / (n2 * ct1 + n1 * ct2), (n1*ct1 - n2*ct2) / (n1 * ct1 + n2 * ct2));
		phi.x = PI; // (r.x < 0.0) ? PI : -PI;
		phi.y = (r.y < 0.0) ? PI : 0.0;
		R = sqr(r);
	}
}

// Fresnel equations for dielectric/conductor interfaces.
void fresnelConductor(in float ct1, in float n1, in float n2, in float k, out float2 R, out float2 phi) {

	if (k == 0.0) { // use dielectric formula to avoid numerical issues
		fresnelDielectric(ct1, n1, n2, R, phi);
		return;
	}

	float A = sqr(n2) * (1.0 - sqr(k)) - sqr(n1) * (1 - sqr(ct1));
	float B = sqrt(sqr(A) + sqr(2 * sqr(n2)*k));
	float U = sqrt((A + B) / 2.0);
	float V = sqrt((B - A) / 2.0);

	R.y = (sqr(n1*ct1 - U) + sqr(V)) / (sqr(n1*ct1 + U) + sqr(V));
	phi.y = atan2(2 * n1 * V*ct1, sqr(U) + sqr(V) - sqr(n1*ct1)) + PI;

	R.x = (sqr(sqr(n2) * (1.0 - sqr(k))*ct1 - n1*U) + sqr(2 * sqr(n2)*k*ct1 - n1*V))
		/ (sqr(sqr(n2) * (1.0 - sqr(k))*ct1 + n1*U) + sqr(2 * sqr(n2)*k*ct1 + n1*V));
	phi.x = atan2(2 * n1*sqr(n2)*ct1 * (2 * k*U - (1 - sqr(k))*V), sqr(sqr(n2)*(1 + sqr(k))*ct1) - sqr(n1)*(sqr(U) + sqr(V)));
}

// Evaluation XYZ sensitivity curves in Fourier space
float3 evalSensitivity(float opd, float shift) 
{
	// Use Gaussian fits, given by 3 parameters: val, pos and var
	float phase = 2 * PI * opd * 1.0e-6;
	float3 val = float3(5.4856e-13, 4.4201e-13, 5.2481e-13);
	float3 pos = float3(1.6810e+06, 1.7953e+06, 2.2084e+06);
	float3 var = float3(4.3278e+09, 9.3046e+09, 6.6121e+09);
	float3 xyz = val * sqrt(2 * PI * var) * cos(pos * phase + shift) * exp(-var * phase*phase);
	xyz.x += 9.7470e-14 * sqrt(2 * PI * 4.5282e+09) * cos(2.2399e+06 * phase + shift) * exp(-4.5282e+09 * phase*phase);
	return xyz / 1.0685e-7;
}

float3 iridescentFresnel(in BSDF bsdf)
{
	const row_major float3x3 XYZ_TO_RGB = float3x3(
		2.3706743, -0.5138850, 0.0052982,
		-0.9000405, 1.4253036, -0.0146949,
		-0.4706338, 0.0885814, 1.0093968);

	// Force eta_2 -> 1.0 when Dinc -> 0.0
	float eta_2 = lerp(1.0, eta2, smoothstep(0.0, 0.03, Dinc));

	// Compute dot products
	float NdotL = bsdf.LdotN; // dot(N,L);
	float NdotV = bsdf.VdotN; // dot(N,V);
	if (NdotL < 0 || NdotV < 0) 
		return 0.0;

	// float3 H = normalize(L + V);
	float NdotH = bsdf.NdotH; // dot(N,H);
	float cosTheta1 = bsdf.LdotH; // dot(H,L);
	float cosTheta2 = sqrt(1.0 - sqr(1.0 / eta_2) * (1.0 - sqr(cosTheta1)));

	// First interface
	float2 R12, phi12;
	fresnelDielectric(cosTheta1, 1.0, eta_2, R12, phi12);
	float2 R21 = R12;
	float2 T121 = 1.0 - R12;
	float2 phi21 = PI - phi12;

	// Second interface
	float2 R23, phi23;
	fresnelConductor(cosTheta2, eta_2, eta3, kappa3, R23, phi23);

	// Phase shift
	float OPD = Dinc * cosTheta2;
	float2 phi2 = phi21 + phi23;

	// Compound terms
	float3 I = 0.0;
	float2 R123 = R12 * R23;
	float2 r123 = sqrt(R123);
	float2 Rs = sqr(T121) * R23 / (1.0 - R123);

	// Reflectance term for m=0 (DC term amplitude)
	float2 C0 = R12 + Rs;
	float3 S0 = evalSensitivity(0.0, 0.0);
	I += depol(C0) * S0;

	// Reflectance term for m>0 (pairs of diracs)
	float2 Cm = Rs - T121;
	for (int m = 1; m <= 3; ++m)
	{
		Cm *= r123;
		float3 SmS = 2.0 * evalSensitivity(m * OPD, m * phi2.x);
		float3 SmP = 2.0 * evalSensitivity(m * OPD, m * phi2.y);
		I += depolColor(Cm.x * SmS, Cm.y * SmP);
	}

	return saturate(XYZ_TO_RGB * I);
}
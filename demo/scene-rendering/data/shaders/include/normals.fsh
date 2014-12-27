vec2 encodeNormal(in vec3 n)
{
	return vec2(n.xy / sqrt(8.0 * n.z + 8.0) + 0.5);
}

vec3 decodeNormal(in vec2 v)
{
	vec2 fenc = 4.0 * v - 2.0;
	float f = dot(fenc, fenc);
	return vec3(fenc * sqrt(1.0 - 0.25 * f), 1.0 - 0.5 * f);
}

vec3 randomVectorOnHemisphere(in vec3 normal, in vec3 noise)
{
	vec3 n = normalize(noise - 0.5);
	return n * sign(dot(n, normal));
}

vec3 randomVectorOnHemisphereWithScale(in vec3 normal, in vec3 noise, in float scale)
{
	vec3 n = normalize(noise - 0.5 + normal * scale);
	return n * sign(dot(n, normal));
}
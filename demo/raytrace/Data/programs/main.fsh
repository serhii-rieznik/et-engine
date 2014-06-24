uniform sampler2D noiseTexture;

uniform mat4 mModelViewProjection;
uniform mat4 mModelViewProjectionInverse;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;
uniform int maxBounces;

#define MAX_OBJECTS					20
#define ENABLE_SPHERICAL_LIGHT		1
#define PRODUCTION_RENDER			1

uniform int numSpheres;
uniform vec4 spheres[MAX_OBJECTS];
uniform vec4 sphereColors[MAX_OBJECTS];

uniform int numPlanes;
uniform vec4 planes[MAX_OBJECTS];
uniform vec4 planeColors[MAX_OBJECTS];

etFragmentIn vec2 vertex;

const float lightSphereRadius = 7.5f;
const vec4 lightColor = vec4(25.0);

const float pi = 3.1415926536;
const float halfPi = pi / 2.0;
const float doublePi = 2.0 * pi;

const float defaultIOR = 1.0 / 2.418;

const int missingObjectIndex = -1;
const int lightObjectIndex = -2;

float fresnel(in float IdotN, in float indexOfRefraction)
{
	float eta = indexOfRefraction * IdotN;
	float eta2 = eta * eta;
	float beta = 1.0 - indexOfRefraction * indexOfRefraction;
	float result = 1.0 + 2.0 * (eta2 + eta * sqrt(beta + eta2)) / beta;
	return clamp(result * result, 0.0, 1.0);
}

bool raySphere(in vec3 origin, in vec3 direction, in vec4 sphere, out vec3 intersectionPoint)
{
	vec3 dv = origin - sphere.xyz;
	
	float b = 2.0f * dot(direction, dv);
	if (b > 0.0) return false;
	
	float d = b * b - 4.0f * (dot(dv, dv) - sphere.w * sphere.w);
	if (d < 0.0) return false;
		
	intersectionPoint = origin - 0.5 * (sqrt(d) + b) * direction;
	return true;
}

bool rayPlane(in vec3 origin, in vec3 direction, in vec4 plane, inout vec3 intersectionPoint)
{
	float d = dot(direction, plane.xyz);
	if (d >= 0.0) return false;
	
	float t = dot(plane.xyz, plane.xyz * plane.w - origin) / d;
	if (t <= 0.0) return false;
	
	intersectionPoint = origin + t * direction;
	return true;
}

int findFirstIntersection(in vec3 origin, in vec3 direction, out vec3 intersectionPoint, out vec3 normal, out vec4 color)
{
	int result = missingObjectIndex;

	vec3 point = vec3(0.0);
	float minDistance = 10000000.0;
	
	for (int i = 0; i < numSpheres; ++i)
	{
		if (raySphere(origin, direction, spheres[i], point))
		{
			vec3 incidence = point - origin;
			float currentDistance = dot(incidence, incidence);
			if (currentDistance < minDistance)
			{
				minDistance = currentDistance;
				intersectionPoint = point;
				color = sphereColors[i];
				normal = normalize(intersectionPoint - spheres[i].xyz);
				result = i;
			}
		}
	}
	
	for (int i = 0; i < numPlanes; ++i)
	{
		if (rayPlane(origin, direction, planes[i], point))
		{
			vec3 incidence = point - origin;
			float currentDistance = dot(incidence, incidence);
			if (currentDistance < minDistance)
			{
				minDistance = currentDistance;
				intersectionPoint = point;
				color = planeColors[i];
				normal = planes[i].xyz;
				result = 100 + i;
			}
		}
	}
	
#if (ENABLE_SPHERICAL_LIGHT)
	if (raySphere(origin, direction, vec4(vPrimaryLight, lightSphereRadius), point))
	{
		vec3 incidence = point - origin;
		float currentDistance = dot(incidence, incidence);
		if (currentDistance < minDistance)
		{
			color = lightColor;
			intersectionPoint = point;
			normal = normalize(intersectionPoint - vPrimaryLight.xyz);
			result = lightObjectIndex;
		}
	}
#endif
	
	if (result == missingObjectIndex)
	{
		color = vec4(0.0);
		point = vec3(0.0);
		normal = vec3(0.0);
	}
	
	return result;
}

float computeShadowTerm(in vec3 point, in vec3 normal, in int objectIndex)
{
	vec3 shadowNormal;
	vec4 shadowColor;
	vec3 shadowIntersectionPoint;
	
	vec3 lightVector = normalize(point - vPrimaryLight);
	
	int shadowObjectIndex =
		findFirstIntersection(vPrimaryLight, lightVector, shadowIntersectionPoint, shadowNormal, shadowColor);
	
	return (shadowObjectIndex == objectIndex) ? 1.0 : 0.0;
}

float computeDiffuseTerm(in vec3 point, in vec3 normal, in int objectIndex)
{
	return computeShadowTerm(point, normal, objectIndex) * max(0.0, dot(normalize(vPrimaryLight - point), normal));
}

vec4 rayTracePreview(in vec3 origin, in vec3 direction)
{
	vec3 hitPoint;
	vec3 hitNormal;
	vec4 hitColor;
	
	int objectIndex = findFirstIntersection(origin, direction, hitPoint, hitNormal, hitColor);
	
	return (objectIndex == missingObjectIndex) ? vec4(0.0) :
		(hitColor * computeDiffuseTerm(hitPoint, hitNormal, objectIndex));
}

vec4 computeColorSequence(in vec3 origin, in vec3 direction)
{
	vec3 hitPoint;
	vec3 hitNormal;
	vec4 hitColor;
	
	int bounces = 0;
	int objectIndex = 0;
	
	vec4 result = vec4(1.0);
	while ((bounces < maxBounces) && (objectIndex >= 0))
	{
		objectIndex = findFirstIntersection(origin, direction, hitPoint, hitNormal, hitColor);
		direction = normalize(reflect(hitPoint - origin, hitNormal));
		origin = hitPoint;
		result *= hitColor;
		++bounces;
	}

	return result;
}

vec4 computeDiffuseColor(in vec3 point, in vec3 normal, float t)
{
#if (PRODUCTION_RENDER)
	const int maxSamples = 2048;
#else
	const int maxSamples = 512;
#endif
	
	vec3 samplingNormal = normal;
	
	vec4 result = vec4(0.0);
	float samples = 0.0;
	for (int i = 0; i < maxSamples; ++i)
	{
		vec4 randomSample = etTexture2D(noiseTexture, t * (point.xy - samplingNormal.yz + vec2(samplingNormal.z, samplingNormal.x)));
		vec3 randomNormal = normalize(randomSample.xyz - 0.5);
		
		float LdotN = dot(randomNormal, normal);
		samplingNormal = randomNormal * sign(LdotN);
		LdotN = abs(LdotN);
		
		result += LdotN * computeColorSequence(point, samplingNormal);
		samples += LdotN;
	}
	
	return result / samples;
}

vec4 rayTrace(in vec3 origin, in vec3 direction)
{
	vec3 hitPoint;
	vec3 hitNormal;
	vec4 hitColors[50];
	
	int objectIndex = findFirstIntersection(origin, direction, hitPoint, hitNormal, hitColors[0]);
	if (objectIndex == missingObjectIndex)
		return vec4(0.0);
	
	if (objectIndex == lightObjectIndex)
		return hitColors[0];
		
	int bounces = 0;
	while ((bounces < maxBounces) && (objectIndex != missingObjectIndex) && (objectIndex != lightObjectIndex))
	{
		vec3 incidence = normalize(hitPoint - origin);
		origin = hitPoint;
		
		hitColors[bounces].xyz *= computeDiffuseTerm(hitPoint, hitNormal, objectIndex);
		hitColors[bounces].w *= fresnel(dot(incidence, hitNormal), defaultIOR);
		
		++bounces;
		
		objectIndex = findFirstIntersection(origin, reflect(incidence, hitNormal), hitPoint, hitNormal, hitColors[bounces]);
	}
	
	vec4 result = hitColors[bounces - 1];
	while (bounces > 1)
	{
		result = mix(hitColors[bounces - 2], result, hitColors[bounces - 2].w);
		--bounces;
	}
	
	return result;
}

vec4 pathTrace(in vec3 origin, in vec3 direction, in float t)
{
	vec3 hitPoint;
	vec3 hitNormal;
	vec4 hitColor;
	
	int objectIndex = findFirstIntersection(origin, direction, hitPoint, hitNormal, hitColor);
	
	if ((objectIndex == missingObjectIndex) || (objectIndex == lightObjectIndex))
		return hitColor;

	return 1.0 - exp(-0.5 * hitColor * computeDiffuseColor(hitPoint, hitNormal, t));
}
	
vec4 performPT(vec2 coord, float t)
{
	vec4 farVertex = mModelViewProjectionInverse * vec4(coord, 1.0, 1.0);
	return pathTrace(vCamera, normalize(farVertex.xyz / farVertex.w - vCamera), t);
}

vec4 performRT(vec2 coord)
{
	vec4 farVertex = mModelViewProjectionInverse * vec4(coord, 1.0, 1.0);
	return rayTrace(vCamera, normalize(farVertex.xyz / farVertex.w - vCamera));
}

vec4 performRTPreview(vec2 coord)
{
	vec4 farVertex = mModelViewProjectionInverse * vec4(coord, 1.0, 1.0);
	return rayTracePreview(vCamera, normalize(farVertex.xyz / farVertex.w - vCamera));
}

void main()
{
	float t = 1.0 + 1.0 / (1.0 + float(maxBounces));
#if (PRODUCTION_RENDER)
	float d = 1.0 / 1024.0;
	vec4 p1 = performPT(vertex + vec2(-d, -d), 0.85 * t);
	vec4 p2 = performPT(vertex + vec2(-d,  d), t);
	vec4 p3 = performPT(vertex + vec2( d, -d), 1.33333 * t);
	vec4 p4 = performPT(vertex + vec2( d,  d), 1.4 * t);
	etFragmentOut = 0.25 * (p1 + p2 + p3 + p4);
#else
	etFragmentOut = performPT(vertex, t);
#endif
}

#define MAX_OBJECTS					50
#define ENABLE_SPHERICAL_LIGHT		1
#define PRODUCTION_RENDER			0

uniform sampler2D noiseTexture;

uniform mat4 mModelViewProjectionInverse;
uniform vec3 vCamera;
uniform vec3 vPrimaryLight;

uniform vec4 planes[MAX_OBJECTS];
uniform vec4 spheres[MAX_OBJECTS];

uniform vec4 planeColors[MAX_OBJECTS];
uniform vec4 sphereColors[MAX_OBJECTS];

uniform int numPlanes;
uniform int numSpheres;
uniform int maxBounces;

etFragmentIn vec2 vertex;

const float lightSphereRadius = 10.0f;
const vec4 lightColor = vec4(25.0);
const int missingObjectIndex = -1;
const int lightObjectIndex = -2;

bool raySphere(in vec3 origin, in vec3 direction, in vec4 sphere, out vec3 intersectionPoint, out vec3 incidence)
{
	vec3 dv = origin - sphere.xyz;
	
	float b = dot(direction, dv);
	if (b < 0.0)
	{
		float d = b * b - (dot(dv, dv) - sphere.w * sphere.w);
		if (d >= 0.0)
		{
			incidence = -(b + sqrt(d)) * direction;
			intersectionPoint = origin + incidence;
			return true;
		}
	}
	return false;
}

bool rayPlane(in vec3 origin, in vec3 direction, in vec4 plane, out vec3 intersectionPoint, out vec3 incidence)
{
	float d = dot(direction, plane.xyz);
	if (d < 0.0)
	{
		incidence = (dot(plane.xyz, plane.xyz * plane.w - origin) / d) * direction;
		intersectionPoint = origin + incidence;
		return true;
	}
	return false;
}

int findFirstIntersection(in vec3 origin, in vec3 direction, out vec3 intersectionPoint, out vec3 normal, out vec4 color)
{
	int result = missingObjectIndex;

	float minDistance = 10000000.0;
	float currentDistance = minDistance;
	
	vec3 incidence;
	vec3 point = vec3(0.0);
	
	for (int i = 0; i < numSpheres; ++i)
	{
		if (raySphere(origin, direction, spheres[i], point, incidence))
		{
			if ((currentDistance = dot(incidence, incidence)) < minDistance)
			{
				minDistance = currentDistance;
				intersectionPoint = point;
				color = sphereColors[i];
				normal = intersectionPoint - spheres[i].xyz;
				result = i;
			}
		}
	}
	
	for (int i = 0; i < numPlanes; ++i)
	{
		if (rayPlane(origin, direction, planes[i], point, incidence))
		{
			if ((currentDistance = dot(incidence, incidence)) < minDistance)
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
	if (raySphere(origin, direction, vec4(vPrimaryLight, lightSphereRadius), point, incidence))
	{
		if (dot(incidence, incidence) < minDistance)
		{
			color = lightColor;
			intersectionPoint = point;
			normal = intersectionPoint - vPrimaryLight.xyz;
			result = lightObjectIndex;
		}
	}
#endif
	
	normal = normalize(normal);
	return result;
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

	result.w = 1.0;
	return result;
}

vec4 computeDiffuseColor(in vec3 point, in vec3 normal, float t)
{
#if (PRODUCTION_RENDER)
	const int maxSamples = 2048;
#else
	int maxSamples = 2 * maxBounces * maxBounces;
#endif
	
	vec3 samplingNormal = normal;
	
	vec4 result = vec4(0.0);
	for (int i = 0; i < maxSamples; ++i)
	{
		vec3 randomNormal = normalize(etTexture2D(noiseTexture, t * (point.xy - samplingNormal.yz + samplingNormal.zx)).xyz - 0.5);
		float LdotN = dot(randomNormal, normal);
		samplingNormal = sign(LdotN) * randomNormal;
		result += abs(LdotN) * computeColorSequence(point, samplingNormal);
	}
	
	return result / result.w;
}

vec4 pathTrace(in vec3 origin, in vec3 direction, in float t)
{
	vec3 hitPoint;
	vec3 hitNormal;
	vec4 hitColor;
	findFirstIntersection(origin, direction, hitPoint, hitNormal, hitColor);
	return 1.0 - exp(-0.5 * (hitColor * computeDiffuseColor(hitPoint, hitNormal, t)));
}
	
vec4 performPT(vec2 coord, float t)
{
	vec4 farVertex = mModelViewProjectionInverse * vec4(coord, 1.0, 1.0);
	return pathTrace(vCamera, normalize(farVertex.xyz / farVertex.w - vCamera), t);
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

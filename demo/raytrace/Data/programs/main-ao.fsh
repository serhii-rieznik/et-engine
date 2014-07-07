uniform mat4 mModelViewProjection;
uniform mat4 mModelViewProjectionInverse;
uniform vec3 vCamera;

uniform vec3 sphere1Center;
uniform float sphere1Radius;

uniform vec3 sphere2Center;
uniform float sphere2Radius;

etFragmentIn vec2 vertex;

const vec4 planeEquation = vec4(normalize(vec3(0.0, 1.0, 0.0)), 0.0);

const float pi = 3.1415926536;
const float halfPi = pi / 2.0;
const float doublePi = 2.0 * pi;
const float dTheta = halfPi / 45.0;
const float dPhi = doublePi / 180.0;
const vec4 planeColor = vec4(0.25, 0.5, 0.25, 1.0);
const vec4 sphere1Color = vec4(1.0, 0.25, 0.0, 1.0);
const vec4 sphere2Color = vec4(0.0, 0.25, 1.0, 1.0);
const float shadowsTreshold = 0.0;
const float reflectionsTreshold = 0.8;

const int missingObjectIndex = -1;

const int planeIndex = 0;
const int sphere1Index = 1;
const int sphere2Index = 2;

const float defaultIOR = 1.0 / 2.418;

float fresnel(in float IdotN, in float indexOfRefraction)
{
	etMediump float eta = indexOfRefraction * IdotN;
	etMediump float eta2 = eta * eta;
	etMediump float beta = 1.0 - indexOfRefraction * indexOfRefraction;
	etMediump float result = 1.0 + 2.0 * (eta2 + eta * sqrt(beta + eta2)) / beta;
	return clamp(result * result, 0.0, 1.0);
}

bool raySphere(in vec3 origin, in vec3 direction, in vec3 center, in float radius, inout vec3 intersectionPoint)
{
	vec3 dv = origin - center;
	
	float b = 2.0f * dot(direction, dv);
	if (b > 0.0) return false;
	
	float d = b * b - 4.0f * (dot(dv, dv) - radius * radius);
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
	
	vec3 dp = intersectionPoint - plane.xyz * plane.w;
	
	return (dot(dp, dp) < 49.0);
}

bool missSphere(in vec3 origin, in vec3 direction, in vec3 center, in float radius)
{
	vec3 dv = origin - center;
	
	float b = 2.0f * dot(direction, dv);
	if (b > 0.0) return true;
	
	float d = b * b - 4.0f * (dot(dv, dv) - radius * radius);
	return (d < 0.0);
}

bool missPlane(in vec3 origin, in vec3 direction, in vec4 plane)
{
	float d = dot(direction, plane.xyz);
	if (d >= 0.0f) return true;
	
	float t = dot(plane.xyz, plane.xyz * plane.w - origin) / d;
	if (t <= 0.0f) return true;
	
	return false;
}

float gatherSphereAO(in vec3 point, in vec3 baseNormal, in vec3 center, in float radius, in float threshold)
{
	float accum = 0.0;
	float samples = 0.0;

	float theta = -halfPi;
	while (theta <= halfPi)
	{
		float phi = 0;
		while (phi < doublePi)
		{
			vec3 direction = vec3(cos(phi) * cos(theta), sin(theta), sin(phi) * cos(theta));
			if (dot(direction, baseNormal) >= threshold)
			{
				if (missSphere(point, direction, center, radius))
					accum += 1.0;
				samples += 1.0;
			}
			phi += dPhi;
		}
		theta += dTheta;
	}
	
	return accum / samples;
}

float gatherPlaneAO(in vec3 point, in vec3 baseNormal, in vec4 plane, in float threshold)
{
	float accum = 0.0;
	float samples = 0.0;
	
	float theta = -halfPi;
	while (theta <= halfPi)
	{
		float phi = 0;
		while (phi < doublePi)
		{
			vec3 direction = vec3(cos(phi) * cos(theta), sin(theta), sin(phi) * cos(theta));
			if (dot(direction, baseNormal) >= threshold)
			{
				if (missPlane(point, direction, plane))
					accum += 1.0;
				samples += 1.0;
			}
			phi += dPhi;
		}
		theta += dTheta;
	}
	
	return accum / samples;
}

vec4 performAO(vec2 coord)
{
	vec4 result = vec4(0.1, 0.2, 0.3, 1.0); // default color
	
	vec4 farVertex = mModelViewProjectionInverse * vec4(coord, 1.0, 1.0);
	vec3 veryFarPoint = 1000000.0 * farVertex.xyz / farVertex.w;
	vec3 direction = normalize(veryFarPoint - vCamera);

	vec3 planePoint = veryFarPoint;
	vec3 sphere1Point = veryFarPoint;
	vec3 sphere2Point = veryFarPoint;
	
	bool plane = rayPlane(vCamera, direction, planeEquation, planePoint);
	bool sphere1 = raySphere(vCamera, direction, sphere1Center, sphere1Radius, sphere1Point);
	bool sphere2 = raySphere(vCamera, direction, sphere2Center, sphere2Radius, sphere2Point);
	
	if (plane || sphere1 || sphere2)
	{
		vec3 dp = planePoint - vCamera;
		vec3 ds1 = sphere1Point - vCamera;
		vec3 ds2 = sphere2Point - vCamera;
		
		float distanceToPlane = dot(dp, dp);
		float distanceToSphere1 = dot(ds1, ds1);
		float distanceToSphere2 = dot(ds2, ds2);
		
		float minDistance = min(distanceToSphere1, min(distanceToSphere2, distanceToPlane));
		
		if (minDistance == distanceToPlane)
		{
			vec3 shadowNormal = planeEquation.xyz;
			vec3 reflectiveNormal = reflect(normalize(dp), planeEquation.xyz);
			
			float shadow = gatherSphereAO(planePoint, shadowNormal, sphere1Center, sphere1Radius, shadowsTreshold) *
				gatherSphereAO(planePoint, shadowNormal, sphere2Center, sphere2Radius, shadowsTreshold);
			
			float r1 = gatherSphereAO(planePoint, reflectiveNormal, sphere1Center, sphere1Radius, reflectionsTreshold);
			float r2 = gatherSphereAO(planePoint, reflectiveNormal, sphere2Center, sphere2Radius, reflectionsTreshold);
			
			result = shadow * mix(planeColor, sphere1Color, 0.5 - 0.5 * r1) * mix(planeColor, sphere2Color, 0.5 - 0.5 * r2); // shadow * mix(planeColor, sphere1Color, r1);
		}
		else if (minDistance == distanceToSphere1)
		{
			vec3 normal = normalize(sphere1Point - sphere1Center);
			
			float planeShadow = gatherPlaneAO(sphere1Point, normal, planeEquation, reflectionsTreshold);
			float sphereShadow = gatherSphereAO(sphere1Point, normal, sphere2Center, sphere2Radius, shadowsTreshold);
			
			result = mix(mix(sphere1Color, planeColor, 0.5 - 0.5 * planeShadow), sphere2Color, 0.5 - 0.5 * sphereShadow) * (planeShadow * sphereShadow);
		}
		else if (minDistance == distanceToSphere2)
		{
			vec3 normal = normalize(sphere2Point - sphere2Center);
			
			float planeShadow = gatherPlaneAO(sphere2Point, normal, planeEquation, reflectionsTreshold);
			float sphereShadow = gatherSphereAO(sphere2Point, normal, sphere1Center, sphere1Radius, shadowsTreshold);
			
			result = mix(mix(sphere2Color, planeColor, 0.5 - 0.5 * planeShadow), sphere1Color, 0.5 - 0.5 * sphereShadow) * (planeShadow * sphereShadow);
		}
	}
	
	return result;
}

int findFirstIntersection(in vec3 origin, in vec3 direction, inout vec3 point)
{
	vec3 veryFarPoint = origin + 1000000.0 * direction;
	
	vec3 planePoint = veryFarPoint;
	vec3 sphere1Point = veryFarPoint;
	vec3 sphere2Point = veryFarPoint;
	
	bool plane = rayPlane(origin, direction, planeEquation, planePoint);
	bool sphere1 = raySphere(origin, direction, sphere1Center, sphere1Radius, sphere1Point);
	bool sphere2 = raySphere(origin, direction, sphere2Center, sphere2Radius, sphere2Point);
	
	if (plane)
	{
		if (sphere1)
		{
			if (sphere2)
			{
				vec3 d1 = origin - planePoint;
				vec3 d2 = origin - sphere1Point;
				vec3 d3 = origin - sphere2Point;
				float s1 = dot(d1, d1);
				float s2 = dot(d2, d2);
				float s3 = dot(d3, d3);
				float minDistance = min(s1, min(s2, s3));
				if (minDistance == s1)
				{
					point = planePoint;
					return planeIndex;
				}
				else if (minDistance == s2)
				{
					point = sphere1Point;
					return sphere1Index;
				}
				else
				{
					point = sphere2Point;
					return sphere2Index;
				}
			}
			else
			{
				vec3 d1 = origin - planePoint;
				vec3 d2 = origin - sphere1Point;
				if (dot(d1, d1) < dot(d2, d2))
				{
					point = planePoint;
					return planeIndex;
				}
				else
				{
					point = sphere1Point;
					return sphere1Index;
				}
			}
		}
		else if (sphere2)
		{
			vec3 d1 = origin - planePoint;
			vec3 d2 = origin - sphere2Point;
			if (dot(d1, d1) < dot(d2, d2))
			{
				point = planePoint;
				return planeIndex;
			}
			else
			{
				point = sphere2Point;
				return sphere2Index;
			}
		}
		else
		{
			point = planePoint;
			return planeIndex;
		}
	}
	else if (sphere1)
	{
		if (sphere2)
		{
			vec3 d1 = origin - sphere1Point;
			vec3 d2 = origin - sphere2Point;
			if (dot(d1, d1) < dot(d2, d2))
			{
				point = sphere1Point;
				return sphere1Index;
			}
			else
			{
				point = sphere2Point;
				return sphere2Index;
			}
		}
		else
		{
			point = sphere1Point;
			return sphere1Index;
		}
	}
	else if (sphere2)
	{
		point = sphere2Point;
		return sphere2Index;
	}

	return missingObjectIndex;
}

vec3 normalFromObjectIndexAndPoint(in int objectIndex, in vec3 point)
{
	if (objectIndex == planeIndex)
		return planeEquation.xyz;
	
	if (objectIndex == sphere1Index)
		return normalize(point - sphere1Center);
	
	if (objectIndex == sphere2Index)
		return normalize(point - sphere2Center);
	
	return vec3(0.0);
}

vec4 colorFromObjectIndex(in int objectIndex)
{
	if (objectIndex == planeIndex)
		return planeColor;
	
	if (objectIndex == sphere1Index)
		return sphere1Color;
	
	if (objectIndex == sphere2Index)
		return sphere2Color;
	
	return vec4(1.0);
}

vec4 performRT(vec2 coord)
{
	const float maxSamples = 10.0;
	
	vec4 farVertex = mModelViewProjectionInverse * vec4(coord, 1.0, 1.0);
	vec3 direction = normalize(farVertex.xyz / farVertex.w - vCamera);
	vec3 origin = vCamera;
	vec3 intersectionPoint = vec3(0.0);
	
	vec4 result = colorFromObjectIndex(missingObjectIndex);
	float samples = 0.0;
	
	float mixCoefficient = 1.0;
	int objectIndex = findFirstIntersection(origin, direction, intersectionPoint);
	while ((objectIndex != missingObjectIndex) && (samples < maxSamples))
	{
		vec3 normal = normalFromObjectIndexAndPoint(objectIndex, intersectionPoint);
		vec3 incidence = normalize(intersectionPoint - origin);
		vec4 hitColor = colorFromObjectIndex(objectIndex);
		
		result = mix(result, hitColor, mixCoefficient);
		mixCoefficient = fresnel(dot(incidence, normal), defaultIOR);
		
		direction = normalize(reflect(incidence, normal));
		origin = intersectionPoint;
		
		objectIndex = findFirstIntersection(origin, direction, intersectionPoint);
		
		samples += 1.0;
	}
	
	return result;
}

#define ENABLE_MULTISAMPLE	1

void main()
{
#if (ENABLE_MULTISAMPLE)
	float d = 1.0 / 1024.0;
	vec4 p1 = performRT(vertex + vec2(-d, -d));
	vec4 p2 = performRT(vertex + vec2(-d,  d));
	vec4 p3 = performRT(vertex + vec2( d, -d));
	vec4 p4 = performRT(vertex + vec2( d,  d));
	etFragmentOut = 0.25 * (p1 + p2 + p3 + p4);
#else
	etFragmentOut = performRT(vertex);
#endif
}

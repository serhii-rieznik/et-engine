uniform vec2 clipPlanes;
uniform vec2 texCoordScales;

#define NEAR	clipPlanes.x
#define FAR		clipPlanes.y

float restoreViewSpaceDistance(in float depth)
{
	return (2.0 * NEAR * FAR) / (depth * (FAR - NEAR) - FAR - NEAR);
}

vec3 restoreViewSpacePosition(in vec2 texCoords, in float depth)
{
	return vec3(texCoords * texCoordScales, 1.0) * restoreViewSpaceDistance(depth);
}

float projectViewSpaceDistance(in float z)
{
	return (2.0 * NEAR * FAR / z + FAR + NEAR) / (FAR - NEAR);
}

vec3 projectViewSpacePosition(in vec3 viewSpace)
{
	return 0.5 + 0.5 * vec3((viewSpace.xy / texCoordScales) / viewSpace.z, projectViewSpaceDistance(viewSpace.z));
}

uniform vec2 clipPlanes;

const vec3 normalBias = vec3(2.0, 2.0, -2.0);
const vec3 normalScale = vec3(1.0, 1.0, -1.0);

vec3 restoreViewSpacePosition2(in vec2 texCoords, in float depth)
{
	float z = (clipPlanes.x * clipPlanes.y) / (depth * (clipPlanes.y - clipPlanes.x) - clipPlanes.y);
	return vec3(-texCoords, 1.0) * z;
}

vec3 restoreViewSpacePosition1(in vec2 texCoords, in float depth)
{
	float z = (clipPlanes.x * clipPlanes.y) / (clipPlanes.y - depth * (clipPlanes.y - clipPlanes.x));
	return vec3(texCoords, 1.0) * z;
}

vec3 projectViewSpacePosition1(in vec3 viewSpace)
{
	float depth = (clipPlanes.y - clipPlanes.x * clipPlanes.y / viewSpace.z) / (clipPlanes.y - clipPlanes.x);
	return 0.5 + 0.5 * vec3(viewSpace.xy / viewSpace.z, depth);
}

float restoreViewSpaceDepth(in float depth)
{
	return (clipPlanes.x * clipPlanes.y) / (depth * (clipPlanes.y - clipPlanes.x) - clipPlanes.y);
}

vec3 restoreViewSpacePosition(in vec2 texCoords, in float depth)
{
	return vec3(texCoords, 1.0) * restoreViewSpaceDepth(depth);
}

vec3 projectViewSpacePosition(in vec3 viewSpace)
{
	float depth = (clipPlanes.x * clipPlanes.y / viewSpace.z + clipPlanes.y) / (clipPlanes.y - clipPlanes.x);
	return 0.5 + 0.5 * vec3(viewSpace.xy / viewSpace.z, depth);
}

uniform mat4 mModelViewProjection;
uniform mat4 mModelViewProjectionInverse;

vec3 restoreWorldSpacePosition(in vec2 texCoords, in float depth)
{
	vec4 result = mModelViewProjectionInverse * vec4(texCoords, depth, 1.0);
	return result.xyz / result.w;
}

vec3 projectWorldSpacePosition(in vec3 worldSpace)
{
	vec4 result = mModelViewProjection * vec4(worldSpace, 1.0);
	return 0.5 + result.xyz * (0.5 / result.w);
}

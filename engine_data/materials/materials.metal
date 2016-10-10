#include "shaders.inl"

struct VSOutput
{
	FLOAT(4) position [[position]];
	FLOAT(4) worldPosition;

#if (NORMAL_SIZE > 0)
	FLOAT(NORMAL_SIZE) normal;
#endif

#if (TEXCOORD0_SIZE > 0)
	FLOAT(TEXCOORD0_SIZE) texCoord0;
#endif

#if (TEXCOORD1_SIZE > 0)
	FLOAT(TEXCOORD1_SIZE) texCoord1;
#endif

#if (TEXCOORD2_SIZE > 0)
	FLOAT(TEXCOORD2_SIZE) texCoord2;
#endif

#if (TEXCOORD3_SIZE > 0)
	FLOAT(TEXCOORD3_SIZE) texCoord3;
#endif

#if (USING_LIGHT_POSITION)
	FLOAT(4) lightPosition;
#endif

#if (USING_CAMERA_POSITION)
	FLOAT(3) cameraPosition;
#endif
};

struct MaterialVariables
{
	float4 diffuseColor;

#if (USING_ROUGHNESS)
	float roughness;
#endif
};

struct ObjectVariables
{
	float4x4 worldTransform;
};

vertex VSOutput vertexMain(VSInput vsInput [[stage_in]]
	, constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]]
	, constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]]
	)
{
	VSOutput vOut;

	constant float4x4& w = objectVariables.worldTransform;
	vOut.worldPosition = w * float4(vsInput.position, 1.0);

	constant float4x4& vp = passVariables.viewProjection;
	vOut.position = vp * vOut.worldPosition;

#if (NORMAL_SIZE > 0)
	float3x3 rotationMatrix = float3x3(w[0].xyz, w[1].xyz, w[2].xyz);
	vOut.normal = rotationMatrix * FLOAT(NORMAL_SIZE)(vsInput.normal);
#endif

#if (TEXCOORD0_SIZE > 0)
	vOut.texCoord0 = vsInput.texCoord0;
#endif

#if (USING_LIGHT_POSITION)
	vOut.lightPosition = passVariables.lightPosition;
#endif

#if (USING_CAMERA_POSITION)
	vOut.cameraPosition = passVariables.cameraPosition.xyz;
#endif

	return vOut;
}

struct LightVariables
{
	float3 normal;
	float3 lightDirection;
	float3 viewDirection;
	float3 halfVector;
	float LdotN;
	float VdotN;
	float HdotN;
	float HdotL;
};

LightVariables buildLightVariables(VSOutput fragmentIn);
float4 getDiffuseLighting(LightVariables light, constant MaterialVariables& materialVariables);

fragment float4 fragmentMain(VSOutput fragmentIn [[stage_in]]
	, constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]]
#if (ALBEDO_TEXTURE)
	, texture2d<float> albedoTexture [[texture(0)]]
	, sampler albedoSampler [[sampler(0)]]
#endif
	)
{
#if (DISPLAY_NORMALS)
	return float4(0.5 + 0.5 * normalize(fragmentIn.normal), 1.0);
#else

	LightVariables light = buildLightVariables(fragmentIn);
	float4 result = getDiffuseLighting(light, materialVariables);

#if (ALBEDO_TEXTURE)
	EXPECT_TEXCOORD0;
	result *= albedoTexture.sample(albedoSampler, fragmentIn.texCoord0);
#endif

	return result;
#endif
}

LightVariables buildLightVariables(VSOutput fragmentIn)
{
	LightVariables result;

#if (DIFFUSE_LAMBERT || DIFFUSE_BURLEY)
	result.normal = normalize(fragmentIn.normal);
	result.lightDirection = normalize(fragmentIn.lightPosition.xyz - fragmentIn.worldPosition.xyz * fragmentIn.lightPosition.w);
	result.LdotN = max(0.0, dot(result.lightDirection, result.normal));
#endif

#if (DIFFUSE_BURLEY)
	result.viewDirection = normalize(fragmentIn.cameraPosition - fragmentIn.worldPosition.xyz);
	result.halfVector = normalize(result.viewDirection + result.lightDirection);
	result.VdotN = max(0.0, dot(result.viewDirection, result.normal));
	result.HdotN = max(0.0, dot(result.halfVector, result.normal));
	result.HdotL = max(0.0, dot(result.halfVector, result.lightDirection));
#endif

	return result;
}

float4 getDiffuseLighting(LightVariables light, constant MaterialVariables& materialVariables)
{
#if (DIFFUSE_LAMBERT)

	return float4(light.LdotN, light.LdotN, light.LdotN, 1.0);

#elif (DIFFUSE_BURLEY)

	float diffuse = INV_PI * burleyDiffuse(light.LdotN, light.VdotN, light.HdotL, materialVariables.roughness);
	return float4(diffuse, diffuse, diffuse, 1.0);

#else

	return float4(1.0);

#endif
}

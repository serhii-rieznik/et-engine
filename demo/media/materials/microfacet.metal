#define VertexStreamBufferIndex 0
#define ObjectVariablesBufferIndex 4
#define MaterialVariablesBufferIndex 5
#define PassVariablesBufferIndex 6

struct PassVariables
{
	float4x4 viewProjection;
	float4x4 projection;
	float4x4 view;
	float4 cameraPosition;
	float4 cameraDirection;
	float4 cameraUp;
	float4 lightPosition;
};

struct MaterialVariables
{
	float4 diffuseColor;
	float4 specularColor;
	float3 lightPosition;
	float roughness;
};

struct ObjectVariables
{
	float4x4 worldTransform;
};

struct VSInput
{
	packed_float3 position;
	packed_float3 normal;
	packed_float2 texCoord;
};

struct VSOutput
{
	float4 position [[position]];
	float3 normal;
	float3 cameraDirection;
	float3 lightDirection;
	float2 texCoord;
};

vertex VSOutput vertexMain(device VSInput* vsInput [[buffer(VertexStreamBufferIndex)]],
	constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
	constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]],
	constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]],
	uint vertexId [[vertex_id]])
{
	constant float4x4& w = objectVariables.worldTransform;
	constant float4x4& vp = passVariables.viewProjection;

	float3x3 rotationMatrix = float3x3(w[0].xyz, w[1].xyz, w[2].xyz);
	float4 transformedVertex = w * float4(vsInput[vertexId].position, 1.0);

	VSOutput vOut;
	vOut.position = vp * transformedVertex;
	vOut.normal = rotationMatrix * float3(vsInput[vertexId].normal);
	vOut.cameraDirection = passVariables.cameraPosition.xyz - transformedVertex.xyz;
	vOut.lightDirection = passVariables.lightPosition.xyz - transformedVertex.xyz * passVariables.lightPosition.w;
	vOut.texCoord = vsInput[vertexId].texCoord;
	return vOut;
}

/*
 * Fragment shader
 */
constant float invPi = 0.3183098862;

float fresnelShlickApproximation(float f0, float cosTheta);
float burleyDiffuse(float LdotN, float VdotN, float LdotH, float alpha);
float smithGGX(float a2, float cosTheta);
float microfacetSpecular(float NdotL, float NdotV, float NdotH, float alpha);
float remapSpecularPowerToRoughness(float Ns);

fragment float4 fragmentMain(VSOutput fragmentIn [[stage_in]],
	constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
	constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]],
	constant PassVariables& passVariables [[buffer(PassVariablesBufferIndex)]],
	texture2d<float> albedoTexture [[texture(0)]])
{
	constexpr sampler defaultSampler(coord::normalized, address::repeat, filter::linear);
	float4 albedo = albedoTexture.sample(defaultSampler, fragmentIn.texCoord);

	float3 vNormal = normalize(fragmentIn.normal);
	float3 vView = normalize(fragmentIn.cameraDirection);
	float3 vLight = normalize(fragmentIn.lightDirection);
	float3 vHalf = normalize(vView + vLight);

	float NdotV = max(0.00001, dot(vNormal, vView));
	float NdotL = max(0.0, dot(vNormal, vLight));
	float HdotL = dot(vHalf, vLight);
	float NdotH = dot(vNormal, vHalf);

	float alpha = remapSpecularPowerToRoughness(materialVariables.roughness);
	float kS = microfacetSpecular(NdotL, NdotV, NdotH, alpha) * invPi;
	float kD = burleyDiffuse(NdotL, NdotV, HdotL, alpha) * invPi;

	return albedo * materialVariables.diffuseColor * kD;
	// ; // * kD + materialVariables.specularColor * kS;
}

/*
 * Helper functions
 */
float fresnelShlickApproximation(float f0, float cosTheta)
{
	return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);
}

float burleyDiffuse(float LdotN, float VdotN, float LdotH, float alpha)
{
	alpha = 1.0 - pow(alpha, 4.0);
	float fl = pow(1.0 - LdotN, 5.0);
	float fv = pow(1.0 - VdotN, 5.0);
	float fd90 = 0.5 + alpha * (LdotH * LdotH);
	return (1.0 - fd90 * fl) * (1.0 + fd90 * fv);
}

float smithGGX(float a2, float cosTheta)
{
	cosTheta *= cosTheta;
	return 2.0 / (1.0 + sqrt(1.0 + a2 * (1.0 - cosTheta) / cosTheta));
}

float microfacetSpecular(float NdotL, float NdotV, float NdotH, float alpha)
{
	float a2 = alpha * alpha;
	float denom = 1.0 + NdotH * NdotH * (a2 - 1.0);
	float ggxD = a2 / (denom * denom);
	float ggxF = fresnelShlickApproximation(0.2f, NdotV);
	float ggxV = smithGGX(a2, NdotV) * smithGGX(a2, NdotL);
	return (ggxF * ggxD * ggxV) / max(0.00001, 4.0 * NdotL * NdotV);
}

float remapSpecularPowerToRoughness(float Ns)
{
	return 1.0 / (Ns + 1.0);
}

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
};

struct Variables
{
	float4x4 viewProjection;
	float4x4 transform;
	packed_float3 cameraPosition;
	packed_float3 lightPosition;
	float roughness;
};

vertex VSOutput vertexMain(constant VSInput* vsInput [[buffer(0)]],
						   constant Variables& variables [[buffer(1)]],
						   uint vertexId [[vertex_id]])
{
	float3x3 rotationMatrix = float3x3(variables.transform[0].xyz, variables.transform[1].xyz, variables.transform[2].xyz);
	float4 transformedVertex = variables.transform * float4(vsInput[vertexId].position, 1.0);

	VSOutput vOut;
	vOut.position = variables.viewProjection * transformedVertex;
	vOut.normal = rotationMatrix * float3(vsInput[vertexId].normal);
	vOut.cameraDirection = variables.cameraPosition - transformedVertex.xyz;
	vOut.lightDirection = variables.lightPosition - transformedVertex.xyz;
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

fragment float4 fragmentMain(VSOutput fragmentIn [[stage_in]], constant Variables& variables [[buffer(1)]])
{
	const float4 diffuseColor = float4(1.0);
	const float4 specularColor = float4(1.0);

	float3 vNormal = normalize(fragmentIn.normal);
	float3 vView = normalize(fragmentIn.cameraDirection);
	float3 vLight = normalize(fragmentIn.lightDirection);
	float3 vHalf = normalize(vView + vLight);

	float NdotV = max(0.00001, dot(vNormal, vView));
	float NdotL = max(0.0, dot(vNormal, vLight));
	float HdotL = dot(vHalf, vLight);
	float NdotH = dot(vNormal, vHalf);

	float kS = microfacetSpecular(NdotL, NdotV, NdotH, variables.roughness) * invPi;
	float kD = burleyDiffuse(NdotL, NdotV, HdotL, variables.roughness) * invPi;

	return diffuseColor * kD + specularColor * kS;
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

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
	float3 lightDirection;
	float2 texCoord;
};

vertex VSOutput vertexMain(device VSInput* vsInput [[buffer(VertexStreamBufferIndex)]],
						   constant ObjectVariables& objectVariables [[buffer(ObjectVariablesBufferIndex)]],
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
	vOut.lightDirection = passVariables.lightPosition.xyz - transformedVertex.xyz * passVariables.lightPosition.w;
	vOut.texCoord = vsInput[vertexId].texCoord;
	return vOut;
}

fragment float4 fragmentMain(VSOutput fragmentIn [[stage_in]],
	constant MaterialVariables& materialVariables [[buffer(MaterialVariablesBufferIndex)]],
	texture2d<float> albedoTexture [[texture(0)]],
	sampler albedoSampler [[sampler(0)]])
{
	float4 albedo = albedoTexture.sample(albedoSampler, fragmentIn.texCoord);
	return albedo * materialVariables.diffuseColor;
}

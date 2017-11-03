VSOutput vertexMain(uint vertexIndex : SV_VertexID)
{
	float2 pos = float2((vertexIndex << 1) & 2, vertexIndex & 2) * 2.0 - 1.0;

	VSOutput vsOut;
	vsOut.texCoord0 = pos * 0.5 + 0.5;
	vsOut.position = float4(pos, 0.0, 1.0);
	return vsOut;
}

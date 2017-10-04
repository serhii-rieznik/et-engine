VSOutput vertexMain(VSInput vsIn)
{
	VSOutput vsOut;
	vsOut.texCoord0 = vsIn.position * 0.5 + 0.5;
	vsOut.position = float4(vsIn.position, 1.0);
	return vsOut;
}

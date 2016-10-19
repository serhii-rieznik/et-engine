struct VSOutput
{
	float4 position [[position]];

#if (TEXCOORD0_SIZE > 0)
	FLOAT(TEXCOORD0_SIZE) texCoord0;
#endif
};

struct FSOutput
{
	float4 color0 [[color0]];
};

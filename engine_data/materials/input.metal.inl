struct VSInput
{
#if (POSITION_SIZE == 0)
	#error POSITION SIZE NOT DEFINED
#else
    FLOAT(POSITION_SIZE) position [[attribute(PositionAttribute)]];
#endif

#if (NORMAL_SIZE > 0)
    FLOAT(NORMAL_SIZE) normal [[attribute(NormalAttribute)]];
#endif

#if (TEXCOORD0_SIZE > 0)
    FLOAT(TEXCOORD0_SIZE) texCoord0 [[attribute(TexCoord0)]];
#endif

#if (TEXCOORD1_SIZE > 0)
	FLOAT(TEXCOORD1_SIZE) texCoord1 [[attribute(TexCoord1)]];
#endif

#if (TEXCOORD2_SIZE > 0)
	FLOAT(TEXCOORD2_SIZE) texCoord2 [[attribute(TexCoord2)]];
#endif

#if (TEXCOORD3_SIZE > 0)
	FLOAT(TEXCOORD3_SIZE) texCoord3 [[attribute(TexCoord3)]];
#endif
};

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

};

struct ObjectVariables
{
	float4x4 worldTransform;
};

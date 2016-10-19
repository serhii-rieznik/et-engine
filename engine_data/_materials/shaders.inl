#define VertexStreamBufferIndex         0
#define ObjectVariablesBufferIndex      4
#define MaterialVariablesBufferIndex    5
#define PassVariablesBufferIndex        6

#define PositionAttribute               0
#define NormalAttribute                 1
#define Color                           2
#define Tangent                         3
#define Binormal                        4

#define TexCoord0                       5
#define TexCoord1                       6
#define TexCoord2                       7
#define TexCoord3                       8

#define PI                              3.1415926536
#define HALF_PI                         1.5707963268
#define INV_PI                          0.3183098862

#define FLOAT_IMPL(N)                   float##N
#define FLOAT(N)                        FLOAT_IMPL(N)

#define PACKED_FLOAT_IMPL(N)            packed_float##N
#define PACKED_FLOAT(N)                 PACKED_FLOAT_IMPL(N)

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

#if (DIFFUSE_LAMBERT || DIFFUSE_BURLEY)
#	define USING_LIGHT_POSITION 1
#endif

#if (DIFFUSE_BURLEY)
#	define USING_CAMERA_POSITION 1
#endif

#if (DIFFUSE_BURLEY)
#	define USING_ROUGHNESS 1
#endif

#if (TEXCOORD0_SIZE > 0)
#	define EXPECT_TEXCOORD0
#else
#	define EXPECT_TEXCOORD0 [[ Input layout should have texCoord0 ]]
#endif

#if (NORMAL_SIZE > 0)
#	define EXPECT_NORMAL
#else
#	define EXPECT_NORMAL [[ Input layout should have normal ]]
#endif

#include "lighting.inl"

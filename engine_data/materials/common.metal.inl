#define VertexStreamBufferIndex         0
#define ObjectVariablesBufferIndex      4
#define MaterialVariablesBufferIndex    5
#define PassVariablesBufferIndex        6

#define PI                              3.1415926536
#define HALF_PI                         1.5707963268
#define INV_PI                          0.3183098862

using namespace metal;

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

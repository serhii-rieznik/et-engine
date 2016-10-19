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

uniform sampler2D texture_color;
uniform vec2 texel;

etFragmentIn vec2 TexCoord;
etFragmentIn vec2 TexCoordNW;
etFragmentIn vec2 TexCoordNE;
etFragmentIn vec2 TexCoordSW;
etFragmentIn vec2 TexCoordSE;

#define FXAA_REDUCE_MUL		(1.0/32.0)
#define FXAA_REDUCE_MIN		(1.0/128.0)
#define DIR_SCALE			(1.0/6.0)

const vec2 FXAA_SPAN_MAX = vec2(8.0);

const vec4 luma = vec4(0.299, 0.587, 0.114, 0.0);

void main()
{
	float lumaM = dot(etTexture2D(texture_color, TexCoord), luma);
	float lumaNW = dot(etTexture2D(texture_color, TexCoordNW), luma);
	float lumaNE = dot(etTexture2D(texture_color, TexCoordNE), luma);
	float lumaSW = dot(etTexture2D(texture_color, TexCoordSW), luma);
	float lumaSE = dot(etTexture2D(texture_color, TexCoordSE), luma);
	
	vec2 dir = vec2((lumaSW + lumaSE) - (lumaNW + lumaNE), (lumaNW + lumaSW) - (lumaNE + lumaSE));
	
	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * FXAA_REDUCE_MUL, FXAA_REDUCE_MIN);
	float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
	
	dir = texel * min(FXAA_SPAN_MAX, max(-FXAA_SPAN_MAX, dir * rcpDirMin));
	
	vec4 rgbA = 0.5 * (etTexture2D(texture_color, TexCoord - DIR_SCALE * dir) +
		etTexture2D(texture_color, TexCoord + DIR_SCALE * dir));
	
	vec4 rgbB = 0.5 * rgbA + 0.25 * (etTexture2D(texture_color, TexCoord - 0.5 * dir) +
		etTexture2D(texture_color, TexCoord + 0.5 * dir));
	
	float lumaB = dot(rgbB, luma);
	
	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
	
	etFragmentOut = ((lumaB < lumaMin) || (lumaB > lumaMax)) ? rgbA : rgbB;
}

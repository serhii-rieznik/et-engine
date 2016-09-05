#if defined(VERTEX_SHADER)

etVertexIn vec2 Vertex;
etVertexOut vec2 TexCoord;

void main()
{
	TexCoord = 0.5 * Vertex + vec2(0.5);
	gl_Position = vec4(Vertex, 0.0, 1.0);
}

#elif defined(FRAGMENT_SHADER)

#if defined(TEXTURE_CUBE)
uniform etLowp samplerCube color_texture;
#elif defined(TEXTURE_RECTANGLE)
uniform etLowp sampler2DRect color_texture;
uniform etHighp vec2 color_texture_size;
#elif defined(TEXTURE_2D_ARRAY)
uniform etLowp sampler2DArray color_texture;
#else
uniform etLowp sampler2D color_texture;
#endif

etFragmentIn etHighp vec2 TexCoord;

void main()
{
#if defined(TEXTURE_CUBE)
	etFragmentOut = etTextureCube(color_texture, vec3(TexCoord, 0.0));
#elif defined(TEXTURE_RECTANGLE)
	etFragmentOut = etTextureRect(color_texture, TexCoord * color_texture_size);
#elif defined(TEXTURE_2D_ARRAY)
	etFragmentOut = etTexture2DArray(color_texture, vec3(TexCoord, 0.0));
#else
	etFragmentOut = etTexture2D(color_texture, TexCoord);
#endif
}

#endif

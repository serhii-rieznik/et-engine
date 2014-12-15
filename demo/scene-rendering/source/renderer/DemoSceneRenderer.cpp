//
//  DemoSceneRenderer.cpp
//  SceneRendering
//
//  Created by Sergey Reznik on 14/12/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/rendering/rendercontext.h>
#include "DemoSceneRenderer.h"

using namespace et;
using namespace demo;

extern const std::string basicVertexShader;
extern const std::string basicFragmentShader;

enum
{
	diffuseTextureUnit,
	transparencyTextureUnit,
	normalTextureUnit,
};

void SceneRenderer::init(et::RenderContext* rc)
{
	_rc = rc;
	
	_defaultTexture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(1), GL_RGBA,
		GL_UNSIGNED_BYTE, BinaryDataStorage(4, 255), "white-texture");
	
	BinaryDataStorage normalData(4, 128);
	normalData[2] = 255;
	
	_defaultNormalTexture = _rc->textureFactory().genTexture(GL_TEXTURE_2D, GL_RGBA, vec2i(1), GL_RGBA,
		GL_UNSIGNED_BYTE, normalData, "normal-texture");
	
	programs.basic = _rc->programFactory().genProgram("basic-program", basicVertexShader, basicFragmentShader);
	programs.basic->setUniform("texture_diffuse", diffuseTextureUnit);
	programs.basic->setUniform("texture_mask", transparencyTextureUnit);
	programs.basic->setUniform("texture_normal", normalTextureUnit);
}

void SceneRenderer::setScene(et::s3d::Scene::Pointer aScene)
{
	_scene = aScene;
	_allObjects = _scene->childrenOfType(s3d::ElementType_Mesh);
	log::info("Scene set. %llu objects to render", static_cast<uint64_t>(_allObjects.size()));
}

void SceneRenderer::render(const et::Camera& cam)
{
	auto& rs = _rc->renderState();
	auto rn = _rc->renderer();
	
	rs.setDepthMask(true);
	rs.setDepthTest(true);

	rs.bindProgram(programs.basic);
	
	programs.basic->setCameraProperties(cam);
	
	for (s3d::Mesh::Pointer e : _allObjects)
	{
		const auto& mat = e->material();
		
		rs.bindVertexArray(e->vertexArrayObject());
		
		if (mat->hasTexture(MaterialParameter_DiffuseMap))
			rs.bindTexture(diffuseTextureUnit, mat->getTexture(MaterialParameter_DiffuseMap));
		else
			rs.bindTexture(diffuseTextureUnit, _defaultTexture);
		
		if (mat->hasTexture(MaterialParameter_TransparencyMap))
			rs.bindTexture(transparencyTextureUnit, mat->getTexture(MaterialParameter_TransparencyMap));
		else
			rs.bindTexture(transparencyTextureUnit, _defaultTexture);

		if (mat->hasTexture(MaterialParameter_NormalMap))
			rs.bindTexture(normalTextureUnit, mat->getTexture(MaterialParameter_NormalMap));
		else
			rs.bindTexture(normalTextureUnit, _defaultNormalTexture);
		
		rn->drawElements(e->indexBuffer(), e->startIndex(), e->numIndexes());
	}
}

/*
 * Shaders
 */
const std::string basicVertexShader = R"(

uniform mat4 mModelViewProjection;

etVertexIn vec3 Vertex;
etVertexIn vec2 TexCoord0;
etVertexIn vec3 Normal;

etVertexOut vec2 TexCoord;
etVertexOut vec3 vNormalWS;

void main()
{
	vNormalWS = Normal;
	TexCoord = TexCoord0;
	gl_Position = mModelViewProjection * vec4(Vertex, 1.0);
}

)";

const std::string basicFragmentShader = R"(

uniform sampler2D texture_diffuse;
uniform sampler2D texture_mask;
uniform sampler2D texture_normal;

etFragmentIn vec2 TexCoord;
etFragmentIn vec3 vNormalWS;

void main()
{
	vec4 maskSample = etTexture2D(texture_mask, TexCoord);
	if (maskSample.x < 0.5) discard;
	
	vec4 normalSample = etTexture2D(texture_normal, TexCoord);
	
	etFragmentOut = etTexture2D(texture_diffuse, TexCoord);
}

)";


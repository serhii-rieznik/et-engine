//
//  MainController.cpp
//  SceneRendering
//
//  Created by Sergey Reznik on 13/12/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#include <et/primitives/primitives.h>
#include "MainController.h"

using namespace et;
using namespace demo;

void MainController::setApplicationParameters(et::ApplicationParameters& p)
{
	p.windowStyle = WindowStyle_Sizable | WindowStyle_Caption;
}

void MainController::setRenderContextParameters(et::RenderContextParameters& params)
{
	params.contextSize = vec2i(1024, 768);
	params.contextBaseSize = params.contextSize;
	params.multisamplingQuality = MultisamplingQuality_Best;
}

void MainController::applicationDidLoad(et::RenderContext* rc)
{
	_camera.lookAt(vec3(20.0f));

	rc->renderState().setDepthTest(true);
	rc->renderState().setDepthMask(true);

	createModels(rc);
	createTextures(rc);
	createPrograms(rc); 
	
	rc->renderingInfoUpdated.connect([this](const et::RenderingInfo& info)
	{
		log::info("Rendering stats: %lu fps, %lu polys, %lu draw calls", info.averageFramePerSecond,
			info.averagePolygonsPerSecond, info.averageDIPPerSecond);
	});

	_frameTimeTimer.run();
}

void MainController::createModels(et::RenderContext* rc)
{
	VertexDeclaration decl(true);
	decl.push_back(VertexAttributeUsage::Position, VertexAttributeType::Vec3);
	decl.push_back(VertexAttributeUsage::Normal, VertexAttributeType::Vec3);

	VertexArray::Pointer vertices = VertexArray::Pointer::create(decl, 0);
	primitives::createIcosahedron(vertices, 5.0f);

	IndexArray::Pointer indices = IndexArray::Pointer::create(IndexArrayFormat::Format_16bit, 
		vertices->size(), PrimitiveType::Triangles);
	indices->linearize(vertices->size());

	_testModel = rc->vertexBufferFactory().createVertexArrayObject("test-model",
		vertices, BufferDrawType::Static, indices, BufferDrawType::Static);

	_transformMatrix = identityMatrix;
}

void MainController::createTextures(et::RenderContext* rc)
{
	vec2i maxSize = rc->sizei();

	auto allScreens = availableScreens();
	for (const auto& screen : allScreens)
		maxSize = maxv(screen.frame.size(), maxSize);

	_noiseTexture = rc->textureFactory().genNoiseTexture(maxSize, false, "noise-texture");
}

void MainController::createPrograms(et::RenderContext* rc)
{
	const std::string vertexShader =
	R"(
		uniform mat4 mModelViewProjection;
		uniform mat4 mTransform;
		etVertexIn vec3 Vertex;
		etVertexIn vec3 Normal;
		etVertexOut vec3 vNormalWS;
		void main()
		{
			vNormalWS = normalize(mat3(mTransform) * Normal);
			vec4 vVertexWS = mTransform * vec4(Vertex, 1.0);
			gl_Position = mModelViewProjection * vVertexWS;
		}
	)";

	const std::string fragmentShader =
		R"(
		etFragmentIn vec3 vNormalWS;
		const vec3 lightDirection = vec3(0.0, 1.0, 0.0);
		void main()
		{
			float LdotN = max(0.0, dot(vNormalWS, lightDirection));
			etFragmentOut = vec4((0.5 + 0.5 * vNormalWS) * LdotN, 1.0);
		}
	)";

	_defaultProgram = rc->programFactory().genProgram("default-progam", vertexShader, fragmentShader);
}

 void MainController::applicationWillResizeContext(const et::vec2i& sz)
{
	vec2 floatSize = vector2ToFloat(sz);
	_camera.perspectiveProjection(QUARTER_PI, floatSize.aspect(), 1.0f, 100.0f);
}

void MainController::render(et::RenderContext* rc)
{
	float deltaTime = _frameTimeTimer.lap();
	_transformMatrix *= rotationYXZMatrix(vec3(2.0f, 0.5f, -1.0f) * deltaTime);

	auto ren = rc->renderer();
	auto& rs = rc->renderState();

	ren->renderTexture(_noiseTexture, (rc->sizei() - _noiseTexture->size()) / 2);
	ren->clear(false, true);
	
	rs.bindVertexArray(_testModel);
	rs.bindTexture(0, _noiseTexture);
	rs.bindProgram(_defaultProgram);
	_defaultProgram->setCameraProperties(_camera);
	_defaultProgram->setTransformMatrix(_transformMatrix);

	ren->drawAllElements(_testModel->indexBuffer());
}

et::IApplicationDelegate* et::Application::initApplicationDelegate()
	{ return sharedObjectFactory().createObject<MainController>(); }

et::ApplicationIdentifier MainController::applicationIdentifier() const
	{ return ApplicationIdentifier("com.cheetek.et.getstarted", "Cheetek", "Get Started Demo"); }

//
//  DemoSceneRenderer.h
//  SceneRendering
//
//  Created by Sergey Reznik on 14/12/2014.
//  Copyright (c) 2014 Cheetek. All rights reserved.
//

#pragma once

#include <et/scene3d/scene3d.h>

namespace demo
{
	class SceneRenderer
	{
	public:
		void init(et::RenderContext*);
		void render(const et::Camera&, const et::Camera&, bool);
		
		void setScene(et::s3d::Scene::Pointer);
		
		void handlePressedKey(size_t);
		
	private:
		void renderToGeometryBuffer(const et::Camera&);
		void computeAmbientOcclusion(const et::Camera&);
		
	private:
		struct
		{
			et::Program::Pointer prepass;
			et::Program::Pointer ambientOcclusion;
			et::Program::Pointer ambientOcclusionBlur;
			et::Program::Pointer fxaa;
			et::Program::Pointer final;
			et::Program::Pointer motionBlur;
		} programs;
		
	private:
		et::RenderContext* _rc = nullptr;
		et::Framebuffer::Pointer _geometryBuffer;
		et::Framebuffer::Pointer _downsampledBuffer;
		et::Framebuffer::Pointer _finalBuffers[2];
		
		et::s3d::Scene::Pointer _scene;
		std::vector<et::s3d::SupportMesh::Pointer> _allObjects;
		std::vector<et::vec3> _lightPositions;
		
		et::Texture _noiseTexture;
		et::Texture _defaultTexture;
		et::Texture _defaultNormalTexture;
		
		et::VertexArrayObject _cubeMesh;
		et::VertexArrayObject _planeMesh;
		
		et::mat4 _previousProjectionMatrix;
		size_t _finalBufferIndex = 0;
		float _updateTime = 0.0f;
		bool _shouldSetPreviousProjectionMatrix = true;
	};
}

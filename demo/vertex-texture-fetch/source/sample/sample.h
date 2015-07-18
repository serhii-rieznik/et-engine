#pragma once

#include <et/camera/camera.h>
#include <et/rendering/rendercontext.h>
#include <et/timers/inertialvalue.h>

namespace demo
{
	class Sample : public et::EventReceiver
	{
	public:
		void prepare(et::RenderContext*);
		void render(et::RenderContext*);

		void stopCamera();
		void dragCamera(const et::vec2&);
		void panCamera(const et::vec2&);
		void zoom(float);
		
		void setModelToDraw(const et::VertexArrayObject&);

		void toggleObserving();
		void toggleWireframe();

	private:
		void loadPrograms(et::RenderContext*);
		void initCamera(et::RenderContext*);
		void createGeometry(et::RenderContext*);

		void createFrustumGeometry(et::RenderContext*);

		void updateCamera();
		void updateProjectorMatrix();
		void updateFrustumGeometry(bool projector);

	private:
		typedef et::StaticDataStorage<et::vec4, 4> PlanePoints;

	private:
		et::ObjectsCache _cache;

		et::VertexArrayObject _vao;
		et::VertexArrayObject _frustumGeometry;
		et::VertexArrayObject _model;

		et::Camera _camera;
		et::Camera _observingCamera;
		et::Camera _projectorCamera;

		et::Program::Pointer _program;
		et::Program::Pointer _frustumProgram;
		et::Program::Pointer _modelProgram;
		et::Texture::Pointer _texture;

		et::InertialValue<et::vec2> _cameraAngles;
		et::InertialValue<et::vec3> _cameraPosition;

		et::StaticDataStorage<et::vec3, 8> _frustumLines;
		et::VertexArray::Pointer _frustumLinesData;

		et::mat4 _projectorMatrix;
		float _contextAspect;
		bool _shouldRenderGrid;
		bool _observing;
		bool _wireframe;
		bool _belowSurface;
	};
}
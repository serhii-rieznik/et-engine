#include <et/app/application.h>
#include <et/gui/gui.h>
#include <et/scene3d/scene3d.h>
#include <et/timers/interpolationvalue.h>
#include <et/input/gestures.h>

namespace fbxc
{
	class MainLayout : public et::gui::Layout 
	{
		void layout(const et::vec2&) { }
	};

	class Converter : public et::IApplicationDelegate
	{
	public:
		Converter();

	private:
		et::ApplicationIdentifier applicationIdentifier() const;

		void setRenderContextParameters(et::RenderContextParameters&);
		void applicationDidLoad(et::RenderContext*);
		void render(et::RenderContext*);

	private:

		void onPointerPressed(et::PointerInputInfo);
		void onPointerMoved(et::PointerInputInfo);
		void onPointerReleased(et::PointerInputInfo);
		void onZoom(float);
		void onDrag(et::vec2, et::PointerType);
		void onScroll(et::vec2, et::PointerOrigin);
		void onCameraUpdated();

		void onBtnOpenClick(et::gui::Button*); 
		void onBtnSaveClick(et::gui::Button*); 
		void performLoading(std::string);
		void performBinarySaving(std::string);
		void performBinaryWithReadableMaterialsSaving(std::string);

		void renderMeshList(et::RenderContext* rc, const et::s3d::Element::List& meshes);
		void performSceneRendering();

	private:
		et::RenderContext* _rc;
		et::ObjectsCache _texCache;
		et::GesturesRecognizer _gestures;
		et::AutoPtr<et::gui::Gui> _gui;
		et::s3d::Scene _scene;
		et::Program::Pointer _defaultProgram;
		et::Camera _camera;

		et::IntrusivePtr<MainLayout> _mainLayout;

		et::gui::Font _mainFont;
		et::gui::Label::Pointer _labStatus;
		et::gui::Button::Pointer _btnDrawNormalMeshes;
		et::gui::Button::Pointer _btnDrawSupportMeshes;
		et::gui::Button::Pointer _btnWireframe;

		et::InterpolationValue<float> _vDistance;
		et::InterpolationValue<et::vec2> _vAngle;
	};
}
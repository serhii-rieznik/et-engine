#pragma once

#include <et/apiobjects/texture.h>

namespace et
{
	namespace gui
	{
		class GuiRenderer;

		enum ElementRepresentation
		{
			ElementRepresentation_2d,
			ElementRepresentation_3d,
			ElementRepresentation_max
		};

		template <typename T>
		struct ElementClass
		{
			static const std::string className;
			static AtomicCounter instanceConter;
			static std::string uniqueName(const std::string&);
		};

#		define ET_DECLARE_GUI_ELEMENT_CLASS(CLASS) template <> \
			const std::string et::gui::ElementClass<et::gui::CLASS*>::className = std::string(#CLASS);\
			template<>AtomicCounter et::gui::ElementClass<et::gui::CLASS*>::instanceConter = { };\
			template<>std::string et::gui::ElementClass<et::gui::CLASS*>::uniqueName(const std::string& inputName)\
			{ return (inputName.empty()) ? className + intToStr(instanceConter.retain()) : inputName; }

#		define ET_GUI_PASS_NAME_TO_BASE_CLASS ElementClass<decltype(this)>::uniqueName(name)

		struct AnimationDescriptor
		{
		public:
			size_t flags;
			float duration;

		public:
			AnimationDescriptor() :
				flags(AnimationFlag_None), duration(0.0f) { }

			AnimationDescriptor(size_t aFlags, float aDuration) :
				flags(aFlags), duration(aDuration) { }
		};

		struct ContentOffset
		{
		public:
			float left;
			float top;
			float right;
			float bottom;

		public:
			ContentOffset(float value = 0.0f) :
				left(value), top(value), right(value), bottom(value) { }

			ContentOffset(const vec2& values) :
				left(values.x), top(values.y), right(values.x), bottom(values.y) { }

			ContentOffset(float l, float t, float r, float b) :	
				left(l), top(t), right(r), bottom(b) { }

			const vec2 origin() const
				{ return vec2(left, top); }
		};

		struct ImageDescriptor
		{
			vec2 origin;
			vec2 size;
			ContentOffset contentOffset;

			ImageDescriptor() : 
				origin(0.0f), size(0.0f) { } 

			ImageDescriptor(const Texture& tex) :
				origin(0.0f), size(tex.valid() ? tex->sizeFloat() : vec2(0.0f)) { }

			ImageDescriptor(const Texture& tex, const ContentOffset& offset) : 
				origin(0.0f), size(tex.valid() ? tex->sizeFloat() : vec2(0.0f)), contentOffset(offset) { }

			ImageDescriptor(const vec2& aOrigin, const vec2& aSize, const ContentOffset& offset = ContentOffset()) : 
				origin(aOrigin), size(aSize), contentOffset(offset) { }

			vec2 centerPartTopLeft() const 
				{ return origin + contentOffset.origin(); }

			vec2 centerPartTopRight() const 
				{ return origin + vec2(size.x - contentOffset.right, contentOffset.top); }

			vec2 centerPartBottomLeft() const 
				{ return origin + vec2(contentOffset.left, size.y - contentOffset.bottom); }

			vec2 centerPartBottomRight() const 
				{ return origin + size - vec2(contentOffset.right, contentOffset.bottom); }

			rect rectangle() const
				{ return rect(origin, size); }
		};

		struct Image
		{
			Texture texture;
			ImageDescriptor descriptor;

			Image()
				{ }

			Image(const Texture& t) :
				texture(t), descriptor(ImageDescriptor(t)) { }

			Image(const Texture& t, const ImageDescriptor& d) :
				texture(t), descriptor(d) { }
		};

		struct GuiVertex
		{
		public:
			GuiVertex() 
				{ }

			GuiVertex(const vec2& pos, const vec4& tc, const vec4& c = vec4(1.0f)) : 
				position(pos, 0.0f), texCoord(tc), color(c) { }

			GuiVertex(const vec3& pos, const vec4& tc, const vec4& c = vec4(1.0f)) : 
				position(pos), texCoord(tc), color(c) { }

		public:
			vec3 position;
			vec4 texCoord;
			vec4 color; 
		};

		struct RenderChunk
		{
			size_t first;
			size_t count;
			recti clip;
			Texture layers[RenderLayer_max];
			ElementRepresentation representation;

			RenderChunk(size_t f, size_t cnt, const Texture& l0, const Texture& l1,
				const recti& aClip, ElementRepresentation c) : first(f), count(cnt), clip(aClip),
				representation(c)
			{ 
				layers[RenderLayer_Layer0] = l0;
				layers[RenderLayer_Layer1] = l1;
			}
		};

		struct ElementDragInfo
		{
			vec2 currentPosition;
			vec2 initialPosition;
			vec2 normalizedPointerPosition;
			
			ElementDragInfo(const vec2& c, const vec2& i, const vec2& npp) : 
				currentPosition(c), initialPosition(i), normalizedPointerPosition(npp) { }
		};

		struct GuiMessage
		{
			enum Type
			{
				Type_None = 0x0000,
				Type_TextInput = 0x0001,

				Type_User = 0xFFFF
			};

			union Parameter
			{
				size_t szValue;
				unsigned int uintValue;
				int intValue;
				unsigned short ushortValues[2];
				short shortValues[2];
				unsigned char ucharValues[4];
				char charValues[4];
			};

			size_t type;
			Parameter param;
			Parameter data;

			GuiMessage(size_t t, size_t p1, size_t p2 = 0) : type(t)
			{ 
				param.szValue = p1;
				data.szValue = p2;
			}
		};

		struct ElementLayout
		{
			vec2 position;
			vec2 size;
			vec2 scale;
			vec2 pivotPoint;
			float angle;
			size_t mask;

			LayoutMode positionMode;
			LayoutMode sizeMode;

			ElementLayout() : scale(1.0f), angle(0.0f), mask(LayoutMask_All),
				positionMode(LayoutMode_Absolute), sizeMode(LayoutMode_Absolute) { }

			ElementLayout(const vec2& pos, const vec2& sz, LayoutMode pMode,
				LayoutMode sMode) : position(pos), size(sz), scale(1.0f), angle(0.0f),
				mask(LayoutMask_All), positionMode(pMode), sizeMode(sMode) { }
		};


		extern const recti Clip_None;

		typedef GuiVertex* GuiVertexPointer;
		typedef DataStorage<GuiVertex> GuiVertexList;
		typedef std::vector<Image> ImageList;
		typedef std::map<std::string, Image> ImageMap;
		typedef std::list<RenderChunk> RenderChunkList;
		typedef Animator<vec2> Vector2Animator;
		typedef Animator<vec3> Vector3Animator;
		typedef Animator<vec4> Vector4Animator;
		typedef Animator<mat4> MatrixAnimator;
		typedef Animator<rect> RectAnimator;
	}
}

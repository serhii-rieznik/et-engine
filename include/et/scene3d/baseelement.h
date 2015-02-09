/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <set>
#include <typeinfo>
#include <et/core/et.h>
#include <et/core/flags.h>
#include <et/core/transformable.h>
#include <et/core/serialization.h>
#include <et/timers/notifytimer.h>
#include <et/scene3d/material.h>
#include <et/scene3d/serialization.h>
#include <et/scene3d/animation.h>

namespace et
{
	namespace s3d
	{
		class BaseElement;
		class ElementFactory;
		class Storage;

		enum Flags : uint32_t
		{
			Flag_Renderable = 0x0001,
			Flag_HasAnimations = 0x0002
		};

		enum ElementType : uint32_t
		{
			ElementType_Storage,
			ElementType_Container,
			ElementType_Mesh,
			ElementType_SupportMesh,
			ElementType_Camera,
			ElementType_Light,
			ElementType_ParticleSystem,
			ElementType_Any = 0xffffffff
		};

		typedef Hierarchy<BaseElement, LoadableObject> ElementHierarchy;
		class BaseElement : public ElementHierarchy, public FlagsHolder, public ComponentTransformable
		{
		public:
			ET_DECLARE_POINTER(BaseElement)

			typedef std::vector<BaseElement::Pointer> List;

		public:
			size_t tag = 0;

		public:
			virtual ElementType type() const = 0;
			virtual BaseElement* duplicate() = 0;

		public:
			BaseElement(const std::string& name, BaseElement* parent);
			
			void animate();
			
			Animation& defaultAnimation();
			const Animation& defaultAnimation() const;

			bool isKindOf(ElementType t) const;

			const mat4& finalTransform();
			const mat4& finalInverseTransform();
			
			const mat4& localTransform();
			
			void invalidateTransform();
			virtual void transformInvalidated() { }

			void setParent(BaseElement* p);

			Pointer childWithName(const std::string& name, ElementType ofType = ElementType_Any,
				bool assertFail = false);
			
			BaseElement::List childrenOfType(ElementType ofType) const;
			BaseElement::List childrenHavingFlag(size_t flag);

			virtual void serialize(Dictionary, const std::string&);
			virtual void deserialize(Dictionary, ElementFactory* factory);
			
			void clear();
			void clearRecursively();

			const std::set<std::string>& properties() const
				{ return _properites; }

			std::set<std::string>& properties()
				{ return _properites; }

			void addPropertyString(const std::string& s)
				{ _properites.insert(s); }
			
			bool hasPropertyString(const std::string& s) const;
			
			void addAnimation(const Animation&);
			
			void removeAnimations();
			
		protected:
			void serializeGeneralParameters(Dictionary);
			void serializeChildren(Dictionary, const std::string&);

			void deserializeGeneralParameters(Dictionary);
			void deserializeChildren(Dictionary, ElementFactory* factory);

			void duplicateChildrenToObject(BaseElement* object);
			void duplicateBasePropertiesToObject(BaseElement* object);
			
		private:
			Pointer childWithNameCallback(const std::string& name, Pointer root, ElementType ofType);
			void childrenOfTypeCallback(ElementType t, BaseElement::List& list, Pointer root) const;
			void childrenHavingFlagCallback(size_t flag, BaseElement::List& list, Pointer root);
			
			void buildTransform();

		private:
			Animation _emptyAnimation;
			NotifyTimer _animationTimer;
			
			std::set<std::string> _properites;
			std::vector<Animation> _animations;
			
			mat4 _animationTransform;
			mat4 _cachedLocalTransform;
			mat4 _cachedFinalTransform;
			mat4 _cachedFinalInverseTransform;
		};

		class ElementContainer : public BaseElement
		{
		public:
			ET_DECLARE_POINTER(ElementContainer)
			
		public:
			ElementContainer(const std::string& name, BaseElement* parent) :
				BaseElement(name, parent) { }

			ElementType type() const 
				{ return ElementType_Container; }

			ElementContainer* duplicate()
			{
				ElementContainer* result = sharedObjectFactory().createObject<ElementContainer>(name(), parent());
				duplicateChildrenToObject(result);
				result->tag = tag;
				return result; 
			}
		};

		class RenderableElement : public ElementContainer
		{
		public:
			ET_DECLARE_POINTER(RenderableElement)
			
		public:
			RenderableElement(const std::string& name, BaseElement* parent) :
				ElementContainer(name, parent) {
				setFlag(Flag_Renderable);
			}

			Material::Pointer& material()
				{ return _material; }

			const Material::Pointer& material() const
				{ return _material; }

			void setMaterial(const Material::Pointer& material)
				{ _material = material; }

			bool visible() const
				{ return _visible; }
			
			void setVisible(bool visible)
				{ _visible = visible; }

			void serialize(Dictionary, const std::string&);
			void deserialize(Dictionary, ElementFactory*);
			
		private:
			Material::Pointer _material;
			bool _visible = true;
		};

		class ElementFactory
		{
		public:
			virtual Material::Pointer materialWithName(const std::string&) = 0;

			virtual BaseElement::Pointer createElementOfType(uint64_t, BaseElement*) = 0;
			
			virtual VertexStorage::Pointer vertexStorageWithName(const std::string&) = 0;

			virtual IndexArray::Pointer primaryIndexArray() = 0;
			
			virtual ~ElementFactory() { }
		};

	}
}

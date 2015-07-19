/*
 * This file is part of `et engine`
 * Copyright 2009-2015 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <set>
#include <et/core/flags.h>
#include <et/core/transformable.h>
#include <et/core/serialization.h>
#include <et/timers/notifytimer.h>
#include <et/scene3d/base.h>
#include <et/scene3d/serialization.h>
#include <et/scene3d/animation.h>

namespace et
{
	namespace s3d
	{
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
			void animateRecursive();
			
			Animation& defaultAnimation();
			const Animation& defaultAnimation() const;

			bool isKindOf(ElementType t) const;

			const mat4& finalTransform();
			const mat4& finalInverseTransform();
			
			const mat4& localTransform();
			
			void invalidateTransform();

			void setParent(BaseElement* p);

			Pointer childWithName(const std::string& name, ElementType ofType = ElementType::DontCare,
				bool assertFail = false);
			
			BaseElement::List childrenOfType(ElementType ofType) const;
			BaseElement::List childrenHavingFlag(size_t flag);

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
			virtual void serialize(Dictionary, const std::string&);
			virtual void deserialize(Dictionary, SerializationHelper*);
			virtual void transformInvalidated() { }

			void serializeGeneralParameters(Dictionary);
			void serializeChildren(Dictionary, const std::string&);

			void deserializeGeneralParameters(Dictionary);
			void deserializeChildren(Dictionary, SerializationHelper*);

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
			
			mat4 _animationTransform = mat4(1.0f);
			mat4 _cachedLocalTransform = mat4(1.0f);
			mat4 _cachedFinalTransform = mat4(1.0f);
			mat4 _cachedFinalInverseTransform = mat4(1.0f);
		};
	}
}

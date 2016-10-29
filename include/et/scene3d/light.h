/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/camera/camera.h>
#include <et/scene3d/scene3d.h>

namespace et
{
    namespace s3d
    {
        class Light : public BaseElement
        {
        public:
            ET_DECLARE_POINTER(Light);
            
        public:
            Light(BaseElement* parent = nullptr) :
                BaseElement("light", parent) { }
            
            ElementType type() const override
                { return ElementType::Light; };
            
            Camera::Pointer& camera()
                { return _camera; }
            
            const Camera::Pointer& camera() const
                { return _camera; }

            void setCamera(const Camera::Pointer& cam)
                { _camera = cam; }
            
            BaseElement* duplicate() override
                { ET_FAIL("TODO"); return nullptr; }
            
        private:
			Camera::Pointer _camera = Camera::Pointer::create();
        };
    }
}

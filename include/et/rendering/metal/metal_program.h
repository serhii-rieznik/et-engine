/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/interface/program.h>

namespace et
{
    struct MetalState;
    struct MetalNativeProgram;
	class MetalProgramPrivate;
	class MetalProgram : public Program
	{
	public:
		ET_DECLARE_POINTER(MetalProgram);
		
	public:
        MetalProgram(MetalState&);
		~MetalProgram();
        
        void bind() override;
        void build(const std::string& vertexSource, const std::string& fragmentSource) override;

        void setTransformMatrix(const mat4 &m, bool force) override;
        void setCameraProperties(const Camera& cam) override;
        void setDefaultLightPosition(const vec3& p, bool force) override;
        
        const MetalNativeProgram& nativeProgram() const;
        
	private:
		ET_DECLARE_PIMPL(MetalProgram, 32)
	};
}

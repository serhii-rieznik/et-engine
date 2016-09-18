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
    class Camera;
	class VulkanState;
	class VulkanShaderModules;
	class VulkanProgramPrivate;
    class VulkanProgram : public Program
    {
    public:
        ET_DECLARE_POINTER(VulkanProgram);

    public:
		VulkanProgram(VulkanState&);
		~VulkanProgram();

        void build(const std::string& source, const std::string& fragmentSource) override;

		const VulkanShaderModules& shaderModules() const;
        
        void setTransformMatrix(const mat4 &m, bool force) override;
		void setCameraProperties(const Camera& cam) override;
        void setDefaultLightPosition(const vec3& p, bool force) override;

	private:
		ET_DECLARE_PIMPL(VulkanProgram, 128);
    };
}

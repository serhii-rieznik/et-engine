/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/rendering/rendering.h>

namespace et
{
    class Camera;
    class Program : public LoadableObject
    {
    public:
        ET_DECLARE_POINTER(Program);
 
        struct ShaderConstant
        {
            uint32_t type = 0;
            int32_t location = -1;
        };
        typedef std::unordered_map<std::string, ShaderConstant> ShaderConstantMap;
        
    public:
        Program() = default;
        
        Program(const std::string& name, const std::string& origin)
            : LoadableObject(name, origin) { }
        
        ShaderConstantMap::iterator findConstant(const std::string& name)
            { return _constants.find(name); }
        
        ShaderConstantMap::const_iterator findConstant(const std::string& name) const
            { return _constants.find(name); }
        
        ShaderConstantMap& shaderConstants()
            { return _constants; }
        
        const ShaderConstantMap& shaderConstants() const
            { return _constants; }

        void clearShaderConstants()
            { _constants.clear(); }
        
        const StringList& defines() const
            { return _defines; }
        
        void setDefines(const StringList& defines)
            { _defines = defines; }
        
        virtual void bind() = 0;
        virtual void build(const std::string& vertexSource, const std::string& fragmentSource) = 0;
        
        virtual void setTransformMatrix(const mat4 &m, bool force) = 0;
        virtual void setCameraProperties(const Camera& cam) = 0;
        virtual void setDefaultLightPosition(const vec3& p, bool force) = 0;
        
    private:
        StringList _defines;
        ShaderConstantMap _constants;
    };
}

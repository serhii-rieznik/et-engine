/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et-ext/rt/kdtree.h>
#include <et-ext/rt/environment.h>

namespace et
{
namespace rt
{
    class Integrator : public Object
    {
    public:
        ET_DECLARE_POINTER(Integrator)
        
    public:
        virtual ~Integrator() { }
        virtual rt::float4 gather(const rt::Ray& inRay, size_t depth, size_t& maxDepth,
            KDTree& tree, EnvironmentSampler::Pointer&, const rt::Material::Collection&) = 0;
    };
    
    class AmbientOcclusionIntegrator : public Integrator
    {
    public:
        ET_DECLARE_POINTER(AmbientOcclusionIntegrator)
        
    public:
        rt::float4 gather(const rt::Ray& inRay, size_t depth, size_t& maxDepth,
            KDTree& tree, EnvironmentSampler::Pointer&, const rt::Material::Collection&) override;
    };
    
    class PathTraceIntegrator : public Integrator
    {
    public:
        ET_DECLARE_POINTER(PathTraceIntegrator)
        
        enum
        {
            MaxTraverseDepth = 32
        };
        
    public:
        rt::float4 gather(const rt::Ray& inRay, size_t depth, size_t& maxDepth,
            KDTree& tree, EnvironmentSampler::Pointer&, const rt::Material::Collection&) override;

    private:
        void choseNewRayDirectionAndMaterial(rt::float4& normal, const rt::Material& mat, const rt::float4& inDirection,
            rt::float4& direction, rt::float4& output);
    };
}
}

/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#include <et-ext/rt/integrator.h>

namespace et
{
namespace rt
{
   
float4 AmbientOcclusionIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth,
    KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection&)
{
    KDTree::TraverseResult hit0 = tree.traverse(inRay);
    if (hit0.triangleIndex == InvalidIndex)
        return env->sampleInDirection(inRay.direction);
    
    const auto& tri = tree.triangleAtIndex(hit0.triangleIndex);
    float4 surfaceNormal = tri.interpolatedNormal(hit0.intersectionPointBarycentric);
    float4 nextDirection = randomVectorOnHemisphere(surfaceNormal, HALF_PI);
    float4 nextOrigin = hit0.intersectionPoint + nextDirection * Constants::epsilon;
    
    if (tree.traverse(Ray(nextOrigin, nextDirection)).triangleIndex == InvalidIndex)
        return env->sampleInDirection(nextDirection) * nextDirection.dot(surfaceNormal);
    
    return float4(0.0f);
}
    
/*
 * PathTraceIntegrator
 */
struct ET_ALIGNED(16) Bounce
{
    rt::float4 add;
    rt::float4 mul;
    Bounce() = default;
    Bounce(const float4& a, const float4& m) :
        add(a), mul(m) { }
};
    
float4 PathTraceIntegrator::gather(const Ray& inRay, size_t depth, size_t& maxDepth,
    KDTree& tree, EnvironmentSampler::Pointer& env, const Material::Collection& materials)
{
    auto currentRay = inRay;
    float4 materialColor;
    
    FastStack<PathTraceIntegrator::MaxTraverseDepth, Bounce> bounces;
    while (bounces.size() < MaxTraverseDepth)
    {
        KDTree::TraverseResult traverse = tree.traverse(currentRay);
        if (traverse.triangleIndex == InvalidIndex)
        {
            bounces.emplace(env->sampleInDirection(currentRay.direction), float4(0.0f));
            break;
        }
        const auto& tri = tree.triangleAtIndex(traverse.triangleIndex);
        float4 clearN = tri.interpolatedNormal(traverse.intersectionPointBarycentric);

		const auto& mat = materials[tri.materialIndex];
        float4 roughN = randomVectorOnHemisphere(clearN, mat.roughness);
        float4 directionScale = clearN.dotVector(roughN);
        choseNewRayDirectionAndMaterial(roughN, mat, currentRay.direction, currentRay.direction, materialColor);
        bounces.emplace(mat.emissive, materialColor * directionScale);
        currentRay.origin = traverse.intersectionPoint + currentRay.direction * Constants::epsilon;
    }
    maxDepth = bounces.size();
    
    float4 result(0.0f);
    do
    {
        result *= bounces.top().mul;
        result += bounces.top().add;
        bounces.pop();
    }
    while (bounces.hasSomething());
    
    return result;
}

inline void computeReflectionVector(const float4& indidence, const float4& normal, float4& direction)
{
    direction = reflect(indidence, normal);
    if (direction.dot(normal) <= Constants::epsilon)
        direction = reflect(direction, normal);
}

inline void computeRefractionVector(const float4& incidence, const float4& normal, float k, float eta, float4& direction)
{
    direction = incidence * eta - normal * (eta * normal.dot(incidence) + std::sqrt(k));
    if (direction.dot(normal) >= Constants::minusEpsilon)
        direction = reflect(direction, normal);
}
    
void PathTraceIntegrator::choseNewRayDirectionAndMaterial(float4& normal, const Material& mat,
    const float4& inDirection, float4& direction, float4& output)
{
    if (mat.ior >= Constants::onePlusEpsilon)
    {
        float eta;
        if (normal.dot(inDirection) >= 0.0)
        {
            normal *= -1.0f;
            eta = mat.ior;
        }
        else
        {
            eta = 1.00001f / mat.ior;
        }
        
        float k = computeRefractiveCoefficient(inDirection, normal, eta);
        if (k >= Constants::epsilon) // refract
        {
            float fresnel = computeFresnelTerm(inDirection, normal, eta);
            if (fastRandomFloat() >= fresnel)
            {
                // refract
                output = mat.diffuse;
                computeRefractionVector(inDirection, normal, k, eta, direction);
            }
            else
            {
                output = mat.specular;
                computeReflectionVector(inDirection, normal, direction);
            }
        }
        else // reflect due to total internal reflection
        {
            output = mat.specular;
            computeReflectionVector(inDirection, normal, direction);
        }
    }
    else if (fastRandomFloat() >= mat.roughness)
    {
        // compute specular reflection
        output = mat.specular;
        computeReflectionVector(inDirection, normal, direction);
    }
    else
    {
        // compute diffuse reflection
        output = mat.diffuse;
        direction = normal;
    }
}

}
}
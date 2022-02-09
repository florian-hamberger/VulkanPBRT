#pragma once

#include <scene/RayTracingVisitor.hpp>

#include <vsg/all.h>
#include <vector>

class TerrainRayTracingSceneDescriptorCreationVisitor : public vsg::Inherit<RayTracingSceneDescriptorCreationVisitor, TerrainRayTracingSceneDescriptorCreationVisitor>
{
public:
    TerrainRayTracingSceneDescriptorCreationVisitor();

    void apply(vsg::BindDescriptorSet& bds) override;
};


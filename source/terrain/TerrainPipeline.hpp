#pragma once

#include <buffers/GBuffer.hpp>
#include <buffers/IlluminationBuffer.hpp>
#include <buffers/AccumulationBuffer.hpp>
#include <renderModules/PBRTPipeline.hpp>
#include <terrain/TerrainRayTracingVisitor.hpp>

#include <vsg/all.h>
#include <vsgXchange/glsl.h>

#include <cstdint>

class TerrainPipeline : public vsg::Inherit<PBRTPipeline, TerrainPipeline>
{
public:
    TerrainPipeline(vsg::ref_ptr<vsg::Node> scene, vsg::ref_ptr<GBuffer> gBuffer, vsg::ref_ptr<AccumulationBuffer> accumulationBuffer,
                 vsg::ref_ptr<IlluminationBuffer> illuminationBuffer, bool writeGBuffer, RayTracingRayOrigin rayTracingRayOrigin, uint32_t maxRecursionDepth);

    void updateTlas(vsg::ref_ptr<vsg::AccelerationStructure> as, vsg::ref_ptr<vsg::Context> context);
    void updateScene(vsg::ref_ptr<vsg::Node> scene, vsg::ref_ptr<vsg::Context> context);
protected:
    void setupPipeline(vsg::Node* scene, bool useExternalGBuffer);
};

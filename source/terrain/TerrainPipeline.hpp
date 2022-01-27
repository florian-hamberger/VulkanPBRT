#pragma once

#include <buffers/GBuffer.hpp>
#include <buffers/IlluminationBuffer.hpp>
#include <scene/RayTracingVisitor.hpp>
#include <buffers/AccumulationBuffer.hpp>
#include <renderModules/PBRTPipeline.hpp>

#include <vsg/all.h>
#include <vsgXchange/glsl.h>

#include <cstdint>

class TerrainPipeline : public vsg::Inherit<vsg::Object, TerrainPipeline>
{
public:
    TerrainPipeline(vsg::ref_ptr<vsg::Node> scene, vsg::ref_ptr<GBuffer> gBuffer, vsg::ref_ptr<AccumulationBuffer> accumulationBuffer,
                 vsg::ref_ptr<IlluminationBuffer> illuminationBuffer, bool writeGBuffer, RayTracingRayOrigin rayTracingRayOrigin, uint32_t maxRecursionDepth);

    void setTlas(vsg::ref_ptr<vsg::AccelerationStructure> as);
    void updateTlas(vsg::ref_ptr<vsg::AccelerationStructure> as, vsg::ref_ptr<vsg::Context> context);
    void updateScene(vsg::ref_ptr<vsg::Node> scene, vsg::ref_ptr<vsg::Context> context);
    void compile(vsg::Context& context);
    void updateImageLayouts(vsg::Context& context);
    void addTraceRaysToCommandGraph(vsg::ref_ptr<vsg::Commands> commandGraph, vsg::ref_ptr<vsg::PushConstants> pushConstants);
    vsg::ref_ptr<IlluminationBuffer> getIlluminationBuffer() const;
    enum class LightSamplingMethod{
        SampleSurfaceStrength,
        SampleLightStrength,
        SampleUniform
    }lightSamplingMethod = LightSamplingMethod::SampleSurfaceStrength;
private:
    void setupPipeline(vsg::Node* scene, bool useExternalGBuffer);
    vsg::ref_ptr<vsg::ShaderStage> setupRaygenShader(std::string raygenPath, bool useExternalGBuffer);

    std::vector<bool> opaqueGeometries;
    uint32_t width, height, maxRecursionDepth, samplePerPixel;

    // TODO: add buffers here
    vsg::ref_ptr<GBuffer> gBuffer;
    vsg::ref_ptr<AccumulationBuffer> accumulationBuffer;
    vsg::ref_ptr<IlluminationBuffer> illuminationBuffer;

    //resources which have to be added as childs to a scenegraph for rendering
    vsg::ref_ptr<vsg::BindRayTracingPipeline> bindRayTracingPipeline;
    vsg::ref_ptr<vsg::BindDescriptorSet> bindRayTracingDescriptorSet;
    vsg::ref_ptr<vsg::PushConstants> pushConstants;

    //shader binding table for trace rays
    vsg::ref_ptr<vsg::RayTracingShaderBindingTable> shaderBindingTable;

    //binding map containing all descriptor bindings in the shaders
    vsg::BindingMap bindingMap;
};

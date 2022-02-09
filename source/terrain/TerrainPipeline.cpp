#include "TerrainPipeline.hpp"

#include <cassert>

namespace
{
    struct ConstantInfos
    {
        uint32_t lightCount;
        float lightStrengthSum;
        uint32_t minRecursionDepth;
        uint32_t maxRecursionDepth;
    };

    class ConstantInfosValue : public vsg::Inherit<vsg::Value<ConstantInfos>, ConstantInfosValue>
    {
    public:
        ConstantInfosValue()
        {
        }
    };
}

TerrainPipeline::TerrainPipeline(vsg::ref_ptr<vsg::Node> scene, vsg::ref_ptr<GBuffer> gBuffer,
                 vsg::ref_ptr<IlluminationBuffer> illuminationBuffer, bool writeGBuffer, RayTracingRayOrigin rayTracingRayOrigin, uint32_t maxRecursionDepth) :
    Inherit(gBuffer, illuminationBuffer)
{
    this->maxRecursionDepth = maxRecursionDepth;

    if (writeGBuffer) assert(gBuffer);
    bool useExternalGBuffer = rayTracingRayOrigin == RayTracingRayOrigin::GBUFFER;
    setupPipeline(scene, useExternalGBuffer);
}

void TerrainPipeline::updateTlas(vsg::ref_ptr<vsg::AccelerationStructure> as, vsg::ref_ptr<vsg::Context> context)
{
    auto tlas = as.cast<vsg::TopLevelAccelerationStructure>();
    assert(tlas);
    for (int i = 0; i < tlas->geometryInstances.size(); ++i)
    {
        if (opaqueGeometries[i])
            tlas->geometryInstances[i]->shaderOffset = 0;
        else
            tlas->geometryInstances[i]->shaderOffset = 1;
        tlas->geometryInstances[i]->flags = VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
    }
    auto accelDescriptor = vsg::DescriptorAccelerationStructure::create(vsg::AccelerationStructures{ as }, 0, 0);

    //bindRayTracingDescriptorSet->descriptorSet->descriptors = vsg::Descriptors{ accelDescriptor };

    auto descriptorSet = bindRayTracingDescriptorSet->descriptorSet;
    int index = -1;
    for (int i = 0; i < descriptorSet->descriptors.size(); ++i) {
        if (descriptorSet->descriptors[i]->dstBinding == 0 && descriptorSet->descriptors[i]->dstArrayElement == 0 && descriptorSet->descriptors[i]->descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) {
            index = i;

            std::cout << "found descriptor to remove: " << index << std::endl;
            break;
        }
    }
    if (index >= 0) {
        std::cout << "removing descriptor: " << index << std::endl;
        descriptorSet->descriptors.erase(descriptorSet->descriptors.begin() + index);
        //descriptors[index] = accelDescriptor;
    }
    //bindRayTracingDescriptorSet->descriptorSet->descriptors.clear();

    bindRayTracingDescriptorSet->descriptorSet->descriptors.push_back(accelDescriptor);

    std::cout << "descriptors: " << bindRayTracingDescriptorSet->descriptorSet->descriptors.size() << std::endl;

    bindRayTracingDescriptorSet->descriptorSet->release();
    bindRayTracingDescriptorSet->descriptorSet->compile(*context);
}

void TerrainPipeline::updateScene(vsg::ref_ptr<vsg::Node> scene, vsg::ref_ptr<vsg::Context> context) {
    // parsing data from scene
    TerrainRayTracingSceneDescriptorCreationVisitor buildDescriptorBinding;
    scene->accept(buildDescriptorBinding);
    opaqueGeometries = buildDescriptorBinding.isOpaque;

    const int maxLights = 800;
    if (buildDescriptorBinding.packedLights.size() > maxLights) lightSamplingMethod = LightSamplingMethod::SampleUniform;

    std::cout << "descriptors: " << bindRayTracingDescriptorSet->descriptorSet->descriptors.size() << std::endl;

    bindRayTracingDescriptorSet->descriptorSet->descriptors.clear();

    buildDescriptorBinding.updateDescriptor(bindRayTracingDescriptorSet, bindingMap);

    std::cout << "descriptors: " << bindRayTracingDescriptorSet->descriptorSet->descriptors.size() << std::endl;

    //// creating the constant infos uniform buffer object
    auto constantInfos = ConstantInfosValue::create();
    constantInfos->value().lightCount = buildDescriptorBinding.packedLights.size();
    constantInfos->value().lightStrengthSum = buildDescriptorBinding.packedLights.back().inclusiveStrength;
    constantInfos->value().maxRecursionDepth = maxRecursionDepth;
    uint32_t uniformBufferBinding = vsg::ShaderStage::getSetBindingIndex(bindingMap, "Infos").second;
    auto constantInfosDescriptor = vsg::DescriptorBuffer::create(constantInfos, uniformBufferBinding, 0);
    bindRayTracingDescriptorSet->descriptorSet->descriptors.push_back(constantInfosDescriptor);

    // update the descriptor sets
    illuminationBuffer->updateDescriptor(bindRayTracingDescriptorSet, bindingMap);
    if (gBuffer)
        gBuffer->updateDescriptor(bindRayTracingDescriptorSet, bindingMap);

    std::cout << "descriptors: " << bindRayTracingDescriptorSet->descriptorSet->descriptors.size() << std::endl;

}

void TerrainPipeline::setupPipeline(vsg::Node *scene, bool useExternalGbuffer)
{
    // parsing data from scene
    TerrainRayTracingSceneDescriptorCreationVisitor buildDescriptorBinding;
    scene->accept(buildDescriptorBinding);
    opaqueGeometries = buildDescriptorBinding.isOpaque;

    const int maxLights = 800;
    if(buildDescriptorBinding.packedLights.size() > maxLights) lightSamplingMethod = LightSamplingMethod::SampleUniform;

    //creating the shader stages and shader binding table
    std::string raygenPath = "shaders/ptRaygen.rgen"; //raygen shader not yet precompiled
    std::string raymissPath = "shaders/ptMiss.rmiss.spv";
    std::string shadowMissPath = "shaders/shadow.rmiss.spv";
    std::string closesthitPath = "shaders/ptClosesthit.rchit.spv";
    std::string anyHitPath = "shaders/ptAlphaHit.rahit.spv";

    auto raygenShader = setupRaygenShader(raygenPath, useExternalGbuffer);
    auto raymissShader = vsg::ShaderStage::read(VK_SHADER_STAGE_MISS_BIT_KHR, "main", raymissPath);
    auto shadowMissShader = vsg::ShaderStage::read(VK_SHADER_STAGE_MISS_BIT_KHR, "main", shadowMissPath);
    auto closesthitShader = vsg::ShaderStage::read(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "main", closesthitPath);
    auto anyHitShader = vsg::ShaderStage::read(VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "main", anyHitPath);
    if (!raygenShader || !raymissShader || !closesthitShader || !shadowMissShader || !anyHitShader)
    {
        throw vsg::Exception{"Error: TerrainPipeline::TerrainPipeline(...) failed to create shader stages."};
    }
    bindingMap = vsg::ShaderStage::mergeBindingMaps(
        {raygenShader->getDescriptorSetLayoutBindingsMap(),
         raymissShader->getDescriptorSetLayoutBindingsMap(),
         shadowMissShader->getDescriptorSetLayoutBindingsMap(),
         closesthitShader->getDescriptorSetLayoutBindingsMap(),
         anyHitShader->getDescriptorSetLayoutBindingsMap()});

    auto descriptorSetLayout = vsg::DescriptorSetLayout::create(bindingMap.begin()->second.bindings);
    // auto rayTracingPipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{descriptorSetLayout}, vsg::PushConstantRanges{{VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(RayTracingPushConstants)}});
    auto rayTracingPipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{descriptorSetLayout},
                                                                raygenShader->getPushConstantRanges());
    auto shaderStage = vsg::ShaderStages{raygenShader, raymissShader, shadowMissShader, closesthitShader, anyHitShader};
    auto raygenShaderGroup = vsg::RayTracingShaderGroup::create();
    raygenShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raygenShaderGroup->generalShader = 0;
    auto raymissShaderGroup = vsg::RayTracingShaderGroup::create();
    raymissShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    raymissShaderGroup->generalShader = 1;
    auto shadowMissShaderGroup = vsg::RayTracingShaderGroup::create();
    shadowMissShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shadowMissShaderGroup->generalShader = 2;
    auto closesthitShaderGroup = vsg::RayTracingShaderGroup::create();
    closesthitShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    closesthitShaderGroup->closestHitShader = 3;
    auto transparenthitShaderGroup = vsg::RayTracingShaderGroup::create();
    transparenthitShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    transparenthitShaderGroup->closestHitShader = 3;
    transparenthitShaderGroup->anyHitShader = 4;
    auto shaderGroups = vsg::RayTracingShaderGroups{
        raygenShaderGroup, raymissShaderGroup, shadowMissShaderGroup, closesthitShaderGroup, transparenthitShaderGroup};
    shaderBindingTable = vsg::RayTracingShaderBindingTable::create();
    shaderBindingTable->bindingTableEntries.raygenGroups = {raygenShaderGroup};
    shaderBindingTable->bindingTableEntries.raymissGroups = {raymissShaderGroup, shadowMissShaderGroup};
    shaderBindingTable->bindingTableEntries.hitGroups = {closesthitShaderGroup, transparenthitShaderGroup};
    auto pipeline = vsg::RayTracingPipeline::create(rayTracingPipelineLayout, shaderStage, shaderGroups, shaderBindingTable, 1);
    bindRayTracingPipeline = vsg::BindRayTracingPipeline::create(pipeline);
    auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{});
    bindRayTracingDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipelineLayout, descriptorSet);

    buildDescriptorBinding.updateDescriptor(bindRayTracingDescriptorSet, bindingMap);
    // creating the constant infos uniform buffer object
    auto constantInfos = ConstantInfosValue::create();
    constantInfos->value().lightCount = buildDescriptorBinding.packedLights.size();
    constantInfos->value().lightStrengthSum = buildDescriptorBinding.packedLights.back().inclusiveStrength;
    constantInfos->value().maxRecursionDepth = maxRecursionDepth;
    uint32_t uniformBufferBinding = vsg::ShaderStage::getSetBindingIndex(bindingMap, "Infos").second;
    auto constantInfosDescriptor = vsg::DescriptorBuffer::create(constantInfos, uniformBufferBinding, 0);
    bindRayTracingDescriptorSet->descriptorSet->descriptors.push_back(constantInfosDescriptor);

    // update the descriptor sets
    illuminationBuffer->updateDescriptor(bindRayTracingDescriptorSet, bindingMap);
    if (gBuffer)
        gBuffer->updateDescriptor(bindRayTracingDescriptorSet, bindingMap);
}
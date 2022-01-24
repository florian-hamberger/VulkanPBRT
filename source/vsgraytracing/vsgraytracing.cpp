#include <iostream>
#include <vsg/all.h>

#ifdef vsgXchange_FOUND
#    include <vsgXchange/all.h>
#endif

struct RayTracingUniform
{
    vsg::mat4 viewInverse;
    vsg::mat4 projInverse;
};

class RayTracingUniformValue : public vsg::Inherit<vsg::Value<RayTracingUniform>, RayTracingUniformValue>
{
public:
    RayTracingUniformValue() {}
};

vsg::ref_ptr<vsg::CompileTraversal> compileNode(vsg::ref_ptr<vsg::Object> object, vsg::ref_ptr<vsg::AccelerationStructure> accelerationStructure, vsg::ref_ptr<vsg::Window> window, vsg::ref_ptr<vsg::ViewportState> viewport, bool useAccelerationStructure, vsg::ref_ptr<vsg::CompileTraversal> externalCompileTraversal, bool useExternalCompileTraversal, vsg::ref_ptr<vsg::DescriptorAccelerationStructure> descriptorAccelerationStructure, bool useDescriptorAccelerationStructure)
{
    if (useAccelerationStructure) {
        object = accelerationStructure;
    } else if (useDescriptorAccelerationStructure) {
        object = descriptorAccelerationStructure;
    }

    std::cout << "Compiling " << std::endl;

    vsg::ref_ptr<vsg::CompileTraversal> compileTraversal;
    if (useExternalCompileTraversal) {
        compileTraversal = externalCompileTraversal;
    } else {
        std::cout << "takeCompileTraversal() creating a new CompileTraversal" << std::endl;
        vsg::ResourceRequirements resourceRequirements;
        compileTraversal = vsg::CompileTraversal::create(window, viewport, resourceRequirements);

        vsg::CollectResourceRequirements collectRequirements;
        object->accept(collectRequirements);

        auto maxSets = collectRequirements.requirements.computeNumDescriptorSets();
        auto descriptorPoolSizes = collectRequirements.requirements.computeDescriptorPoolSizes();

        // brute force allocation of new DescrptorPool for this subgraph, TODO : need to preallocate large DescritorPoil for multiple loaded subgraphs
        if (descriptorPoolSizes.size() > 0) compileTraversal->context.descriptorPool = vsg::DescriptorPool::create(compileTraversal->context.device, maxSets, descriptorPoolSizes);
    }

    if (useAccelerationStructure) {
        accelerationStructure->compile(compileTraversal->context);
    }
    else if (useDescriptorAccelerationStructure) {
        descriptorAccelerationStructure->compile(compileTraversal->context);
    }
    else {
        object->accept(*compileTraversal);
    }

    std::cout << "Finished compile traversal " << std::endl;

    std::cout << std::to_string(compileTraversal->context.buildAccelerationStructureCommands.size()) << std::endl;

    compileTraversal->context.record(); // records and submits to queue

    compileTraversal->context.waitForCompletion();

    std::cout << "Finished waiting for compile " << std::endl;

    return compileTraversal;
}

int main(int argc, char** argv)
{
    try
    {
        // set up defaults and read command line arguments to override them
        vsg::CommandLine arguments(&argc, argv);

        auto options = vsg::Options::create();

        // set up search paths to SPIRV shaders and textures
        options->paths = vsg::getEnvPaths("VSG_FILE_PATH");

#ifdef vsgXchange_all
        // add vsgXchange's support for reading and writing 3rd party file formats
        options->add(vsgXchange::all::create());
#endif

        auto windowTraits = vsg::WindowTraits::create();
        windowTraits->windowTitle = "vsgraytracing";
        windowTraits->debugLayer = arguments.read({"--debug", "-d"});
        windowTraits->apiDumpLayer = arguments.read({"--api", "-a"});
        if (arguments.read({"--fullscreen", "--fs"})) windowTraits->fullscreen = true;
        if (arguments.read({"--window", "-w"}, windowTraits->width, windowTraits->height)) { windowTraits->fullscreen = false; }
        arguments.read("--screen", windowTraits->screenNum);
        auto numFrames = arguments.value(-1, "-f");

        vsg::Path filename;
        if (argc > 1) filename = arguments[1];

        if (arguments.errors()) return arguments.writeErrorMessages(std::cerr);

        // create the viewer and assign window(s) to it
        auto viewer = vsg::Viewer::create();

        windowTraits->queueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
        windowTraits->imageAvailableSemaphoreWaitFlag = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        windowTraits->swapchainPreferences.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // enable the transfer bit as we want to copy the raytraced image to swapchain
        windowTraits->vulkanVersion = VK_API_VERSION_1_1;
        windowTraits->deviceExtensionNames = {
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
            VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, VK_KHR_SPIRV_1_4_EXTENSION_NAME, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME};

        // set up features
        auto features = windowTraits->deviceFeatures = vsg::DeviceFeatures::create();
        /*auto& deviceFeatures =*/ features->get();

        auto& deviceAddressFeatures = features->get<VkPhysicalDeviceBufferDeviceAddressFeaturesEXT, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES>();
        deviceAddressFeatures.bufferDeviceAddress = 1;

        auto& rayTracingFeatures =  features->get<VkPhysicalDeviceRayTracingPipelineFeaturesKHR, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR>();
        rayTracingFeatures.rayTracingPipeline = 1;

        auto& accelerationFeatures = features->get<VkPhysicalDeviceAccelerationStructureFeaturesKHR,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR>();
        accelerationFeatures.accelerationStructure = 1;

        auto window = vsg::Window::create(windowTraits);
        if (!window)
        {
            std::cout << "Could not create windows." << std::endl;
            return 1;
        }

        viewer->addWindow(window);

        // load shaders

        auto raygenShader = vsg::read_cast<vsg::ShaderStage>("shaders/simple_raygen.rgen", options);
        auto missShader = vsg::read_cast<vsg::ShaderStage>("shaders/simple_miss.rmiss", options);
        auto closesthitShader = vsg::read_cast<vsg::ShaderStage>("shaders/simple_closesthit.rchit", options);

        // fallback to spv shaders if GLSL variants can't be loaded
        if (!raygenShader) raygenShader = vsg::ShaderStage::read(VK_SHADER_STAGE_RAYGEN_BIT_KHR, "main", "shaders/simple_raygen.spv", options);
        if (!missShader) missShader = vsg::ShaderStage::read(VK_SHADER_STAGE_MISS_BIT_KHR, "main", "shaders/simple_miss.spv", options);
        if (!closesthitShader) closesthitShader = vsg::ShaderStage::read(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "main", "shaders/simple_closesthit.spv", options);

        if (!raygenShader || !missShader || !closesthitShader)
        {
            std::cout << "Could not create shaders." << std::endl;
            return 1;
        }

        auto shaderStages = vsg::ShaderStages{raygenShader, missShader, closesthitShader};

        // set up shader groups
        auto raygenShaderGroup = vsg::RayTracingShaderGroup::create();
        raygenShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        raygenShaderGroup->generalShader = 0;

        auto missShaderGroup = vsg::RayTracingShaderGroup::create();
        missShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        missShaderGroup->generalShader = 1;

        auto closestHitShaderGroup = vsg::RayTracingShaderGroup::create();
        closestHitShaderGroup->type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        closestHitShaderGroup->closestHitShader = 2;

        auto shaderGroups = vsg::RayTracingShaderGroups{raygenShaderGroup, missShaderGroup, closestHitShaderGroup};

        // create camera matrices and uniform for shader
        auto perspective = vsg::Perspective::create(60.0, static_cast<double>(windowTraits->width) / static_cast<double>(windowTraits->height), 0.1, 10.0);
        vsg::ref_ptr<vsg::LookAt> lookAt;

        vsg::ref_ptr<vsg::Device> device;
        try
        {
            device = window->getOrCreateDevice();
        }
        catch (const vsg::Exception& exception)
        {
            std::cout << exception.message << " VkResult = " << exception.result << std::endl;
            return 0;
        }

        vsg::ref_ptr<vsg::TopLevelAccelerationStructure> tlas;
        vsg::ref_ptr<vsg::TopLevelAccelerationStructure> tlas2;
        //vsg::ref_ptr<vsg::GeometryInstance> geominstance;
        if (filename.empty())
        {
            // acceleration structures
            // set up vertex and index arrays
            auto vertices = vsg::vec3Array::create(
                {{-1.0f, -1.0f, 0.0f},
                 {1.0f, -1.0f, 0.0f},
                 {0.0f, 1.0f, 0.0f}});

            auto indices = vsg::uintArray::create(
                {0, 1, 2});

            // create acceleration geometry
            auto accelGeometry = vsg::AccelerationGeometry::create();
            accelGeometry->assignVertices(vertices);
            accelGeometry->assignIndices(indices);

            // create bottom level acceleration structure using acceleration geom
            auto blas = vsg::BottomLevelAccelerationStructure::create(device);
            blas->geometries.push_back(accelGeometry);

            // create top level acceleration structure
            tlas = vsg::TopLevelAccelerationStructure::create(device);

            // add geometry instance to top level acceleration structure that uses the bottom level structure
            auto geominstance = vsg::GeometryInstance::create();
            geominstance->accelerationStructure = blas;
            geominstance->transform = vsg::mat4();

            tlas->geometryInstances.push_back(geominstance);


            //geominstance = vsg::GeometryInstance::create();
            //geominstance->accelerationStructure = blas;
            //geominstance->transform = vsg::translate(-1.5f, 0.0f, 0.0f);

            //tlas->geometryInstances.push_back(geominstance);

            lookAt = vsg::LookAt::create(vsg::dvec3(0.0, 0.0, -2.5), vsg::dvec3(0.0, 0.0, 0.0), vsg::dvec3(0.0, 1.0, 0.0));
        }
        else
        {
            auto loaded_scene = vsg::read_cast<vsg::Node>(filename, options);
            auto loaded_scene2 = vsg::read_cast<vsg::Node>(filename, options);
            if (!loaded_scene)
            {
                std::cout << "Could not load model : " << filename << std::endl;
                return 1;
            }

            vsg::BuildAccelerationStructureTraversal buildAccelStruct(device);
            loaded_scene->accept(buildAccelStruct);
            tlas = buildAccelStruct.tlas;

            vsg::BuildAccelerationStructureTraversal buildAccelStruct2(device);
            loaded_scene2->accept(buildAccelStruct2);
            tlas2 = buildAccelStruct2.tlas;

            lookAt = vsg::LookAt::create(vsg::dvec3(0.0, 1.0, -5.0), vsg::dvec3(0.0, 0.5, 0.0), vsg::dvec3(0.0, 1.0, 0.0));
        }

        // for convenience create a compile context for creating our storage image
        vsg::CompileTraversal compile(window);

        // create storage image to render into
        auto storageImage = vsg::Image::create();
        storageImage->imageType = VK_IMAGE_TYPE_2D;
        storageImage->format = VK_FORMAT_B8G8R8A8_UNORM; //VK_FORMAT_R8G8B8A8_UNORM;
        storageImage->extent.width = windowTraits->width;
        storageImage->extent.height = windowTraits->height;
        storageImage->extent.depth = 1;
        storageImage->mipLevels = 1;
        storageImage->arrayLayers = 1;
        storageImage->samples = VK_SAMPLE_COUNT_1_BIT;
        storageImage->tiling = VK_IMAGE_TILING_OPTIMAL;
        storageImage->usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        storageImage->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        storageImage->flags = 0;
        storageImage->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto storageImageInfo = vsg::ImageInfo::create(vsg::ref_ptr<vsg::Sampler>{}, createImageView(compile.context, storageImage, VK_IMAGE_ASPECT_COLOR_BIT), VK_IMAGE_LAYOUT_GENERAL);

        auto raytracingUniformValues = new RayTracingUniformValue();
        raytracingUniformValues->value().projInverse = perspective->inverse();
        raytracingUniformValues->value().viewInverse = lookAt->inverse();

        vsg::ref_ptr<RayTracingUniformValue> raytracingUniform(raytracingUniformValues);

        // set up graphics pipeline
        vsg::DescriptorSetLayoutBindings descriptorBindings{
            {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr}, // { binding, descriptorTpe, descriptorCount, stageFlags, pImmutableSamplers}
            {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
            {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr}};

        auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);

        // create DescriptorSets and binding to bind our TopLevelAcceleration structure, storage image and camera matrix uniforms
        auto accelDescriptor = vsg::DescriptorAccelerationStructure::create(vsg::AccelerationStructures{tlas}, 0, 0);
        auto accelDescriptor2 = vsg::DescriptorAccelerationStructure::create(vsg::AccelerationStructures{tlas2}, 0, 0);

        auto storageImageDescriptor = vsg::DescriptorImage::create(storageImageInfo, 1, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

        auto raytracingUniformDescriptor = vsg::DescriptorBuffer::create(raytracingUniform, 2, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        raytracingUniformDescriptor->copyDataListToBuffers();


        auto shaderBindingTable = vsg::RayTracingShaderBindingTable::create();
        shaderBindingTable->bindingTableEntries.raygenGroups = { raygenShaderGroup };
        shaderBindingTable->bindingTableEntries.raymissGroups = { missShaderGroup };
        shaderBindingTable->bindingTableEntries.hitGroups = { closestHitShaderGroup };


        auto pipelineLayout = vsg::PipelineLayout::create(vsg::DescriptorSetLayouts{descriptorSetLayout}, vsg::PushConstantRanges{});
        auto raytracingPipeline = vsg::RayTracingPipeline::create(pipelineLayout, shaderStages, shaderGroups, shaderBindingTable, 0);
        auto bindRayTracingPipeline = vsg::BindRayTracingPipeline::create(raytracingPipeline);

        //auto descriptors2 = vsg::Descriptors{ accelDescriptor2, storageImageDescriptor, raytracingUniformDescriptor };
        auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, vsg::Descriptors{accelDescriptor, storageImageDescriptor, raytracingUniformDescriptor});
        auto bindDescriptorSets = vsg::BindDescriptorSets::create(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, raytracingPipeline->getPipelineLayout(), 0, vsg::DescriptorSets{descriptorSet});

        // state group to bind the pipeline and descriptor set
        auto scenegraph = vsg::Commands::create();
        scenegraph->addChild(bindRayTracingPipeline);
        //scenegraph->addChild(bindDescriptorSets);

        // setup tracing of rays
        auto traceRays = vsg::TraceRays::create();
        traceRays->bindingTable = shaderBindingTable;
        //traceRays->raygen = raygenShaderGroup;
        //traceRays->missShader = missShaderGroup;
        //traceRays->hitShader = closestHitShaderGroup;
        traceRays->width = windowTraits->width;
        traceRays->height = windowTraits->height;
        traceRays->depth = 1;

        scenegraph->addChild(traceRays);

        // camera related details
        auto viewport = vsg::ViewportState::create(0, 0, windowTraits->width, windowTraits->height);
        auto camera = vsg::Camera::create(perspective, lookAt, viewport);

        // assign a CloseHandler to the Viewer to respond to pressing Escape or press the window close button
        viewer->addEventHandlers({vsg::CloseHandler::create(viewer)});

        // set up commandGraph to rendering viewport
        auto commandGraph = vsg::CommandGraph::create(window);

        auto copyImageViewToWindow = vsg::CopyImageViewToWindow::create(storageImageInfo->imageView, window);

        commandGraph->addChild(scenegraph);
        commandGraph->addChild(copyImageViewToWindow);

        viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

        viewer->addEventHandler(vsg::Trackball::create(camera));

        viewer->compile();

        //tlas->geometryInstances[0]->transform = vsg::translate(-1.5f, 0.0f, 0.0f);
        //compileNode(tlas, window, viewport);


        //compileNode(geominstance, window, viewport);
        //tlas->geometryInstances.push_back(geominstance);
        //tlas->geometryInstances[0]->transform = vsg::translate(-1.5f, 0.0f, 0.0f);

        vsg::ref_ptr<vsg::CompileTraversal> compileTraversal;
        vsg::ref_ptr<vsg::Context> context;
        vsg::ref_ptr<vsg::AccelerationStructure> dummy;
        vsg::ref_ptr<vsg::CompileTraversal> dummyCompileTraversal;
        vsg::ref_ptr<vsg::DescriptorAccelerationStructure> dummyDescriptorAccelerationStructure;

        // rendering main loop
        int frameCount = 0;
        while (viewer->advanceToNextFrame() && (numFrames < 0 || (numFrames--) > 0))
        {
            if (frameCount == 0)
            {
                std::cout << "0" << std::endl;

                tlas->geometryInstances[0]->transform = vsg::translate(-1.5f, 0.0f, 0.0f);

                bindDescriptorSets->type = 1;
                scenegraph->addChild(bindDescriptorSets);

                compileTraversal = compileNode(bindDescriptorSets, dummy, window, viewport, false, dummyCompileTraversal, false, dummyDescriptorAccelerationStructure, false);
                context = &compileTraversal->context;
                
            }
            if (frameCount == 200)
            {
                std::cout << "200" << std::endl;

                //int index = -1;
                //auto children = scenegraph->children;
                //std::cout << std::to_string(children.size()) << std::endl;
                //for (int i = 0; i < children.size(); ++i) {
                //    auto command = children[i];
                //    if (command->type == 1) {
                //        index = i;
                //        break;
                //    }
                //}
                //if (index >= 0) {
                //    children.erase(children.begin() + index);
                //}
                //std::cout << std::to_string(children.size()) << std::endl;


                //scenegraph->addChild(bindDescriptorSets);
                //tlas->geometryInstances[0]->transform = vsg::translate(-1.5f, 0.0f, 0.0f);
                //compileNode(bindDescriptorSets, window, viewport);
                
                //context->record();
                //context->waitForCompletion();

                //tlas->update = true;
                //auto tlasCompileTraversal = compileNode(tlas, tlas, window, viewport, true);

                //auto tlasCompileTraversal2 = compileNode(tlas2, tlas2, window, viewport, true, dummyCompileTraversal, false);
                //auto tlasCompileTraversal2 = compileNode(accelDescriptor2, dummy, window, viewport, false, dummyCompileTraversal, false);
                auto tlasCompileTraversal2 = compileNode(accelDescriptor2, dummy, window, viewport, false, compileTraversal, true, accelDescriptor2, true);

                //compileNode(accelDescriptor2, dummy, window, viewport, false);
                //descriptorSet->_implementation[context->deviceID]->assign(*context, descriptors2);



                //descriptorSet->_implementation[context->deviceID] = nullptr;

                //descriptorSet->descriptors = descriptors2;
                //compileNode(descriptorSet, dummy, window, viewport, false, compileTraversal, true);



                std::cout << "TEST1" << std::endl;

                VkWriteDescriptorSet* descriptorWrites = context->scratchMemory->allocate<VkWriteDescriptorSet>(1);

                std::cout << "TEST2" << std::endl;



                VkWriteDescriptorSetAccelerationStructureKHR* asDescriptorWrites = context->scratchMemory->allocate<VkWriteDescriptorSetAccelerationStructureKHR>(1);

                std::cout << "TEST2" << std::endl;


                asDescriptorWrites->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                asDescriptorWrites->accelerationStructureCount = static_cast<uint32_t>(accelDescriptor2->_vkAccelerationStructures.size());
                asDescriptorWrites->pAccelerationStructures = accelDescriptor2->_vkAccelerationStructures.data();
                asDescriptorWrites->pNext = nullptr;


                //*descriptorWrites = {};
                descriptorWrites->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites->dstBinding = accelDescriptor2->dstBinding;
                descriptorWrites->dstArrayElement = accelDescriptor2->dstArrayElement;
                descriptorWrites->descriptorType = accelDescriptor2->descriptorType;

                descriptorWrites->descriptorCount = static_cast<uint32_t>(accelDescriptor2->_vkAccelerationStructures.size());
                descriptorWrites->pNext = asDescriptorWrites;

                //accelDescriptor2->assignTo(*context, descriptorWrites[0]);
                descriptorWrites->dstSet = descriptorSet->_implementation[context->deviceID]->_descriptorSet;

                std::cout << "TEST3" << std::endl;

                std::cout << device->getDevice() << std::endl;
                std::cout << static_cast<uint32_t>(1) << std::endl;

                std::cout << descriptorWrites->dstSet << std::endl;
                std::cout << descriptorWrites->dstBinding << std::endl;
                std::cout << descriptorWrites->dstArrayElement << std::endl;
                std::cout << descriptorWrites->descriptorCount << std::endl;
                std::cout << descriptorWrites->descriptorType << std::endl;
                std::cout << descriptorWrites->pImageInfo << std::endl;
                std::cout << descriptorWrites->pBufferInfo << std::endl;
                std::cout << descriptorWrites->pTexelBufferView << std::endl;


                std::cout << asDescriptorWrites->sType << std::endl;
                std::cout << asDescriptorWrites->accelerationStructureCount << std::endl;
                std::cout << asDescriptorWrites->pAccelerationStructures << std::endl;
                std::cout << asDescriptorWrites->pNext << std::endl;


                std::cout << "TEST4" << std::endl;

                vkUpdateDescriptorSets(device->getDevice(), static_cast<uint32_t>(1), descriptorWrites, 0, nullptr);

                // clean up scratch memory so it can be reused.
                context->scratchMemory->release();

                std::cout << "TEST END" << std::endl;




                //tlas2->update = true;
                //auto tlasCompileTraversal = compileNode(tlas2, tlas2, window, viewport, true);
                

            }


            // pass any events into EventHandlers assigned to the Viewer
            viewer->handleEvents();

            //update camera matrix
            raytracingUniformValues->value().viewInverse = lookAt->inverse();
            raytracingUniformDescriptor->copyDataListToBuffers();

            viewer->update();

            viewer->recordAndSubmit();

            viewer->present();

            frameCount++;
        }

        // clean up done automatically thanks to ref_ptr<>
        return 0;
    }
    catch (const vsg::Exception& exception)
    {
        std::cout << exception.message << " VkResult = " << exception.result << std::endl;
        return 0;
    }

    // clean up done automatically thanks to ref_ptr<>
    return 0;
}

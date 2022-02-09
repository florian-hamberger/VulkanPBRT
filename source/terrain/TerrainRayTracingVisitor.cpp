#include "TerrainRayTracingVisitor.hpp"

TerrainRayTracingSceneDescriptorCreationVisitor::TerrainRayTracingSceneDescriptorCreationVisitor() :
    Inherit()
{
}

void TerrainRayTracingSceneDescriptorCreationVisitor::apply(vsg::BindDescriptorSet& bds)
{
    // TODO: every material that is not set should get a default material assigned
    std::set<int> setTextures;
    isOpaque.push_back(true);
    for (const auto& descriptor : bds.descriptorSet->descriptors)
    {
        if (descriptor->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) //pbr material
        {
            auto d = descriptor.cast<vsg::DescriptorBuffer>();
            if (d->bufferInfoList[0]->data->dataSize() == sizeof(VsgPbrMaterial))
            {
                // pbr material
                VsgPbrMaterial vsgMat;
                std::memcpy(&vsgMat, d->bufferInfoList[0]->data->dataPointer(), sizeof(VsgPbrMaterial));
                WaveFrontMaterialPacked mat;
                std::memcpy(&mat.ambientShininess, &vsgMat.baseColorFactor, sizeof(vsg::vec4));
                std::memcpy(&mat.specularDissolve, &vsgMat.specularFactor, sizeof(vsg::vec4));
                //std::memcpy(&mat.diffuseIor, &vsgMat.diffuseFactor, sizeof(vsg::vec4));
                std::memcpy(&mat.diffuseIor, &vsgMat.baseColorFactor, sizeof(vsg::vec4));
                std::memcpy(&mat.emissionTextureId, &vsgMat.emissiveFactor, sizeof(vsg::vec4));
                if (vsgMat.emissiveFactor.r + vsgMat.emissiveFactor.g + vsgMat.emissiveFactor.b != 0) meshEmissive = true;
                else meshEmissive = false;
                mat.ambientShininess.w = vsgMat.roughnessFactor;
                mat.diffuseIor.w = vsgMat.indexOfRefraction;
                mat.specularDissolve.w = vsgMat.alphaMask;
                mat.emissionTextureId.w = vsgMat.alphaMaskCutoff;
                mat.category_id = vsgMat.category_id;
                _materialArray.push_back(mat);
            }
            else
            {
                // normal material
                VsgMaterial vsgMat;
                std::memcpy(&vsgMat, d->bufferInfoList[0]->data->dataPointer(), sizeof(VsgMaterial));
                WaveFrontMaterialPacked mat;
                std::memcpy(&mat.ambientShininess, &vsgMat.ambient, sizeof(vsg::vec4));
                std::memcpy(&mat.specularDissolve, &vsgMat.specular, sizeof(vsg::vec4));
                std::memcpy(&mat.diffuseIor, &vsgMat.diffuse, sizeof(vsg::vec4));
                std::memcpy(&mat.emissionTextureId, &vsgMat.emissive, sizeof(vsg::vec4));
                if (vsgMat.emissive.r + vsgMat.emissive.g + vsgMat.emissive.b != 0) meshEmissive = true;
                else meshEmissive = false;
                mat.ambientShininess.w = vsgMat.shininess;
                mat.diffuseIor.w = 1;
                mat.specularDissolve.w = vsgMat.alphaMask;
                mat.emissionTextureId.w = vsgMat.alphaMaskCutoff;
                mat.category_id = vsgMat.category_id;
                _materialArray.push_back(mat);
            }
            continue;
        }

        if (descriptor->descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) continue;

        vsg::ref_ptr<vsg::DescriptorImage> d = descriptor.cast<vsg::DescriptorImage>(); //cast to descriptor image
        vsg::ref_ptr<vsg::DescriptorImage> texture;
        switch (descriptor->dstBinding)
        {
        case 0: //diffuse map
            texture = vsg::DescriptorImage::create(d->imageInfoList, 6, _diffuse.size());
            _diffuse.push_back(texture);
            setTextures.insert(6);
            break;
        case 1: //metall roughness map
            texture = vsg::DescriptorImage::create(d->imageInfoList, 7, _mr.size());
            _mr.push_back(texture);
            setTextures.insert(7);
            break;
        case 2: //normal map
            texture = vsg::DescriptorImage::create(d->imageInfoList, 8, _normal.size());
            _normal.push_back(texture);
            setTextures.insert(8);
            break;
        case 3: //light map
            break;
        case 4: //emissive map
            texture = vsg::DescriptorImage::create(d->imageInfoList, 10, _emissive.size());
            _emissive.push_back(texture);
            setTextures.insert(10);
            break;
        case 5: //specular map
            texture = vsg::DescriptorImage::create(d->imageInfoList, 11, _specular.size());
            _specular.push_back(texture);
            setTextures.insert(11);
        default:
            std::cout << "Unkown texture binding. Could not properly detect material" << std::endl;
        }
    }

    //setting the default texture for not set textures
    for (int i = 6; i < 12; ++i)
    {
        if (setTextures.find(i) == setTextures.end())
        {
            vsg::ref_ptr<vsg::DescriptorImage> texture;
            switch (i)
            {
            case 6:
                texture = vsg::DescriptorImage::create(_defaultTexture->imageInfoList, i, _diffuse.size());
                _diffuse.push_back(texture);
                break;
            case 7:
                texture = vsg::DescriptorImage::create(_defaultTexture->imageInfoList, i, _mr.size());
                _mr.push_back(texture);
                break;
            case 8:
                texture = vsg::DescriptorImage::create(_defaultTexture->imageInfoList, i, _normal.size());
                _normal.push_back(texture);
                break;
            case 9:
                break;
            case 10:
                texture = vsg::DescriptorImage::create(_defaultTexture->imageInfoList, i, _emissive.size());
                _emissive.push_back(texture);
                break;
            case 11:
                texture = vsg::DescriptorImage::create(_defaultTexture->imageInfoList, i, _specular.size());
                _specular.push_back(texture);
                break;
            }
        }
    }
}
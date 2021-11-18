#pragma once

#include <vsg/all.h>

//from vsgXchange/assimp/assimp.cpp
struct Material
{
    vsg::vec4 ambient{ 0.0f, 0.0f, 0.0f, 1.0f };
    vsg::vec4 diffuse{ 1.0f, 1.0f, 1.0f, 1.0f };
    vsg::vec4 specular{ 0.0f, 0.0f, 0.0f, 1.0f };
    vsg::vec4 emissive{ 0.0f, 0.0f, 0.0f, 1.0f };
    float shininess{ 0.0f };
    float alphaMask{ 1.0 };
    float alphaMaskCutoff{ 0.5 };

    vsg::ref_ptr<vsg::Data> toData()
    {
        auto buffer = vsg::ubyteArray::create(sizeof(Material));
        std::memcpy(buffer->data(), &ambient.r, sizeof(Material));
        return buffer;
    }
};


class TerrainImporter : public vsg::Inherit<vsg::Object, TerrainImporter>
{
public:
    TerrainImporter();

    vsg::ref_ptr<vsg::Node> TerrainImporter::importTerrain(const vsg::Path& filename);

private:
    using StateCommandPtr = vsg::ref_ptr<vsg::StateCommand>;
    using State = std::pair<StateCommandPtr, StateCommandPtr>;

    vsg::ref_ptr<vsg::Node> TerrainImporter::createTestScene();
    TerrainImporter::State TerrainImporter::createTestMaterial();
    vsg::mat4 TerrainImporter::createIdentityMatrix();
    std::string TerrainImporter::mat4ToString(vsg::mat4 m);
};
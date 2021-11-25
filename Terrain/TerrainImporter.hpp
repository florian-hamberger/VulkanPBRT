#pragma once

#include <vsg/all.h>
#include <vsgXchange/models.h>
#include <vsgXchange/images.h>

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

enum aiTextureMapMode
{
    /** A texture coordinate u|v is translated to u%1|v%1
     */
    aiTextureMapMode_Wrap = 0x0,

    /** Texture coordinates outside [0...1]
     *  are clamped to the nearest valid value.
     */
     aiTextureMapMode_Clamp = 0x1,

     /** If the texture coordinates for a pixel are outside [0...1]
      *  the texture is not applied to that pixel
      */
      aiTextureMapMode_Decal = 0x3,

      /** A texture coordinate u|v becomes u%1|v%1 if (u-(u%1))%2 is zero and
       *  1-(u%1)|1-(v%1) otherwise
       */
       aiTextureMapMode_Mirror = 0x2,
};


class TerrainImporter : public vsg::Inherit<vsg::Object, TerrainImporter>
{
public:
    TerrainImporter();

    vsg::ref_ptr<vsg::Node> TerrainImporter::importTerrain(const vsg::Path& heightmapPath, const vsg::Path& texturePath);

private:
    using StateCommandPtr = vsg::ref_ptr<vsg::StateCommand>;
    using State = std::pair<StateCommandPtr, StateCommandPtr>;
    using BindState = std::vector<State>;

    vsg::vec3 TerrainImporter::getHeightmapVertexPosition(int x, int y, vsg::ref_ptr<vsg::ubvec4Array2D> heightmap);
    vsg::ref_ptr<vsg::Node> TerrainImporter::createGeometry(vsg::ref_ptr<vsg::ubvec4Array2D> heightmap, vsg::ref_ptr<vsg::Data> texture);
    TerrainImporter::State TerrainImporter::createTestMaterial();
    TerrainImporter::State TerrainImporter::loadTextureMaterials(vsg::ref_ptr<vsg::Data> texture);
    vsg::mat4 TerrainImporter::createIdentityMatrix();
    std::string TerrainImporter::mat4ToString(vsg::mat4 m);
};
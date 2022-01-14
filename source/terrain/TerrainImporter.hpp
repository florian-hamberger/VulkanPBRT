#pragma once

#include <vsg/all.h>
#include <vsgXchange/models.h>
#include <vsgXchange/images.h>
#include <iostream>
#include <fstream>

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
    uint32_t category_id{ 0 };

    vsg::ref_ptr<vsg::Data> toData()
    {
        auto buffer = vsg::ubyteArray::create(sizeof(Material));
        std::memcpy(buffer->data(), &ambient.r, sizeof(Material));
        return buffer;
    }
};

struct PbrMaterial
{
    vsg::vec4 baseColorFactor{ 1.0, 1.0, 1.0, 1.0 };
    vsg::vec4 emissiveFactor{ 0.0, 0.0, 0.0, 1.0 };
    vsg::vec4 diffuseFactor{ 1.0, 1.0, 1.0, 1.0 };
    vsg::vec4 specularFactor{ 0.0, 0.0, 0.0, 1.0 };
    float metallicFactor{ 1.0f };
    float roughnessFactor{ 1.0f };
    float alphaMask{ 1.0f };
    float alphaMaskCutoff{ 0.5f };
    float indexOfRefraction{ 1.0f };
    uint32_t category_id{ 0 };

    vsg::ref_ptr<vsg::Data> toData()
    {
        auto buffer = vsg::ubyteArray::create(sizeof(PbrMaterial));
        std::memcpy(buffer->data(), &baseColorFactor.r, sizeof(PbrMaterial));
        return buffer;
    }
};
struct SamplerData
{
    vsg::ref_ptr<vsg::Sampler> sampler;
    vsg::ref_ptr<vsg::Data> data;
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
    TerrainImporter(const vsg::Path& heightmapPath, const vsg::Path& texturePath, float terrainScale, float terrainVertexHeightToPixelRatio, bool terrainFormatLa2d, bool textureFormatS3tc, int heightmapLod, int textureLod);

    vsg::ref_ptr<vsg::Node> TerrainImporter::importTerrain();

private:
    using StateCommandPtr = vsg::ref_ptr<vsg::StateCommand>;
    using State = std::pair<StateCommandPtr, StateCommandPtr>;
    using BindState = std::vector<State>;

    const vsg::Path& heightmapPath, texturePath;
    float terrainScale;
    float terrainScaleVertexHeight;
    bool terrainFormatLa2d;
    bool textureFormatS3tc;
    int heightmapLod;
    int textureLod;



    std::ifstream heightmapIfs;
    float* heightmapLa2dBuffer;
    std::ifstream textureIfs;

    vsg::ref_ptr<vsg::ubvec4Array2D> heightmap;
    vsg::ref_ptr<vsg::Data> texture;


    uint32_t heightmapActualWidth;
    uint32_t heightmapActualHeight;
    int heightmapFullWidth;
    int heightmapFullHeight;

    uint32_t textureActualWidth;
    uint32_t textureActualHeight;
    int textureFullWidth;
    int textureFullHeight;

    vsg::vec3 TerrainImporter::getHeightmapVertexPosition(int x, int y);
    vsg::vec2 TerrainImporter::getTextureCoordinate(int x, int y);
    vsg::ref_ptr<vsg::Node> TerrainImporter::createGeometry();
    TerrainImporter::State TerrainImporter::loadTextureMaterials();
    vsg::mat4 TerrainImporter::createIdentityMatrix();
    std::string TerrainImporter::mat4ToString(vsg::mat4 m);
};
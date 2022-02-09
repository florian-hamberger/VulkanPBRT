#pragma once

#include <vsg/all.h>
#include <vsgXchange/models.h>
#include <vsgXchange/images.h>
#include <iostream>
#include <fstream>

//from vsgXchange/assimp/assimp.cpp
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
    TerrainImporter(const vsg::Path& heightmapPath, const vsg::Path& texturePath, float terrainScale, float terrainVertexHeightToPixelRatio, bool terrainFormatLa2d, bool textureFormatS3tc, int heightmapLod, int textureLod, int test, uint32_t tileCountX, uint32_t tileCountY, int tileLengthLodFactor);

    vsg::ref_ptr<vsg::Node> TerrainImporter::importTerrain();

    vsg::ref_ptr<vsg::Node> loadedScene;
    vsg::ref_ptr<vsg::Array2D<vsg::ref_ptr<vsg::Node>>> loadedTileNodes;

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
    uint32_t tileCountX;
    uint32_t tileCountY;
    int tileLengthLodFactor;

    int test;



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

    float scaleModifier;

    uint32_t TerrainImporter::getVertexIndex(long x, long y, long width);
    vsg::vec3 TerrainImporter::getHeightmapVertexPosition(long xTile, long yTile, long tileStartX, long tileStartY, float heightOffset);
    vsg::vec2 TerrainImporter::getTextureCoordinate(long x, long y);
    vsg::ref_ptr<vsg::Node> TerrainImporter::createGeometry();
    vsg::ref_ptr<vsg::Array2D<vsg::ref_ptr<vsg::Node>>> TerrainImporter::createTileNodes();
    TerrainImporter::State TerrainImporter::loadTextureMaterials();
    std::string TerrainImporter::mat4ToString(vsg::mat4 m);
};
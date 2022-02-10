#pragma once

#include <vsg/all.h>
#include "TerrainImporter.hpp"

using BlasTiles = vsg::Array<vsg::ref_ptr<vsg::Array2D<vsg::ref_ptr<vsg::GeometryInstance>>>>;
using NodeTiles = vsg::Array<vsg::ref_ptr<vsg::Array2D<vsg::ref_ptr<vsg::Node>>>>;

class TerrainAccelerationStructureManager : public vsg::Inherit<vsg::Object, TerrainAccelerationStructureManager>
{
public:
    TerrainAccelerationStructureManager(vsg::ref_ptr<vsg::Device> device, const vsg::Path& heightmapPath, const vsg::Path& texturePath, float terrainScale, float terrainScaleVertexHeight, bool terrainFormatLa2d, bool textureFormatS3tc, int tileLengthLodFactor, int lodLevelCount);
    std::pair<vsg::ref_ptr<vsg::TopLevelAccelerationStructure>, vsg::ref_ptr<vsg::Node>> createTlasAndScene(vsg::dvec3 eyePos, int lodViewDistance, bool maxLod);
    std::vector < vsg::ref_ptr<vsg::TopLevelAccelerationStructure>> buildAllLodTlas(vsg::ref_ptr<vsg::Context> context);

    const vsg::Path& heightmapPath, texturePath;
    float terrainScale;
    float terrainScaleVertexHeight;
    bool terrainFormatLa2d;
    bool textureFormatS3tc;
    int tileLengthLodFactor;
private:
    void loadAllLodLevels();

    int lodLevelCount;
    int minLod;

    uint32_t tileCountX;
    uint32_t tileCountY;

    vsg::ref_ptr<vsg::Device> device;

    vsg::ref_ptr<BlasTiles> blasTiles;
    vsg::ref_ptr<NodeTiles> nodeTiles;
};

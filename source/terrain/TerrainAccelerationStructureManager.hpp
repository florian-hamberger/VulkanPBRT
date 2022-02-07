#pragma once

#include <vsg/all.h>
#include "TerrainImporter.hpp"

using BlasTiles = vsg::Array3D<vsg::ref_ptr<vsg::GeometryInstance>>;
using TileNodes = vsg::Array<vsg::ref_ptr<vsg::Array2D<vsg::ref_ptr<vsg::Node>>>>;

class TerrainAccelerationStructureManager : public vsg::Inherit<vsg::Object, TerrainAccelerationStructureManager>
{
public:
    TerrainAccelerationStructureManager(uint32_t tileCountX, uint32_t tileCountY, uint32_t lodLevelCount, vsg::ref_ptr<vsg::Context> context);
    void loadLodLevel(vsg::ref_ptr<TerrainImporter> terrainImporter, int lodLevel);
    vsg::ref_ptr<vsg::TopLevelAccelerationStructure> createTlas(int lodLevel, bool test);
    vsg::ref_ptr<vsg::Node> createScene(int lodLevel);
private:
    uint32_t tileCountX;
    uint32_t tileCountY;
    uint32_t lodLevelCount;
    vsg::ref_ptr<vsg::Context> context;

    vsg::ref_ptr<BlasTiles> blasTiles;
    vsg::ref_ptr<TileNodes> tileNodes;
};

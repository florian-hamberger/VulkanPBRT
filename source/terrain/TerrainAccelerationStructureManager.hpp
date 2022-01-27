#pragma once

#include <vsg/all.h>
#include "TerrainImporter.hpp"

using BlasTiles = vsg::Array3D<vsg::ref_ptr<vsg::GeometryInstance>>;

class TerrainAccelerationStructureManager : public vsg::Inherit<vsg::Object, TerrainAccelerationStructureManager>
{
public:
    TerrainAccelerationStructureManager(uint32_t tileCountX, uint32_t tileCountY, uint32_t lodLevels, vsg::ref_ptr<vsg::Context> context);
    void loadLodLevel(vsg::ref_ptr<TerrainImporter> terrainImporter, uint32_t lodLevel);
    vsg::ref_ptr<vsg::TopLevelAccelerationStructure> createTlas(uint32_t lodLevel);
private:
    uint32_t tileCountX;
    uint32_t tileCountY;
    uint32_t lodLevels;
    vsg::ref_ptr<vsg::Context> context;

    vsg::ref_ptr<BlasTiles> blasTiles;
};

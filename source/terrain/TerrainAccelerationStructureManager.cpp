#include "TerrainAccelerationStructureManager.hpp"

TerrainAccelerationStructureManager::TerrainAccelerationStructureManager(uint32_t tileCountX, uint32_t tileCountY, uint32_t lodLevels, vsg::ref_ptr<vsg::Context> context) :
    tileCountX(tileCountX),
    tileCountY(tileCountY),
    lodLevels(lodLevels),
    context(context)
{

    blasTiles = BlasTiles::create(tileCountX, tileCountY, lodLevels);

}

void TerrainAccelerationStructureManager::loadLodLevel(vsg::ref_ptr<TerrainImporter> terrainImporter, uint32_t lodLevel)
{
    auto loaded_scene = terrainImporter->importTerrain();

    vsg::BuildAccelerationStructureTraversal buildAccelStruct(context->device);
    loaded_scene->accept(buildAccelStruct);
    auto tlas = buildAccelStruct.tlas;

    for (uint32_t i = 0; i < tlas->geometryInstances.size(); ++i) {
        auto geometryInstance = tlas->geometryInstances[i];
        blasTiles->at(i % tileCountX, i / tileCountY, lodLevel) = geometryInstance;
    }


}

vsg::ref_ptr<vsg::TopLevelAccelerationStructure> TerrainAccelerationStructureManager::createTlas(uint32_t lodLevel)
{
    auto tlas = vsg::TopLevelAccelerationStructure::create(context->device);

    for (uint32_t y = 0; y < tileCountY; ++y) {
        for (uint32_t x = 0; x < tileCountX; ++x) {
            auto geometryInstance = blasTiles->at(x, y, lodLevel);
            tlas->geometryInstances.push_back(geometryInstance);
        }
    }

    return tlas;
}
#include "TerrainAccelerationStructureManager.hpp"

TerrainAccelerationStructureManager::TerrainAccelerationStructureManager(uint32_t tileCountX, uint32_t tileCountY, uint32_t lodLevels, vsg::ref_ptr<vsg::Context> context) :
    tileCountX(tileCountX),
    tileCountY(tileCountY),
    lodLevels(lodLevels),
    context(context)
{

    blasTiles = BlasTiles::create(tileCountX, tileCountY, lodLevels);
    tileNodes = TileNodes::create(lodLevels);

}

void TerrainAccelerationStructureManager::loadLodLevel(vsg::ref_ptr<TerrainImporter> terrainImporter, uint32_t lodLevel)
{
    auto loaded_scene = terrainImporter->importTerrain();
    tileNodes->set(lodLevel, terrainImporter->loadedTileNodes);

    vsg::BuildAccelerationStructureTraversal buildAccelStruct(context->device);
    loaded_scene->accept(buildAccelStruct);
    auto tlas = buildAccelStruct.tlas;

    for (uint32_t i = 0; i < tlas->geometryInstances.size(); ++i) {
        auto geometryInstance = tlas->geometryInstances[i];
        blasTiles->at(i % tileCountX, i / tileCountX, lodLevel) = geometryInstance;
    }


}

vsg::ref_ptr<vsg::TopLevelAccelerationStructure> TerrainAccelerationStructureManager::createTlas(uint32_t lodLevel)
{
    auto tlas = vsg::TopLevelAccelerationStructure::create(context->device);

    for (uint32_t y = 0; y < tileCountY; ++y) {
        for (uint32_t x = 0; x < tileCountX; ++x) {
            if (x % 2 == y % 2) {
                auto geometryInstance = blasTiles->at(x, y, lodLevel);
                tlas->geometryInstances.push_back(geometryInstance);
            } else {
                auto geometryInstance = blasTiles->at(x, y, lodLevel+1);
                tlas->geometryInstances.push_back(geometryInstance);
            }
        }
    }

    return tlas;
}

vsg::ref_ptr<vsg::Node> TerrainAccelerationStructureManager::createScene(uint32_t lodLevel)
{

    auto root = vsg::MatrixTransform::create();
    root->matrix = vsg::mat4();

    auto scenegraph = vsg::StateGroup::create();

    for (int y = 0; y < tileCountY; ++y) {
        for (int x = 0; x < tileCountX; ++x) {
            if (x % 2 == y % 2) {
                auto tileNode = tileNodes->at(lodLevel)->at(x, y);
                scenegraph->addChild(tileNode);
            }
            else {
                auto tileNode = tileNodes->at(lodLevel+1)->at(x, y);
                scenegraph->addChild(tileNode);
            }
            //auto tileNode = tileNodes->at(lodLevel + 1)->at(x, y);
            //scenegraph->addChild(tileNode);
        }
    }

    root->addChild(scenegraph);
    return root;
}

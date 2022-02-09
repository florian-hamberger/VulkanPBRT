#include "TerrainAccelerationStructureManager.hpp"

TerrainAccelerationStructureManager::TerrainAccelerationStructureManager(uint32_t tileCountX, uint32_t tileCountY, uint32_t lodLevelCount, vsg::ref_ptr<vsg::Context> context) :
    tileCountX(tileCountX),
    tileCountY(tileCountY),
    lodLevelCount(lodLevelCount),
    context(context)
{

    blasTiles = BlasTiles::create(tileCountX, tileCountY, lodLevelCount);
    tileNodes = TileNodes::create(lodLevelCount);

}

void TerrainAccelerationStructureManager::loadLodLevel(vsg::ref_ptr<TerrainImporter> terrainImporter, int lodLevel)
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

vsg::ref_ptr<vsg::TopLevelAccelerationStructure> TerrainAccelerationStructureManager::createTlas(int lodLevel, bool test)
{
    auto tlas = vsg::TopLevelAccelerationStructure::create(context->device);

    //for (uint32_t y = 0; y < tileCountY; ++y) {
    //    for (uint32_t x = 0; x < tileCountX; ++x) {
    //        if (x % 2 == y % 2) {
    //            auto geometryInstance = blasTiles->at(x, y, lodLevel);
    //            tlas->geometryInstances.push_back(geometryInstance);
    //        }
    //        else {
    //            auto geometryInstance = blasTiles->at(x, y, lodLevel + 1);
    //            tlas->geometryInstances.push_back(geometryInstance);
    //        }
    //    }
    //}

    for (uint32_t y = 0; y < tileCountY; ++y) {
        for (uint32_t x = 0; x < tileCountX; ++x) {
            int lod = lodLevelCount - 1 - ((x + y) / 8);
            if (lod < lodLevel) lod = lodLevel;

            if (test) lod = lodLevel;

            auto geometryInstance = blasTiles->at(x, y, lod);
            tlas->geometryInstances.push_back(geometryInstance);
        }
    }

    return tlas;
}

vsg::ref_ptr<vsg::Node> TerrainAccelerationStructureManager::createScene(int lodLevel)
{

    auto root = vsg::MatrixTransform::create();
    root->matrix = vsg::mat4();

    auto scenegraph = vsg::StateGroup::create();

    //for (int y = 0; y < tileCountY; ++y) {
    //    for (int x = 0; x < tileCountX; ++x) {
    //        if (x % 2 == y % 2) {
    //            auto tileNode = tileNodes->at(lodLevel)->at(x, y);
    //            scenegraph->addChild(tileNode);
    //        }
    //        else {
    //            auto tileNode = tileNodes->at(lodLevel + 1)->at(x, y);
    //            scenegraph->addChild(tileNode);
    //        }
    //        //auto tileNode = tileNodes->at(lodLevel + 1)->at(x, y);
    //        //scenegraph->addChild(tileNode);
    //    }
    //}

    for (int y = 0; y < tileCountY; ++y) {
        for (int x = 0; x < tileCountX; ++x) {
            int lod = lodLevelCount - 1 - ((x + y) / 8);
            if (lod < lodLevel) lod = lodLevel;

            auto tileNode = tileNodes->at(lod)->at(x, y);
            scenegraph->addChild(tileNode);
        }
    }

    root->addChild(scenegraph);
    return root;
}

std::pair<vsg::ref_ptr<vsg::TopLevelAccelerationStructure>, vsg::ref_ptr<vsg::Node>> TerrainAccelerationStructureManager::createTlasAndScene(int minLod, vsg::dvec3 eyePosInTileCoords)
{
    auto tlas = vsg::TopLevelAccelerationStructure::create(context->device);

    auto sceneRoot = vsg::MatrixTransform::create();
    sceneRoot->matrix = vsg::mat4();

    auto scenegraph = vsg::StateGroup::create();

    for (int y = 0; y < tileCountY; ++y) {
        for (int x = 0; x < tileCountX; ++x) {
            vsg::dvec3 tilePos(x, y, 0);
            tilePos += vsg::dvec3(0.5, 0.5, 0.0);
            double distance = vsg::length(tilePos - eyePosInTileCoords);
            distance *= 0.1;
            int lod = lodLevelCount - 1 - round(distance);
            if (lod < minLod) lod = minLod;

            auto geometryInstance = blasTiles->at(x, y, lod);
            tlas->geometryInstances.push_back(geometryInstance);

            auto tileNode = tileNodes->at(lod)->at(x, y);
            scenegraph->addChild(tileNode);
        }
    }

    sceneRoot->addChild(scenegraph);

    return { tlas, sceneRoot };
}

vsg::ref_ptr<vsg::Node> TerrainAccelerationStructureManager::createCompleteScene(int minLod)
{
    auto root = vsg::MatrixTransform::create();
    root->matrix = vsg::mat4();

    auto scenegraph = vsg::StateGroup::create();

    for (int currentLod = minLod; currentLod < lodLevelCount; ++currentLod) {
    //for (int currentLod = lodLevelCount-1; currentLod >= minLod; --currentLod) {
        auto currentLodTiles = tileNodes->at(currentLod);
        for (int y = 0; y < tileCountY; ++y) {
            for (int x = 0; x < tileCountX; ++x) {
                auto tileNode = currentLodTiles->at(x, y);
                scenegraph->addChild(tileNode);
            }
        }
    }

    root->addChild(scenegraph);
    return root;
}

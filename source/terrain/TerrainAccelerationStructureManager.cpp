#include "TerrainAccelerationStructureManager.hpp"

TerrainAccelerationStructureManager::TerrainAccelerationStructureManager(vsg::ref_ptr<vsg::Device> device, const vsg::Path& heightmapPath, const vsg::Path& texturePath, float terrainScale, float terrainScaleVertexHeight, bool terrainFormatLa2d, bool textureFormatS3tc, int tileLengthLodFactor, int lodLevelCount) :
    device(device),
    heightmapPath(heightmapPath),
    texturePath(texturePath),
    terrainScale(terrainScale),
    terrainScaleVertexHeight(terrainScaleVertexHeight),
    terrainFormatLa2d(terrainFormatLa2d),
    textureFormatS3tc(textureFormatS3tc),
    tileLengthLodFactor(tileLengthLodFactor),
    lodLevelCount(lodLevelCount)
{
    tileCountX = 0;
    tileCountY = 0;

    minLod = 0;
    if (tileLengthLodFactor < 0) {
        minLod = -tileLengthLodFactor;
    }

    blasTiles = BlasTiles::create(lodLevelCount);
    nodeTiles = NodeTiles::create(lodLevelCount);

    loadAllLodLevels();
}

void TerrainAccelerationStructureManager::loadAllLodLevels()
{
    for (int currentLod = minLod; currentLod < lodLevelCount; ++currentLod) {
        std::cout << "Loading LOD " << currentLod << std::endl;

        auto terrainImporter = TerrainImporter::create(heightmapPath, texturePath, terrainScale, terrainScaleVertexHeight, terrainFormatLa2d, textureFormatS3tc, tileLengthLodFactor, currentLod, currentLod, 0);
        auto loadedScene = terrainImporter->importTerrain();
        nodeTiles->set(currentLod, terrainImporter->loadedTiles);

        vsg::BuildAccelerationStructureTraversal buildAccelStruct(device);
        loadedScene->accept(buildAccelStruct);
        auto tlas = buildAccelStruct.tlas;

        if (currentLod == minLod) {
            tileCountX = terrainImporter->loadedTiles->width();
            tileCountY = terrainImporter->loadedTiles->height();
        }
        else if (tileCountX != terrainImporter->loadedTiles->width() || tileCountY != terrainImporter->loadedTiles->height()) {
            std::cout << "Error: LOD tiles height or width mismatch!" << std::endl;
        }
        
        auto blasTilesCurrentLod = vsg::Array2D<vsg::ref_ptr<vsg::GeometryInstance>>::create(tileCountX, tileCountY);
        blasTiles->set(currentLod, blasTilesCurrentLod);

        for (uint32_t i = 0; i < tlas->geometryInstances.size(); ++i) {
            auto geometryInstance = tlas->geometryInstances[i];
            blasTiles->at(currentLod)->at(i % tileCountX, i / tileCountX) = geometryInstance;
        }
    }
}

std::pair<vsg::ref_ptr<vsg::TopLevelAccelerationStructure>, vsg::ref_ptr<vsg::Node>> TerrainAccelerationStructureManager::createTlasAndScene(vsg::dvec3 eyePos, int lodViewDistance, bool maxLod = false)
{

    double scaleModifier = terrainScale * 20.0;
    if (tileLengthLodFactor > 0) {
        scaleModifier *= (1L << tileLengthLodFactor);
    }
    else {
        scaleModifier /= (1L << -tileLengthLodFactor);
    }

    auto eyePosInTileCoords = eyePos / scaleModifier;
    eyePosInTileCoords.y *= -1;

    auto tlas = vsg::TopLevelAccelerationStructure::create(device);

    auto sceneRoot = vsg::MatrixTransform::create();
    sceneRoot->matrix = vsg::mat4();

    auto scenegraph = vsg::StateGroup::create();

    for (int y = 0; y < tileCountY; ++y) {
        for (int x = 0; x < tileCountX; ++x) {
            int lod;
            if (maxLod) {
                lod = lodLevelCount - 1;
            } else {
                vsg::dvec3 tilePos(x, y, 0);
                tilePos += vsg::dvec3(0.5, 0.5, 0.0);
                double distance = vsg::length(tilePos - eyePosInTileCoords);
                distance /= lodViewDistance;
                lod = lodLevelCount - 1 - round(distance);
                if (lod < minLod) lod = minLod;
            }

            auto geometryInstance = blasTiles->at(lod)->at(x, y);
            tlas->geometryInstances.push_back(geometryInstance);

            auto tileNode = nodeTiles->at(lod)->at(x, y);
            scenegraph->addChild(tileNode);
        }
    }

    sceneRoot->addChild(scenegraph);

    return { tlas, sceneRoot };
}

std::vector < vsg::ref_ptr<vsg::TopLevelAccelerationStructure>> TerrainAccelerationStructureManager::buildAllLodTlas(vsg::ref_ptr<vsg::Context> context)
{
    context->buildAccelerationStructureCommands.clear();

    std::vector<vsg::ref_ptr<vsg::TopLevelAccelerationStructure>> tlasTestVector;
    for (int currentLod = minLod; currentLod < lodLevelCount; ++currentLod) {
        std::cout << "Building TLAS LOD " << currentLod << std::endl;

        auto tlasTest = vsg::TopLevelAccelerationStructure::create(context->device);
        for (uint32_t y = 0; y < tileCountY; ++y) {
            for (uint32_t x = 0; x < tileCountX; ++x) {
                auto geometryInstance = blasTiles->at(currentLod)->at(x, y);
                tlasTest->geometryInstances.push_back(geometryInstance);
            }
        }

        tlasTest->compile(*context);
        tlasTestVector.push_back(tlasTest);
    }

    context->record();
    context->waitForCompletion();

    return tlasTestVector;
}

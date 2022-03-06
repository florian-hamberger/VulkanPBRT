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

int TerrainAccelerationStructureManager::getTileLength(int lod) {
    return (1L << (lod + tileLengthLodFactor)) + 1;
}

int TerrainAccelerationStructureManager::getTileArrayIndex(int x, int y, int lod) {
    return y * getTileLength(lod) + x;
}

int TerrainAccelerationStructureManager::calculateLod(int x, int y, vsg::dvec3 eyePosInTileCoords, int lodViewDistance, bool maxLod)
{
    if (maxLod) {
        return lodLevelCount - 1;
    }
    else {
        vsg::dvec3 tilePos(x, y, 0);
        tilePos += vsg::dvec3(0.5, 0.5, 0.0);
        //eyePosInTileCoords.z = 0.0;
        double distance = vsg::length(tilePos - eyePosInTileCoords);
        int lodDecrease = log2((distance / lodViewDistance) + 1);
        int lod = lodLevelCount - 1 - lodDecrease;
        if (lod < minLod) {
            return minLod;
        } else {
            return lod;
        }
    }
}

float TerrainAccelerationStructureManager::getVertexZPos(int x, int y, int lod, int tileX, int tileY) {
    auto geometryInstance = blasTiles->at(lod)->at(x, y);
    auto verticesData = geometryInstance->accelerationStructure->geometries[0]->vertsOriginal;
    auto vertices = vsg::ref_ptr<vsg::vec3Array>(dynamic_cast<vsg::vec3Array*>(verticesData.get()));
    int index = getTileArrayIndex(tileX, tileY, lod);
    return vertices->at(index).z;
}

void TerrainAccelerationStructureManager::updateTile(int x, int y, int lod, vsg::dvec3 eyePosInTileCoords, int lodViewDistance, bool maxLod, vsg::ref_ptr<vsg::GeometryInstance> geometryInstance) {
    auto accelerationGeometry = geometryInstance->accelerationStructure->geometries[0];

    if (!accelerationGeometry->adjacentLods) {
        accelerationGeometry->adjacentLods = vsg::intArray2D::create(3, 3);
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                accelerationGeometry->adjacentLods->set(i, j, -1);
            }
        }
    }

    if (accelerationGeometry->vertsOriginal == accelerationGeometry->verts) {
        auto verticesOriginal = vsg::ref_ptr<vsg::vec3Array>(dynamic_cast<vsg::vec3Array*>(accelerationGeometry->vertsOriginal.get()));
        auto vertices = vsg::vec3Array::create(verticesOriginal->size());
        for (int i = 0; i < verticesOriginal->size(); ++i) {
            vertices->set(i, verticesOriginal->at(i));
        }

        auto verticesData = vsg::ref_ptr<vsg::Data>(dynamic_cast<vsg::Data*>(vertices.get()));
        accelerationGeometry->verts = verticesData;
    }

    bool geometryUpdateNecessary = false;
    for (int dy = -1; dy < 2; ++dy) {
        for (int dx = -1; dx < 2; ++dx) {
            if (dx == 0 && dy == 0) continue;

            int tx = x + dx;
            int ty = y + dy;
            if (tx < 0 || tx >= tileCountX || ty < 0 || ty >= tileCountY) continue;

            int tLod = calculateLod(tx, ty, eyePosInTileCoords, lodViewDistance, maxLod);
            if (accelerationGeometry->adjacentLods->at(dx + 1, dy + 1) != tLod) {
                accelerationGeometry->adjacentLods->set(dx + 1, dy + 1, tLod);
                geometryUpdateNecessary = true;
            }
        }
    }

    if (geometryUpdateNecessary) {
        bool geometryModified = false;
        for (int dy = -1; dy < 2; ++dy) {
            for (int dx = -1; dx < 2; ++dx) {
                if (dx == 0 && dy == 0) continue;

                int tx = x + dx;
                int ty = y + dy;
                if (tx < 0 || tx >= tileCountX || ty < 0 || ty >= tileCountY) continue;

                int tLod = calculateLod(tx, ty, eyePosInTileCoords, lodViewDistance, maxLod);
                if (tLod < lod) {

                    if (!geometryModified) {
                        auto verticesOriginal = vsg::ref_ptr<vsg::vec3Array>(dynamic_cast<vsg::vec3Array*>(accelerationGeometry->vertsOriginal.get()));
                        auto vertices = vsg::vec3Array::create(verticesOriginal->size());
                        for (int i = 0; i < verticesOriginal->size(); ++i) {
                            vertices->set(i, verticesOriginal->at(i));
                        }

                        auto verticesData = vsg::ref_ptr<vsg::Data>(dynamic_cast<vsg::Data*>(vertices.get()));
                        accelerationGeometry->verts = verticesData;
                    }
                    auto vertices = vsg::ref_ptr<vsg::vec3Array>(dynamic_cast<vsg::vec3Array*>(accelerationGeometry->verts.get()));

                    int tileLength = getTileLength(lod);
                    int stepX = -dy;
                    int stepY = dx;
                    int currentX = tileLength - 1;
                    int currentXAdjacent = tileLength / 2;
                    int currentY = tileLength - 1;
                    int currentYAdjacent = tileLength / 2;
                    if (dx < 0 || dx == 0 && dy == -1) {
                        currentX = 0;
                    }
                    if (dy < 0 || dx == 1 && dy == 0) {
                        currentY = 0;
                    }
                    if (dx == 1 || dx == 0 && dy == -1) {
                        currentXAdjacent = 0;
                    }
                    if (dy == 1 || dx == 1 && dy == 0) {
                        currentYAdjacent = 0;
                    }

                    for (int i = 0; i < tileLength; ++i) {
                        if (i % 2 == 0) {
                            float z = getVertexZPos(tx, ty, tLod, currentXAdjacent, currentYAdjacent);
                            int index = getTileArrayIndex(currentX, currentY, lod);
                            vertices->at(index).z = z;
                        }
                        else {
                            float z1 = getVertexZPos(tx, ty, tLod, currentXAdjacent, currentYAdjacent);
                            currentXAdjacent += stepX;
                            currentYAdjacent += stepY;
                            float z2 = getVertexZPos(tx, ty, tLod, currentXAdjacent, currentYAdjacent);
                            float z = (z1 + z2) / 2;
                            int index = getTileArrayIndex(currentX, currentY, lod);
                            vertices->at(index).z = z;
                        }
                        if (dx * dy != 0) break;
                        currentX += stepX;
                        currentY += stepY;
                    }

                    geometryInstance->accelerationStructure->_vkGeometries.clear();
                    accelerationGeometry->_geometry.geometry.triangles.vertexData.deviceAddress = VkDeviceAddress{ 0 };

                    accelerationGeometry->geometryModified = true;
                    geometryModified = true;
                }
            }
        }
        if (!geometryModified && accelerationGeometry->geometryModified) {
            auto verticesOriginal = vsg::ref_ptr<vsg::vec3Array>(dynamic_cast<vsg::vec3Array*>(accelerationGeometry->vertsOriginal.get()));
            auto vertices = vsg::ref_ptr<vsg::vec3Array>(dynamic_cast<vsg::vec3Array*>(accelerationGeometry->verts.get()));
            for (int i = 0; i < verticesOriginal->size(); ++i) {
                vertices->set(i, verticesOriginal->at(i));
            }

            geometryInstance->accelerationStructure->_vkGeometries.clear();
            accelerationGeometry->_geometry.geometry.triangles.vertexData.deviceAddress = VkDeviceAddress{ 0 };

            accelerationGeometry->geometryModified = false;
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

    int counter = 0;
    for (int y = 0; y < tileCountY; ++y) {
        for (int x = 0; x < tileCountX; ++x) {
            int lod = calculateLod(x, y, eyePosInTileCoords, lodViewDistance, maxLod);

            auto geometryInstance = blasTiles->at(lod)->at(x, y);
            updateTile(x, y, lod, eyePosInTileCoords, lodViewDistance, maxLod, geometryInstance);
            if (geometryInstance->accelerationStructure->_vkGeometries.empty()) {
                counter++;
            }
            tlas->geometryInstances.push_back(geometryInstance);

            auto tileNode = nodeTiles->at(lod)->at(x, y);
            auto xform = vsg::ref_ptr<vsg::MatrixTransform>(dynamic_cast<vsg::MatrixTransform*>(tileNode.get()));
            auto stateGroup = vsg::ref_ptr<vsg::StateGroup>(dynamic_cast<vsg::StateGroup*>(xform->children[0].get()));
            auto vid = vsg::ref_ptr<vsg::VertexIndexDraw>(dynamic_cast<vsg::VertexIndexDraw*>(stateGroup->children[0].get()));

            auto accelerationGeometry = geometryInstance->accelerationStructure->geometries[0];
            //auto verticesOriginal = vsg::ref_ptr<vsg::vec3Array>(dynamic_cast<vsg::vec3Array*>(accelerationGeometry->vertsOriginal.get()));

            vid->arrays[0]->data = accelerationGeometry->verts;

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

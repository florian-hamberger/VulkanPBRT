#include "TerrainImporter.hpp"

TerrainImporter::TerrainImporter(const vsg::Path& heightmapPath, const vsg::Path& texturePath, float terrainScale, float terrainScaleVertexHeight, bool terrainFormatLa2d, bool textureFormatS3tc, int heightmapLod, int textureLod) :
    heightmapPath(heightmapPath), texturePath(texturePath), terrainScale(terrainScale), terrainScaleVertexHeight(terrainScaleVertexHeight), terrainFormatLa2d(terrainFormatLa2d), textureFormatS3tc(textureFormatS3tc), heightmapLod(heightmapLod), textureLod(textureLod) {

}

vsg::ref_ptr<vsg::Node> TerrainImporter::importTerrain() {
    auto options = vsg::Options::create(vsgXchange::assimp::create(), vsgXchange::dds::create(), vsgXchange::stbi::create(), vsgXchange::openexr::create());

    if (terrainFormatLa2d) {
        int headerSize = 8192;

        std::cout << "importing heightmap la2d data...";

        vsg::Path heightmapFullPath = heightmapPath;
        if (heightmapLod >= 0) {
            heightmapFullPath.append("_L");
            if (heightmapLod < 10) {
                heightmapFullPath.append("0");
            }
            heightmapFullPath.append(std::to_string(heightmapLod));
            heightmapFullPath.append(".la2d");
        }

        heightmapIfs = std::ifstream(heightmapFullPath, std::ios::in | std::ios::binary);
        heightmapIfs.seekg(24);
        heightmapIfs.read(reinterpret_cast<char*>(&heightmapActualWidth), sizeof(heightmapActualWidth));
        heightmapIfs.read(reinterpret_cast<char*>(&heightmapActualHeight), sizeof(heightmapActualHeight));
        uint32_t heightmapTileWidth;
        uint32_t heightmapTileHeight;
        heightmapIfs.read(reinterpret_cast<char*>(&heightmapTileWidth), sizeof(heightmapTileWidth));
        heightmapIfs.read(reinterpret_cast<char*>(&heightmapTileHeight), sizeof(heightmapTileHeight));
        heightmapIfs.seekg(headerSize);

        heightmapFullWidth = ((heightmapActualWidth / heightmapTileWidth) + 1) * heightmapTileWidth;
        heightmapFullHeight = ((heightmapActualHeight / heightmapTileHeight) + 1) * heightmapTileHeight;

        heightmapLa2dBuffer = new float[heightmapFullWidth * heightmapFullHeight];

        for (int yOffset = 0; yOffset < heightmapFullHeight; yOffset += heightmapTileHeight) {
            for (int xOffset = 0; xOffset < heightmapFullWidth; xOffset += heightmapTileWidth) {
                for (int y = 0; y < heightmapTileHeight; y++) {
                    for (int x = 0; x < heightmapTileWidth; x++) {
                        float inputValue;
                        if (heightmapIfs.read(reinterpret_cast<char*>(&inputValue), sizeof(inputValue))) {
                            heightmapLa2dBuffer[heightmapFullWidth * (yOffset + y) + xOffset + x] = inputValue;
                        }
                        else {
                            std::cout << "error: could not read from file" << std::endl;
                        }
                    }
                }
            }
        }
        uint8_t inputValue;
        if (!heightmapIfs.read(reinterpret_cast<char*>(&inputValue), sizeof(inputValue))) {
            //std::cout << "end of file check" << std::endl;
            std::cout << "done" << std::endl;
        }
        else {
            std::cout << "error: end of file not yet reached?" << std::endl;
        }
        //std::cout << "heightmap la2d data imported" << std::endl;



        std::cout << "importing texture la2d data...";

        vsg::Path textureFullPath = texturePath;
        if (textureLod >= 0) {
            textureFullPath.append("_L");
            if (textureLod < 10) {
                textureFullPath.append("0");
            }
            textureFullPath.append(std::to_string(textureLod));
            if (textureFormatS3tc) {
                textureFullPath.append("_S3TC");
            }
            textureFullPath.append(".la2d");
        }

        textureIfs = std::ifstream(textureFullPath, std::ios::in | std::ios::binary);
        textureIfs.seekg(24);
        textureIfs.read(reinterpret_cast<char*>(&textureActualWidth), sizeof(textureActualWidth));
        textureIfs.read(reinterpret_cast<char*>(&textureActualHeight), sizeof(textureActualHeight));
        uint32_t textureTileWidth;
        uint32_t textureTileHeight;
        textureIfs.read(reinterpret_cast<char*>(&textureTileWidth), sizeof(textureTileWidth));
        textureIfs.read(reinterpret_cast<char*>(&textureTileHeight), sizeof(textureTileHeight));
        textureIfs.seekg(headerSize);

        textureFullWidth = ((textureActualWidth / textureTileWidth) + 1) * textureTileWidth;
        textureFullHeight = ((textureActualHeight / textureTileHeight) + 1) * textureTileHeight;

        auto textureLa2dBufferS3tc = new uint8_t[0][8];
        auto textureLa2dBufferRgb = new uint8_t[0][3];

        int channelsToLoad;
        int componentsToLoad;

        if (textureFormatS3tc) {
            channelsToLoad = 8;
            //componentsToLoad = width / 4 * height / 4;
            componentsToLoad = textureFullWidth * textureFullHeight;
            textureLa2dBufferS3tc = new uint8_t[textureFullWidth * textureFullHeight * 16][8];
        } else {
            channelsToLoad = 3;
            componentsToLoad = textureFullWidth * textureFullHeight;
            textureLa2dBufferRgb = new uint8_t[textureFullWidth * textureFullHeight][3];
        }

        for (int yOffset = 0; yOffset < textureFullHeight; yOffset += textureTileHeight) {
            for (int xOffset = 0; xOffset < textureFullWidth; xOffset += textureTileWidth) {
                for (int y = 0; y < textureTileHeight; y++) {
                    for (int x = 0; x < textureTileWidth; x++) {
                        for (int channel = 0; channel < channelsToLoad; channel++) {
                            if (textureIfs.read(reinterpret_cast<char*>(&inputValue), sizeof(inputValue))) {
                                if (textureFormatS3tc) {
                                    textureLa2dBufferS3tc[textureFullWidth * (yOffset + y) + xOffset + x][channel] = inputValue;
                                }
                                else {
                                    textureLa2dBufferRgb[textureFullWidth * (yOffset + y) + xOffset + x][channel] = inputValue;
                                }
                            }
                            else {
                                std::cout << "error: could not read from file" << std::endl;
                            }
                        }
                    }
                }
            }
        }

        if (!textureIfs.read(reinterpret_cast<char*>(&inputValue), sizeof(inputValue))) {
            //std::cout << "end of file check" << std::endl;
            std::cout << "done" << std::endl;
        }
        else {
            std::cout << "error: end of file not yet reached?" << std::endl;
        }
        //std::cout << "texture la2d data imported" << std::endl;

        VkFormat textureFormat;
        if (textureFormatS3tc) {
            textureFormat = VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        }
        else {
            //textureFormat = VK_FORMAT_R8G8B8A8_UNORM;
            textureFormat = VK_FORMAT_R8G8B8_UNORM;
        }

        if (textureFormatS3tc) {
            auto textureArray2D = vsg::Array2D<uint8_t[8]>::create(textureFullWidth * 4, textureFullHeight * 4, textureLa2dBufferS3tc, vsg::Data::Layout{ textureFormat });
            texture = vsg::ref_ptr<vsg::Data>(dynamic_cast<vsg::Data*>(textureArray2D.get()));
        } else {
            auto textureArray2D = vsg::Array2D<uint8_t[3]>::create(textureFullWidth, textureFullHeight, textureLa2dBufferRgb, vsg::Data::Layout{ textureFormat });
            texture = vsg::ref_ptr<vsg::Data>(dynamic_cast<vsg::Data*>(textureArray2D.get()));
        }

    } else {
        auto heightmapData = vsg::read_cast<vsg::Data>(heightmapPath, options);
        if (!heightmapData.valid()) {
            std::cout << "error loading" << std::endl;
        }

        heightmap = heightmapData.cast<vsg::ubvec4Array2D>();
        if (!heightmap) {
            std::cout << "wrong format" << std::endl;
        }

        heightmapFullWidth = heightmap->width();
        heightmapFullHeight = heightmap->height();

        texture = vsg::read_cast<vsg::Data>(texturePath, options);
        if (!texture.valid()) {
            std::cout << "error loading" << std::endl;
        }
    }

    std::cout << "creating geometry...";
    auto terrain = createGeometry();
    std::cout << "done" << std::endl;
    return terrain;
}

vsg::vec3 TerrainImporter::getHeightmapVertexPosition(int x, int y) {
    //std::cout << "getHeightmapVertexPosition(" << x << ", " << y << ")" << std::endl;
    float heightmapValue;
    vsg::vec3 scaleModifier(1.0f, 1.0f, 1.0f);
    if (terrainFormatLa2d) {
        heightmapValue = heightmapLa2dBuffer[heightmapFullWidth * y + x];
        scaleModifier.y *= heightmapActualWidth;
        scaleModifier /= heightmapActualWidth;

        scaleModifier.y *= 0.000019f;
    }
    else {
        heightmapValue = float(heightmap->data()[heightmap->index(x, y)].r);
        scaleModifier.y /= 256.0f; // normalize height to [0, 1)
        scaleModifier.y *= heightmapFullWidth;
        scaleModifier /= heightmapFullWidth;

        scaleModifier.y *= 0.02f;
    }
    scaleModifier.y *= terrainScaleVertexHeight;
    scaleModifier *= terrainScale * 10.0f;
    return vsg::vec3(float(x) * scaleModifier.x, heightmapValue * scaleModifier.y, float(y) * scaleModifier.z);
}

vsg::vec2 TerrainImporter::getTextureCoordinate(int x, int y) {
    //return vsg::vec2(float(x) / float(heightmapFullWidth), float(y) / float(heightmapFullHeight));
    float u = float(x) / float(heightmapActualWidth) * float(textureActualWidth) / float(textureFullWidth);
    float v = float(y) / float(heightmapActualHeight) * float(textureActualHeight) / float(textureFullHeight);
    return vsg::vec2(u, v);
}

//using code from vsgXchange/assimp/assimp.cpp
vsg::ref_ptr<vsg::Node> TerrainImporter::createGeometry()
{
    //auto pipelineLayout = _defaultPipeline->layout;

    //auto state = createTestMaterial();
    auto state = loadTextureMaterials();

    auto root = vsg::MatrixTransform::create();
    
    root->matrix = vsg::rotate(vsg::PI * 0.5, 1.0, 0.0, 0.0) * vsg::rotate(vsg::PI * 0.75, 0.0, 1.0, 0.0);

    auto scenegraph = vsg::StateGroup::create();
    //scenegraph->add(vsg::BindGraphicsPipeline::create(_defaultPipeline));
    //scenegraph->add(_defaultState);

    //auto rootNode = scene->mRootNode;

    //auto [node, parent] = nodes.top();

    //Matrix4x4 m = rootNode->mTransformation;
    vsg::mat4 m1 = createIdentityMatrix();
    //std::cout << "rootNode->mTransformation: " << mat4ToString(m1) << std::endl;
    //m1.Transpose();

    auto xform = vsg::MatrixTransform::create();
    xform->matrix = vsg::mat4((float*)&m1);
    scenegraph->addChild(xform);

    //auto node = rootNode->mChildren[0];
    auto parent = xform;

    //Matrix4x4 m = node->mTransformation;
    vsg::mat4 m2 = createIdentityMatrix();
    //std::cout << "node->mTransformation: " << mat4ToString(m2) << std::endl;
    //m2.Transpose();

    xform = vsg::MatrixTransform::create();
    xform->matrix = vsg::mat4((float*)&m2);
    parent->addChild(xform);

    int numPixels = heightmapFullWidth * heightmapFullHeight;

    int numMeshes = 1;
    for (int i = 0; i < numMeshes; ++i)
    {
        int mNumVertices = numPixels * 6;
        auto vertices = vsg::vec3Array::create(mNumVertices);
        auto normals = vsg::vec3Array::create(mNumVertices);
        auto texcoords = vsg::vec2Array::create(mNumVertices);
        std::vector<unsigned int> indices;


        int currentVertexIndex = 0;
        for (int y = 0; y < heightmapFullHeight-1; y++) {
            for (int x = 0; x < heightmapFullWidth-1; x++) {
                vertices->at(currentVertexIndex) = getHeightmapVertexPosition(x, y);
                texcoords->at(currentVertexIndex) = getTextureCoordinate(x, y);
                vertices->at(currentVertexIndex + 1) = getHeightmapVertexPosition(x, y + 1);
                texcoords->at(currentVertexIndex + 1) = getTextureCoordinate(x, y + 1);
                vertices->at(currentVertexIndex + 2) = getHeightmapVertexPosition(x + 1, y);
                texcoords->at(currentVertexIndex + 2) = getTextureCoordinate(x + 1, y);
                indices.push_back(currentVertexIndex);
                indices.push_back(currentVertexIndex + 1);
                indices.push_back(currentVertexIndex + 2);
                currentVertexIndex += 3;

                vertices->at(currentVertexIndex) = getHeightmapVertexPosition(x, y + 1);
                texcoords->at(currentVertexIndex) = getTextureCoordinate(x, y + 1);
                vertices->at(currentVertexIndex + 1) = getHeightmapVertexPosition(x + 1, y + 1);
                texcoords->at(currentVertexIndex + 1) = getTextureCoordinate(x + 1, y + 1);
                vertices->at(currentVertexIndex + 2) = getHeightmapVertexPosition(x + 1, y);
                texcoords->at(currentVertexIndex + 2) = getTextureCoordinate(x + 1, y);
                indices.push_back(currentVertexIndex);
                indices.push_back(currentVertexIndex + 1);
                indices.push_back(currentVertexIndex + 2);
                currentVertexIndex += 3;
            }
        }

        for (int j = 0; j < mNumVertices; ++j)
        {
            normals->at(j) = vsg::vec3(0, 0, 0);
            //texcoords->at(j) = vsg::vec2(0, 0);
        }

        vsg::ref_ptr<vsg::Data> vsg_indices;

        if (indices.size() < std::numeric_limits<uint16_t>::max())
        {
            auto myindices = vsg::ushortArray::create(static_cast<uint16_t>(indices.size()));
            std::copy(indices.begin(), indices.end(), myindices->data());
            vsg_indices = myindices;
        }
        else
        {
            auto myindices = vsg::uintArray::create(static_cast<uint32_t>(indices.size()));
            std::copy(indices.begin(), indices.end(), myindices->data());
            vsg_indices = myindices;
        }

        auto stategroup = vsg::StateGroup::create();
        xform->addChild(stategroup);

        stategroup->add(state.first);
        stategroup->add(state.second);

        auto vid = vsg::VertexIndexDraw::create();
        vid->assignArrays(vsg::DataList{vertices, normals, texcoords});
        vid->assignIndices(vsg_indices);
        vid->indexCount = indices.size();
        vid->instanceCount = 1;
        stategroup->addChild(vid);
    }

    root->addChild(scenegraph);

    return root;
}

//using code from vsgXchange/assimp/assimp.cpp
TerrainImporter::State TerrainImporter::loadTextureMaterials()
{

    auto getWrapMode = [](aiTextureMapMode mode) {
        switch (mode)
        {
        case aiTextureMapMode_Wrap: return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case aiTextureMapMode_Clamp: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case aiTextureMapMode_Decal: return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case aiTextureMapMode_Mirror: return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default: break;
        }
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    };

    auto getTexture = [&]() -> SamplerData {
        //std::array<aiTextureMapMode, 3> wrapMode{ {aiTextureMapMode_Wrap, aiTextureMapMode_Wrap, aiTextureMapMode_Wrap} };
        std::array<aiTextureMapMode, 3> wrapMode{ {aiTextureMapMode_Clamp, aiTextureMapMode_Clamp, aiTextureMapMode_Clamp} };

        SamplerData samplerImage;

        //std::string texPath;
        //if (texPath.data[0] == '*')
        //{
        //    const auto texIndex = std::atoi(texPath.C_Str() + 1);
        //    const auto texture = scene->mTextures[texIndex];

        //    //qCDebug(lc) << "Handle embedded texture" << texPath.C_Str() << texIndex << texture->achFormatHint << texture->mWidth << texture->mHeight;

        //    if (texture->mWidth > 0 && texture->mHeight == 0)
        //    {
        //        auto imageOptions = vsg::Options::create(*options);
        //        imageOptions->extensionHint = texture->achFormatHint;
        //        if (samplerImage.data = vsg::read_cast<vsg::Data>(reinterpret_cast<const uint8_t*>(texture->pcData), texture->mWidth, imageOptions); !samplerImage.data.valid())
        //            return {};
        //    }
        //}
        //else
        //{
            //const std::string filename = vsg::findFile(texPath, options);
            //const std::string filename = "C:/Users/Flori/Projects/TUM/7-WS21/BT/VulkanPBRT/out/build/x64-Debug/heightmaps/hm_65.png";

            //auto options = vsg::Options::create(vsgXchange::assimp::create(), vsgXchange::dds::create(), vsgXchange::stbi::create(), vsgXchange::openexr::create());
            samplerImage.data = texture;

            //if (samplerImage.data = vsg::read_cast<vsg::Data>(filename, options); !samplerImage.data.valid())
            //{
            //    //std::cerr << "Failed to load texture: " << filename << " texPath = " << texPath << std::endl;
            //    std::cerr << "Failed to load texture: " << filename << std::endl;
            //    return {};
            //}
        //}

        //defines.push_back("VSG_DIFFUSE_MAP");

        samplerImage.sampler = vsg::Sampler::create();

        samplerImage.sampler->addressModeU = getWrapMode(wrapMode[0]);
        samplerImage.sampler->addressModeV = getWrapMode(wrapMode[1]);
        samplerImage.sampler->addressModeW = getWrapMode(wrapMode[2]);

        samplerImage.sampler->anisotropyEnable = VK_TRUE;
        samplerImage.sampler->maxAnisotropy = 16.0f;
        //samplerImage.sampler->anisotropyEnable = VK_FALSE;
        //samplerImage.sampler->maxAnisotropy = 0.0f;

        samplerImage.sampler->maxLod = samplerImage.data->getLayout().maxNumMipmaps;

        if (samplerImage.sampler->maxLod <= 1.0)
        {
            //                if (texPath.length > 0)
            //                    std::cout << "Auto generating mipmaps for texture: " << scene.GetShortFilename(texPath.C_Str()) << std::endl;;

            // Calculate maximum lod level
            auto maxDim = std::max(samplerImage.data->width(), samplerImage.data->height());
            samplerImage.sampler->maxLod = std::floor(std::log2f(static_cast<float>(maxDim)));
        }

        return samplerImage;
    };

    //const auto material = scene->mMaterials[i];

    bool isPbrMaterial = true;
    if (isPbrMaterial)
    {
        // PBR path
        PbrMaterial pbr;

        //std::vector<std::string> defines;
        //bool isTwoSided{ true };

        bool hasPbrSpecularGlossiness = true;

        //pbr.baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        pbr.baseColorFactor = { 0.2f, 0.2f, 0.2f, 1.0f };
        pbr.emissiveFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        pbr.diffuseFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        //pbr.diffuseFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
        pbr.specularFactor = { 0.0f, 0.0f, 0.0f, 1.0f };
        //pbr.specularFactor = { 1.0f, 1.0f, 1.0f, 1.0f };

        pbr.metallicFactor = 1.0f;
        pbr.roughnessFactor = 1.0f;
        pbr.alphaMask = 1.0f;
        pbr.alphaMaskCutoff = 0.5f;
        pbr.indexOfRefraction = 1.0f;
        pbr.category_id = 0;

        if (hasPbrSpecularGlossiness)
        {
            //defines.push_back("VSG_WORKFLOW_SPECGLOSS");
            //material->Get(AI_MATKEY_COLOR_DIFFUSE, pbr.diffuseFactor);
            //material->Get(AI_MATKEY_COLOR_SPECULAR, pbr.specularFactor);

            //if (material->Get(AI_MATKEY_GLTF_PBRSPECULARGLOSSINESS_GLOSSINESS_FACTOR, pbr.specularFactor.a) != AI_SUCCESS)
            //{
            //    if (float shininess; material->Get(AI_MATKEY_SHININESS, shininess))
            //        pbr.specularFactor.a = shininess / 1000;
            //}
        }
        else
        {
            //material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, pbr.metallicFactor);
            //material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, pbr.roughnessFactor);
        }

        //material->Get(AI_MATKEY_COLOR_EMISSIVE, pbr.emissiveFactor);
        //material->Get(AI_MATKEY_GLTF_ALPHACUTOFF, pbr.alphaMaskCutoff);
        //material->Get(AI_MATKEY_REFRACTI, pbr.indexOfRefraction);

        //if (isTwoSided)
            //defines.push_back("VSG_TWOSIDED");

        vsg::DescriptorSetLayoutBindings descriptorBindings{
            {10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} };
        vsg::Descriptors descList;

        auto buffer = vsg::DescriptorBuffer::create(pbr.toData(), 10);
        descList.push_back(buffer);

        SamplerData samplerImage;
        if (samplerImage = getTexture(); samplerImage.data.valid())
        {
            auto diffuseTexture = vsg::DescriptorImage::create(samplerImage.sampler, samplerImage.data, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            descList.push_back(diffuseTexture);
            descriptorBindings.push_back({ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr });
        }

        auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);
        auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, descList);

        /*auto vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", processGLSLShaderSource(assimp_vertex, defines));
        auto fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", processGLSLShaderSource(assimp_pbr, defines));*/

        //auto pipeline = createPipeline(vertexShader, fragmentShader, descriptorSetLayout, isTwoSided);
        vsg::ref_ptr<vsg::GraphicsPipeline> pipeline = vsg::GraphicsPipeline::create();
        vsg::ref_ptr<vsg::PipelineLayout> pipelineLayout = vsg::PipelineLayout::create();

        auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSet);

        return { vsg::BindGraphicsPipeline::create(pipeline), bindDescriptorSet };
    }
    else
    {
        // Phong shading
        Material mat;
        //std::vector<std::string> defines;


        mat.alphaMaskCutoff = 0.5f;
        mat.ambient = { 0.1f, 0.1f, 0.1f, 1.0f };
        mat.diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
        mat.emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
        mat.shininess = 0.0f;
        mat.specular = { 0.0f, 0.0f, 0.0f, 0.0f };
        mat.category_id = 0;

        if (mat.shininess < 0.01f)
        {
            mat.shininess = 0.0f;
            mat.specular = { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        //bool isTwoSided{ false };

        vsg::DescriptorSetLayoutBindings descriptorBindings{
            {10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} };
        vsg::Descriptors descList;

        SamplerData samplerImage;
        if (samplerImage = getTexture(); samplerImage.data.valid())
        {
            auto diffuseTexture = vsg::DescriptorImage::create(samplerImage.sampler, samplerImage.data, 0, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            descList.push_back(diffuseTexture);
            descriptorBindings.push_back({ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr });

            //if (diffuseResult != AI_SUCCESS)
            //    mat.diffuse = aiColor4D{ 1.0f, 1.0f, 1.0f, 1.0f };
        }

        auto buffer = vsg::DescriptorBuffer::create(mat.toData(), 10);
        descList.push_back(buffer);

        auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);
        //auto vertexShader = vsg::ShaderStage::create(VK_SHADER_STAGE_VERTEX_BIT, "main", processGLSLShaderSource(assimp_vertex, defines));
        //auto fragmentShader = vsg::ShaderStage::create(VK_SHADER_STAGE_FRAGMENT_BIT, "main", processGLSLShaderSource(assimp_phong, defines));

        //auto pipeline = createPipeline(vertexShader, fragmentShader, descriptorSetLayout, isTwoSided);
        vsg::ref_ptr<vsg::GraphicsPipeline> pipeline = vsg::GraphicsPipeline::create();
        vsg::ref_ptr<vsg::PipelineLayout> pipelineLayout = vsg::PipelineLayout::create();

        auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, descList);
        auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSet);

        return { vsg::BindGraphicsPipeline::create(pipeline), bindDescriptorSet };
    }
}

vsg::mat4 TerrainImporter::createIdentityMatrix() {
    vsg::mat4 m;
    for (int i = 0; i < m.rows(); i++) {
        for (int j = 0; j < m.columns(); j++) {
            if (i == j) {
                m[i][j] = 1.0f;
            }
            else {
                m[i][j] = 0.0f;
            }
        }
    }
    return m;
}

std::string TerrainImporter::mat4ToString(vsg::mat4 m) {
    std::string s;
    for (int i = 0; i < m.rows(); i++) {
        s += "\n  ";
        for (int j = 0; j < m.columns(); j++) {
            s += std::to_string(m[i][j]) + " ";
        }
    }
    return s;
}
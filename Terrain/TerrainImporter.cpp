#include "TerrainImporter.hpp"

TerrainImporter::TerrainImporter() {

}

vsg::ref_ptr<vsg::Node> TerrainImporter::importTerrain(const vsg::Path& filename) {
    auto options = vsg::Options::create(vsgXchange::assimp::create(), vsgXchange::dds::create(), vsgXchange::stbi::create(), vsgXchange::openexr::create());
    auto heightmapData = vsg::read_cast<vsg::Data>(filename, options);
    if (!heightmapData.valid()) {
        std::cout << "error loading" << std::endl;
    }

    vsg::ref_ptr<vsg::ubvec4Array2D> heightmap = heightmapData.cast<vsg::ubvec4Array2D>();
    if (!heightmap) {
        std::cout << "wrong format" << std::endl;
    }

    auto terrain = createGeometry(heightmap);
    return terrain;
}

vsg::vec3 TerrainImporter::getHeightmapVertexPosition(int u, int v, vsg::ref_ptr<vsg::ubvec4Array2D> heightmap) {
    float scale = 0.1f;
    float maxHeight = 10.0f;

    float heightmapValue = heightmap->data()[heightmap->index(u, v)].r;
    heightmapValue *= maxHeight / 256.0f;
    return vsg::vec3(u, heightmapValue, v) * scale;
}

//using code from vsgXchange/assimp/assimp.cpp
vsg::ref_ptr<vsg::Node> TerrainImporter::createGeometry(vsg::ref_ptr<vsg::ubvec4Array2D> heightmap)
{
    //auto pipelineLayout = _defaultPipeline->layout;

    auto state = createTestMaterial();

    auto root = vsg::MatrixTransform::create();

    root->setMatrix(vsg::rotate(vsg::PI * 0.5, 1.0, 0.0, 0.0));

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
    xform->setMatrix(vsg::mat4((float*)&m1));
    scenegraph->addChild(xform);

    //auto node = rootNode->mChildren[0];
    auto parent = xform;

    //Matrix4x4 m = node->mTransformation;
    vsg::mat4 m2 = createIdentityMatrix();
    //std::cout << "node->mTransformation: " << mat4ToString(m2) << std::endl;
    //m2.Transpose();

    xform = vsg::MatrixTransform::create();
    xform->setMatrix(vsg::mat4((float*)&m2));
    parent->addChild(xform);

    int width = heightmap->width();
    int height = heightmap->height();
    int numPixels = width * height;

    int numMeshes = 1;
    for (int i = 0; i < numMeshes; ++i)
    {
        int mNumVertices = numPixels * 6;
        auto vertices = vsg::vec3Array::create(mNumVertices);
        auto normals = vsg::vec3Array::create(mNumVertices);
        auto texcoords = vsg::vec2Array::create(mNumVertices);
        std::vector<unsigned int> indices;


        int currentVertexIndex = 0;
        for (int v = 0; v < height-1; v++) {
            for (int u = 0; u < width-1; u++) {
                vertices->at(currentVertexIndex) = getHeightmapVertexPosition(u, v, heightmap);
                vertices->at(currentVertexIndex + 1) = getHeightmapVertexPosition(u, v + 1, heightmap);
                vertices->at(currentVertexIndex + 2) = getHeightmapVertexPosition(u + 1, v, heightmap);
                indices.push_back(currentVertexIndex);
                indices.push_back(currentVertexIndex + 1);
                indices.push_back(currentVertexIndex + 2);
                currentVertexIndex += 3;

                vertices->at(currentVertexIndex) = getHeightmapVertexPosition(u, v + 1, heightmap);
                vertices->at(currentVertexIndex + 1) = getHeightmapVertexPosition(u + 1, v + 1, heightmap);
                vertices->at(currentVertexIndex + 2) = getHeightmapVertexPosition(u + 1, v, heightmap);
                indices.push_back(currentVertexIndex);
                indices.push_back(currentVertexIndex + 1);
                indices.push_back(currentVertexIndex + 2);
                currentVertexIndex += 3;
            }
        }

        for (int j = 0; j < mNumVertices; ++j)
        {
            normals->at(j) = vsg::vec3(0, 0, 0);
            texcoords->at(j) = vsg::vec2(0, 0);
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

        //stategroup->add(state.first);
        stategroup->add(state.second);

        auto vid = vsg::VertexIndexDraw::create();
        vid->arrays = vsg::DataList{ vertices, normals, texcoords };
        vid->indices = vsg_indices;
        vid->indexCount = indices.size();
        vid->instanceCount = 1;
        stategroup->addChild(vid);
    }

    root->addChild(scenegraph);

    return root;
}

//using code from vsgXchange/assimp/assimp.cpp
TerrainImporter::State TerrainImporter::createTestMaterial()
{
    
    Material mat;

    mat.alphaMaskCutoff = 0.5f;
    mat.ambient = { 1.0f, 1.0f, 1.0f, 1.0f };
    mat.diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
    mat.emissive = { 0.0f, 0.0f, 0.0f, 0.0f };
    mat.specular = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (mat.shininess < 0.01f)
    {
        mat.shininess = 0.0f;
        mat.specular = {0.0f, 0.0f, 0.0f, 0.0f};
    }

    vsg::DescriptorSetLayoutBindings descriptorBindings{
        {10, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr} };
    vsg::Descriptors descList;

    auto buffer = vsg::DescriptorBuffer::create(mat.toData(), 10);
    descList.push_back(buffer);

    auto descriptorSetLayout = vsg::DescriptorSetLayout::create(descriptorBindings);
    vsg::ref_ptr<vsg::GraphicsPipeline> pipeline = vsg::GraphicsPipeline::create();

    auto descriptorSet = vsg::DescriptorSet::create(descriptorSetLayout, descList);
    auto bindDescriptorSet = vsg::BindDescriptorSet::create(VK_PIPELINE_BIND_POINT_GRAPHICS, vsg::PipelineLayout::create(), 0, descriptorSet);

    return { vsg::BindGraphicsPipeline::create(pipeline), bindDescriptorSet };
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
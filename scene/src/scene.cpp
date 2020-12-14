
#include <imgui.h>

#include <common/maths.hpp>

#include "scene_impl.hpp"

#include <tiny_obj_loader.h>
#include <stb_image.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "common\maths.hpp"

#include <iostream>
#include <algorithm>

int loadTexture(std::vector<Texture>& textures, const char* filePath)
{
    std::vector<Texture>::iterator it = std::find_if(textures.begin(), textures.end(),
                                        [filePath](Texture& t)
                                        { return t.fileName.compare(filePath) == 0; });
    
    if (it != textures.end())
        return it - textures.begin();

    Texture texture;
    texture.fileName = filePath;
    texture.data = stbi_loadf(filePath, &texture.width, &texture.height, nullptr, STBI_rgb_alpha);

    if (!texture.data)
        return -1;

    textures.push_back(texture);

    return textures.size() - 1;
}

int loadMaterial(std::vector<Material>& materials, float ambient[3], float diffuse[3], float specular[3], float emissive[3], float shininess)
{
    Material mat;
    mat.ambientColor = float4(ambient[0], ambient[1], ambient[2], 1.f);
    mat.diffuseColor = float4(diffuse[0], diffuse[1], diffuse[2], 1.f);
    mat.specularColor = float4(specular[0], specular[1], specular[2], 1.f);
    mat.emissionColor = float4(emissive[0], emissive[1], emissive[2], 0.f);
    mat.shininess = shininess;

    std::vector<Material>::iterator it = std::find_if(materials.begin(), materials.end(),
                                        [mat](const Material& m)
                                        {
                                            return m.shininess == mat.shininess &&
                                                   m.ambientColor == mat.ambientColor &&
                                                   m.diffuseColor == mat.diffuseColor &&
                                                   m.specularColor == mat.specularColor &&
                                                   m.emissionColor == mat.emissionColor;
                                        });

    if (it != materials.end())
        return it - materials.begin();

    materials.push_back(mat);

    return materials.size() - 1;
}

bool loadObject(Object& object, std::vector<Texture>& scnTextures, std::vector<Material>& scnMaterials, std::string filePath, std::string mtlBasedir, float scale = 1.f)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), mtlBasedir.c_str());

    if (!warn.empty()) {
        std::cerr << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        return 0;
    }

    if (!materials.empty())
    {
        for (size_t m = 0; m < materials.size(); m++)
        {
            tinyobj::material_t& mat = materials[m];

            Mesh mesh;
            mesh.textureIndex = loadTexture(scnTextures, (mtlBasedir + mat.diffuse_texname).c_str());
            if (mesh.textureIndex == -1)
                std::cout << mtlBasedir + mat.diffuse_texname << std::endl;
            mesh.materialIndex = loadMaterial(scnMaterials, mat.ambient, mat.diffuse, mat.specular, mat.emission, mat.shininess);
            object.mesh.push_back(mesh);
        }
    }
    else
    {
        Mesh mesh;
        mesh.textureIndex = -1;
        mesh.materialIndex = 0;
        object.mesh.push_back(mesh);
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            int meshIndex = std::max(0,  shapes[s].mesh.material_ids[f]);

            int fv = shapes[s].mesh.num_face_vertices[f];

            Triangle face;

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                rdrVertex vertice;
                vertice.x = attrib.vertices[3 * idx.vertex_index + 0] * scale;
                vertice.y = attrib.vertices[3 * idx.vertex_index + 1] * scale;
                vertice.z = attrib.vertices[3 * idx.vertex_index + 2] * scale;

                if (!attrib.normals.empty())
                {
                    vertice.nx = attrib.normals[3 * idx.normal_index + 0];
                    vertice.ny = attrib.normals[3 * idx.normal_index + 1];
                    vertice.nz = attrib.normals[3 * idx.normal_index + 2];
                }

                if (!attrib.colors.empty())
                {
                  
                    vertice.r = attrib.colors[3 * idx.vertex_index + 0];
                    vertice.g = attrib.colors[3 * idx.vertex_index + 1];
                    vertice.b = attrib.colors[3 * idx.vertex_index + 2];
                    vertice.a = attrib.colors.size() == 4 ? attrib.colors[3 * idx.vertex_index + 3] : 1.f;
                }

                if (!attrib.texcoords.empty() && idx.texcoord_index > 0)
                {
                    vertice.u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertice.v = attrib.texcoords[2 * idx.texcoord_index + 1];
                }

                face.vertices.push_back(vertice);
            }

            index_offset += fv;

            object.mesh[meshIndex].faces.push_back(face);
        }
    }

    return 1;
}

void loadQuad(Object& object, int textureIndex = -1, int materialIndex = 0, int hRes = 1, int vRes = 1)
{
    Mesh mesh;
    mesh.textureIndex = textureIndex;
    mesh.materialIndex = materialIndex;

    float hGrad = 1.f / (float)hRes;
    float vGrad = 1.f / (float)vRes;
    for (int i = 0; i < hRes; i++)
    {
        float u0 = i * hGrad;
        float u1 = u0 + hGrad;

        for (int j = 0; j < vRes; j++)
        {
            float v0 = j * vGrad;
            float v1 = v0 + vGrad;

            //                                      pos                   normal                    color                  uv
            Triangle face1;
            face1.vertices.push_back({ u0 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u0, v0 });
            face1.vertices.push_back({ u1 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u1, v0 });
            face1.vertices.push_back({ u1 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u1, v1 });
            mesh.faces.push_back(face1);

            Triangle face2;
            face2.vertices.push_back({ u1 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u1, v1 });
            face2.vertices.push_back({ u0 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u0, v1 });
            face2.vertices.push_back({ u0 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u0, v0 });
            mesh.faces.push_back(face2);
        }
    }
    object.mesh.push_back(mesh);
}

void loadTriangle(Object& object, int textureIndex = -1, int materialIndex = 0)
{
    Mesh mesh;
    mesh.textureIndex = textureIndex;
    mesh.materialIndex = materialIndex;

    Triangle face;

    //                          pos                   normal                  color                     uv
    face.vertices.push_back({-0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
    face.vertices.push_back({ 0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     0.5f, 0.5f });
    face.vertices.push_back({ 0.0f,  0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f });

    mesh.faces.push_back(face);
    object.mesh.push_back(mesh);
}

scnImpl* scnCreate()
{
    return new scnImpl();
}

void scnDestroy(scnImpl* scene)
{
    delete scene;
}

void scnSetCameraPosition(scnImpl* scene, float* cameraPos)
{
    memcpy(scene->cameraPos.e, cameraPos, sizeof(float3));
}

void scnUpdate(scnImpl* scene, float deltaTime, rdrImpl* renderer)
{
    scene->update(deltaTime, renderer);
}

void scnSetImGuiContext(scnImpl* scene, struct ImGuiContext* context)
{
    ImGui::SetCurrentContext(context);
}

void scnShowImGuiControls(scnImpl* scene)
{
    scene->showImGuiControls();
}

scnImpl::scnImpl()
{
    stbi_set_flip_vertically_on_load(1);

    // HERE: Load the scene
    // Setup some vertices to test
    loadTexture(textures, "assets/inputbilinear.png");
    loadTexture(textures, "assets/Deathclaw.png");

    lights[0].lightPos = { 0.f, 5.f, 0.f, 1.f };

    Object obj1({0.f, 0.f, -2.f});
    loadQuad(obj1, loadTexture(textures, "assets/inputbilinear.png"));
    objects.push_back(obj1);

    Object obj2({ 0.f, 0.f, -2.f }, { 0.f, M_PI, 0.f });
    //loadQuad(obj2, &textures, "assets/Deathclaw.png");
    //if (loadObject(obj2.vertices, "assets/sphere.obj", "assets", 0.5f))
    //objects.push_back(obj2);
    
    Object obj3;
    //loadObject(obj3, textures, materials, "assets/deathclaw.obj", "assets/", 0.5f);
    objects.push_back(obj3);

    Object obj4;
    loadObject(obj4, textures, materials, "assets/sponza-master/sponza.obj", "assets/sponza-master/", 0.005f);
    //loadObject(obj4, textures, materials, "assets/sphere.obj", "assets/");
    //loadObject(obj4.faces, textures, "assets/suzanne.obj", "assets/");
    objects.push_back(obj4);
}

scnImpl::~scnImpl()
{
    for (Texture& texture : textures)
        stbi_image_free(texture.data);
    // HERE: Unload the scene
}

void editLights(rdrImpl* renderer, scnImpl* scene)
{
    if (!scene || !renderer)
        return;

    static int selectedLight = 0;
    if (ImGui::TreeNode("Lights"))
    {
        ImGui::SliderInt("Selected light", &selectedLight, 0, IM_ARRAYSIZE(scene->lights) - 1);
        ImGui::Checkbox("Is light enable", &scene->lights[selectedLight].isEnable);

        ImGui::SliderFloat4("Light position", scene->lights[selectedLight].lightPos.e, -20.f, 20.f);

        bool isPoint = scene->lights[selectedLight].lightPos.w == 1.f;
        if (ImGui::Checkbox("Is point light", (bool*)&isPoint));
            scene->lights[selectedLight].lightPos.w = isPoint;

        ImGui::ColorEdit4("Ambient", scene->lights[selectedLight].ambient.e);
        ImGui::ColorEdit4("Diffuse", scene->lights[selectedLight].diffuse.e);
        ImGui::ColorEdit4("Specular", scene->lights[selectedLight].specular.e);

        ImGui::SliderFloat("Constant attenuation", &scene->lights[selectedLight].constantAttenuation, 0.f, 10.f);
        ImGui::SliderFloat("Linear attenuation", &scene->lights[selectedLight].linearAttenuation, 0.f, 10.f);
        ImGui::SliderFloat("Quadratic attenuation", &scene->lights[selectedLight].quadraticAttenuation, 0.f, 10.f);

        rdrSetUniformLight(renderer, selectedLight, (rdrLight*)&scene->lights[selectedLight]);

        ImGui::TreePop();
    }
}

void editMaterials(scnImpl* scene)
{
    static int selectedMaterial = 0;
    if (ImGui::TreeNode("Materials"))
    {
        ImGui::SliderInt("Selected material", &selectedMaterial, 0, scene->materials.size() - 1);

        ImGui::ColorEdit3("Ambient", scene->materials[selectedMaterial].ambientColor.e);
        ImGui::ColorEdit3("Diffuse", scene->materials[selectedMaterial].diffuseColor.e);
        ImGui::ColorEdit3("Specular", scene->materials[selectedMaterial].specularColor.e);
        ImGui::ColorEdit3("Emission", scene->materials[selectedMaterial].emissionColor.e);

        ImGui::DragFloat("shininess", &scene->materials[selectedMaterial].shininess, 0.f);
    }
}

void editObjects(scnImpl* scene)
{
    if (!scene)
        return;

    static int selectedObject = 0;
    if (ImGui::TreeNode("Objects"))
    {
        ImGui::SliderInt("Selected object", &selectedObject, 0, scene->objects.size() - 1);

        ImGui::Checkbox("Is object enable", &scene->objects[selectedObject].isEnable);

        ImGui::SliderFloat4("Object position", scene->objects[selectedObject].position.e, -10.f, 10.f);
        ImGui::SliderFloat4("Rotation", scene->objects[selectedObject].rotation.e, -10.f, 10.f);
        ImGui::SliderFloat4("Scale", scene->objects[selectedObject].scale.e, -10.f, 10.f);

        if (!scene->objects[selectedObject].mesh.empty())
        {
            static int selectedMesh = 0;
            ImGui::SliderInt("Selected mesh", &selectedMesh, 0, scene->objects[selectedObject].mesh.size() - 1);
            ImGui::SliderInt("Texture index", &scene->objects[selectedObject].mesh[selectedMesh].textureIndex, -1, scene->textures.size() - 1);
        }
        ImGui::TreePop();
    }
}

void scnImpl::drawObject(Object object, rdrImpl* renderer)
{
    if (!object.isEnable)
        return;

    // Get the model matrix of the current object
    rdrSetModel(renderer, object.getModel().e);

    // Then draw all his mesh
    for (const Mesh& mesh : object.mesh)
    {
        if (mesh.materialIndex >= 0)
            rdrSetUniformMaterial(renderer, (rdrMaterial*)&materials[mesh.materialIndex]);

        if (mesh.textureIndex >= 0 &&
            textures[mesh.textureIndex].data &&
            textures[mesh.textureIndex].height > 0 &&
            textures[mesh.textureIndex].width > 0)
            rdrSetTexture(renderer, textures[mesh.textureIndex].data, textures[mesh.textureIndex].width, textures[mesh.textureIndex].height);
        else
            rdrSetTexture(renderer, nullptr, 0, 0);

        // Then draw all his triangle
        for (const Triangle& face : mesh.faces)
            rdrDrawTriangles(renderer, face.vertices.data(), (int)face.vertices.size());
    }
}

void scnImpl::update(float deltaTime, rdrImpl* renderer)
{
    rdrSetUniformBool(renderer, UT_STENCTILTEST, false);

    editLights(renderer, this);

    time += deltaTime;

    // Shoud use pos - camera pos
    // Sort all objects with their distance
    const float3& camPos = cameraPos;
    std::vector<Object> sortedObjects = objects;
    sort(sortedObjects.begin(), sortedObjects.end(),
    [camPos](const Object& a, const Object& b)
    {
        float3 aPos = (a.getModel() * float4 { 0.f, 0.f, 0.f, 1.f }).xyz;
        float3 bPos = (b.getModel() * float4 { 0.f, 0.f, 0.f, 1.f }).xyz;
        return sqMagnitude(camPos - aPos) > sqMagnitude(camPos - bPos);
    });
    
    // Draw all objects
    for (const auto& object : sortedObjects)
        drawObject(object, renderer);
}

void scnImpl::showImGuiControls()
{
    editObjects(this);
    editMaterials(this);
}

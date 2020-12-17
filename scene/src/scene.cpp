
#include <imgui.h>

#include <common/maths.hpp>

#include "scene_impl.hpp"

#include <tiny_obj_loader.h>
#include <stb_image.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>
#include <algorithm>

int scnImpl::loadTexture(const char* filePath)
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

int scnImpl::loadMaterial(float ambient[3], float diffuse[3], float specular[3], float emissive[3], float shininess)
{
    Material mat;
    mat.ambientColor  = float4(ambient[0],  ambient[1],  ambient[2], 1.f);
    mat.diffuseColor  = float4(diffuse[0],  diffuse[1],  diffuse[2], 1.f);
    mat.specularColor = float4(specular[0], specular[1], specular[2], 1.f);
    mat.emissionColor = float4(emissive[0], emissive[1], emissive[2], 0.f);
    mat.shininess = shininess;

    std::vector<Material>::iterator it = std::find_if(materials.begin(), materials.end(),
                                        [mat](const Material& m)
                                        {
                                            return m.shininess     == mat.shininess     &&
                                                   m.ambientColor  == mat.ambientColor  &&
                                                   m.diffuseColor  == mat.diffuseColor  &&
                                                   m.specularColor == mat.specularColor &&
                                                   m.emissionColor == mat.emissionColor;
                                        });

    if (it != materials.end())
        return it - materials.begin();

    materials.push_back(mat);

    return materials.size() - 1;
}

bool scnImpl::loadObject(Object& object, std::string filePath, std::string mtlBasedir, float scale)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.c_str(), mtlBasedir.c_str(), true);

    if (!warn.empty())
        std::cerr << warn << std::endl;

    if (!err.empty())
        std::cerr << err << std::endl;

    if (!ret) return 0;

    if (!materials.empty())
    {
        // For each material of the .obj create a mesh with a texture and a material
        for (size_t m = 0; m < materials.size(); m++)
        {
            tinyobj::material_t& mat = materials[m];

            int textureIndex  = loadTexture((mtlBasedir + mat.diffuse_texname).c_str());
            int materialIndex = loadMaterial(mat.ambient, mat.diffuse, mat.specular, mat.emission, mat.shininess);

            object.mesh.push_back(Mesh(textureIndex, materialIndex));
        }
    }
    // If there is no material, add only one mesh, without material nor texture
    else
        object.mesh.push_back(Mesh(-1, 0));

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            // Get current mesh with material id (because the number of mesh is the number of material)
            size_t meshIndex = shapes[s].mesh.material_ids[f];

            size_t fv = shapes[s].mesh.num_face_vertices[f];
                 
            if (attrib.vertices.empty())
                continue;

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
                else
                    vertice.u = vertice.v = 0.f;

                face.vertices[v] = vertice;
            }

            index_offset += fv;

            // Check if there is not twice the same vertex for the current triangle (to avoid triangle with 0 as area)
            bool isAccepted = true;
            for (int i = 0; i < 3; i++)
            {
                int nextIndex = (i + 1) % 3;
                if (face.vertices[i].x == face.vertices[nextIndex].x &&
                    face.vertices[i].y == face.vertices[nextIndex].y &&
                    face.vertices[i].z == face.vertices[nextIndex].z)
                {
                    isAccepted = false;
                    break;
                }
            }

            if (isAccepted)
                object.mesh[meshIndex].faces.push_back(face);
        }
    }

    return 1;
}

void scnImpl::loadQuad(Object& object, int textureIndex, int materialIndex, int hRes, int vRes)
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

            Triangle face1, face2;

            //                                pos                       normal                  color                  uv
            face1.vertices[0] = { u0 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,     u0, v0 };
            face1.vertices[1] = { u1 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,     u1, v0 };
            face1.vertices[2] = { u1 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,     u1, v1 };
            mesh.faces.push_back(face1);

            face2.vertices[0] = { u1 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,     u1, v1 };
            face2.vertices[1] = { u0 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,     u0, v1 };
            face2.vertices[2] = { u0 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.0f,     u0, v0 };
            mesh.faces.push_back(face2);
        }
    }
    object.mesh.push_back(mesh);
}

void scnImpl::loadTriangle(Object& object, int textureIndex, int materialIndex)
{
    Mesh mesh;
    mesh.textureIndex = textureIndex;
    mesh.materialIndex = materialIndex;

    Triangle face;

    //                          pos                   normal                  color                     uv
    face.vertices[0] = {-0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f };
    face.vertices[1] = { 0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     0.5f, 0.5f };
    face.vertices[2] = { 0.0f,  0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f };

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
    loadTexture("assets/inputbilinear.png");
    loadTexture("assets/Deathclaw.png");

    lights[0].lightPos = { 0.f, 5.f, 0.f, 1.f };

    Object obj1({0.f, 0.f, -2.f});
    loadQuad(obj1, loadTexture("assets/inputbilinear.png"));
    objects.push_back(obj1);

    Object obj2;
    loadObject(obj2, "assets/deathclaw.obj", "assets/", 0.005f);
    objects.push_back(obj2);

    Object obj3;
    //loadObject(obj3, "assets/the_noble_craftsman.obj", "assets/");
    //loadObject(obj3, "assets/sponza-master/sponza.obj", "assets/sponza-master/", 0.005f);
    //objects.push_back(obj3);
}

// Unload the scene
scnImpl::~scnImpl()
{
    // Unload each texture
    for (Texture& texture : textures)
        stbi_image_free(texture.data);
}

void editLights(rdrImpl* renderer, scnImpl* scene)
{
    static int selectedLight = 0;
    if (ImGui::TreeNode("Lights"))
    {
        ImGui::SliderInt("Selected light", &selectedLight, 0, IM_ARRAYSIZE(scene->lights) - 1);
        ImGui::Checkbox("Is light enable", &scene->lights[selectedLight].isEnable);

        ImGui::SliderFloat4("Light position", scene->lights[selectedLight].lightPos.e, -20.f, 20.f);

        bool isPoint = scene->lights[selectedLight].lightPos.w == 1.f;
        if (ImGui::Checkbox("Is point light", (bool*)&isPoint));
            scene->lights[selectedLight].lightPos.w = isPoint;

        ImGui::ColorEdit4("Ambient", scene->lights[selectedLight].ambient.e, ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4("Diffuse", scene->lights[selectedLight].diffuse.e, ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4("Specular", scene->lights[selectedLight].specular.e, ImGuiColorEditFlags_Float);

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

        ImGui::ColorEdit4("Ambient", scene->materials[selectedMaterial].ambientColor.e, ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4("Diffuse", scene->materials[selectedMaterial].diffuseColor.e, ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4("Specular", scene->materials[selectedMaterial].specularColor.e, ImGuiColorEditFlags_Float);
        ImGui::ColorEdit4("Emission", scene->materials[selectedMaterial].emissionColor.e, ImGuiColorEditFlags_Float);

        ImGui::DragFloat("shininess", &scene->materials[selectedMaterial].shininess, 0.f, 128.f);

        ImGui::TreePop();
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

            ImGui::SliderInt("Material index", &scene->objects[selectedObject].mesh[selectedMesh].materialIndex, 0, scene->materials.size() - 1);
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
            rdrDrawTriangles(renderer, face.vertices, 3);
    }
}

std::vector<Object> sortObjects(std::vector<Object> objects, const float3& cameraPos)
{
    // Sort all objects with their distance to the camera
    sort(objects.begin(), objects.end(),
        [cameraPos](const Object& a, const Object& b)
    {
        float3 aPos = (a.getModel() * float4 { 0.f, 0.f, 0.f, 1.f }).xyz;
        float3 bPos = (b.getModel() * float4 { 0.f, 0.f, 0.f, 1.f }).xyz;
        return sqMagnitude(cameraPos - aPos) > sqMagnitude(cameraPos - bPos);
    });

    return objects;
}

void scnImpl::update(float deltaTime, rdrImpl* renderer)
{
    //rdrSetUniformBool(renderer, UT_STENCIL_TEST, false);

    editLights(renderer, this);

    time += deltaTime;

    // Sort objects
    std::vector<Object> sortedObjects = sortObjects(objects, cameraPos);
    
    // Draw all objects
    for (const auto& object : sortedObjects)
        drawObject(object, renderer);
}

void scnImpl::showImGuiControls()
{
    editObjects(this);
    editMaterials(this);
}

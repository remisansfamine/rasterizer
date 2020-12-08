
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

Texture* loadTexture(std::vector<Texture>& textures, const char* filePath)
{
    Texture texture;
    texture.fileName = filePath;
    texture.data = (float4*)stbi_loadf(filePath, &texture.width, &texture.height, nullptr, STBI_rgb_alpha);

    if (!texture.data)
        return nullptr;

    textures.push_back(texture);

    return &textures.back();
}

bool loadObject(std::vector<Face>& faces, std::vector<Texture>& textures, std::string filePath, std::string mtlBasedir, float scale = 1.f)
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

    for (tinyobj::material_t& mat : materials)
    {
        std::string tex_filepath = mtlBasedir + mat.diffuse_texname;

        if (std::find_if(textures.begin(), textures.end(),
            [tex_filepath](Texture& t)
            { return t.fileName.compare(tex_filepath) == 0; }) == textures.end())
            loadTexture(textures, tex_filepath.c_str());
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            int fv = shapes[s].mesh.num_face_vertices[f];

            Face face;

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
                    vertice.a = 1.f;
                    //vertice.a = attrib.colors[3 * idx.vertex_index + 3];
                }

                if (!attrib.texcoords.empty())
                {
                    vertice.u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertice.v = attrib.texcoords[2 * idx.texcoord_index + 1];
                }

                face.vertices.push_back(vertice);
            }

            index_offset += fv;

            if (!materials.empty())
            {
                tinyobj::material_t& mat = materials[shapes[s].mesh.material_ids[f]];

                for (Texture& texture : textures)
                {
                    if (texture.fileName.compare(mtlBasedir + mat.diffuse_texname) == 0)
                    {
                        face.texture = &texture;
                        break;
                    }
                }
            }
            faces.push_back(face);
        }
    }

    return 1;
}

void loadQuad(std::vector<Face>& faces, int hRes = 1, int vRes = 1)
{
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
            Face face1;
            face1.vertices.push_back({ u0 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u0, v0 });
            face1.vertices.push_back({ u1 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u1, v0 });
            face1.vertices.push_back({ u1 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u1, v1 });
            faces.push_back(face1);

            Face face2;
            face2.vertices.push_back({ u1 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u1, v1 });
            face2.vertices.push_back({ u0 - 0.5f, v1 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u0, v1 });
            face2.vertices.push_back({ u0 - 0.5f, v0 - 0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 1.0f, 1.0f, 1.f,     u0, v0 });
            faces.push_back(face2);
        }
    }
}

void loadTriangle(std::vector<Face>& faces)
{
    Face face;

    //                          pos                   normal                  color                     uv
    face.vertices.push_back({-0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
    face.vertices.push_back({ 0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     0.5f, 0.5f });
    face.vertices.push_back({ 0.0f,  0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f });

    faces.push_back(face);
}

scnImpl* scnCreate()
{
    return new scnImpl();
}

void scnDestroy(scnImpl* scene)
{
    delete scene;
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

    Object obj1;
    loadQuad(obj1.faces);
    objects.push_back(obj1);

    Object obj2;
    loadQuad(obj2.faces);
    //if (loadObject(obj2.vertices, "assets/sphere.obj", "assets", 0.5f))
    objects.push_back(obj2);
    
    Object obj3;
    loadObject(obj3.faces, textures, "assets/the_noble_craftsman.obj", "assets/");
    objects.push_back(obj3);

    Object obj4;
    loadObject(obj4.faces, textures, "assets/deathclaw.obj", "assets/", 0.005f);
    //loadObject(obj4.faces, textures, "assets/suzanne.obj", "assets/");
    objects.push_back(obj4);
}

scnImpl::~scnImpl()
{
    for (Texture& texture : textures)
        stbi_image_free(texture.data);
    // HERE: Unload the scene
}

void scnImpl::drawObject(Object object, rdrImpl* renderer)
{
    for (const Face& face : object.faces)
    {
        if (face.texture && face.texture->data && face.texture->height > 0 && face.texture->width > 0)
            rdrSetTexture(renderer, face.texture->data->e, face.texture->width, face.texture->height);

        rdrDrawTriangles(renderer, face.vertices.data(), (int)face.vertices.size());
    }
}

void editLight(rdrImpl* renderer, scnImpl* scene)
{
    static int selectedLight = 0;
    if (ImGui::TreeNode("Lights"))
    {
        if (ImGui::SliderFloat4("Global ambient", scene->globalAmbient.e, 0.f, 1.f))
            rdrSetUniformFloatV(renderer, UT_GLOBALAMBIENT, scene->globalAmbient.e);

        ImGui::SliderInt("Selected light", &selectedLight, 0, 7);
        ImGui::Checkbox("Is enable", &scene->lights[selectedLight].isEnable);

        ImGui::SliderFloat4("Position", scene->lights[selectedLight].lightPos.e, -10.f, 10.f);

        bool isPoint = scene->lights[selectedLight].lightPos.w == 1.f;
        if (ImGui::Checkbox("Is point light", (bool*)&isPoint));
            scene->lights[selectedLight].lightPos.w = isPoint;

        ImGui::SliderFloat4("Ambient", scene->lights[selectedLight].ambient.e, 0.f, 1.f);
        ImGui::SliderFloat4("Diffuse", scene->lights[selectedLight].diffuse.e, 0.f, 1.f);
        ImGui::SliderFloat4("Specular", scene->lights[selectedLight].specular.e, 0.f, 1.f);

        ImGui::SliderFloat("Constant attenuation", &scene->lights[selectedLight].constantAttenuation, 0.f, 10.f);
        ImGui::SliderFloat("Linear attenuation", &scene->lights[selectedLight].linearAttenuation, 0.f, 10.f);
        ImGui::SliderFloat("Quadratic attenuation", &scene->lights[selectedLight].quadraticAttenuation, 0.f, 10.f);

        rdrSetUniformLight(renderer, selectedLight, (rdrLight*)&scene->lights[selectedLight]);

        ImGui::TreePop();
    }
}

void scnImpl::update(float deltaTime, rdrImpl* renderer)
{
    //rdrSetUniformBool(renderer, UT_DEPTHTEST, true);
    rdrSetUniformBool(renderer, UT_STENCTILTEST, false);

    float4 color = { 1.f,1.f,1.f,1.f };
    rdrSetUniformFloatV(renderer, UT_GLOBALCOLOR, color.e);

    editLight(renderer, this);

    time += deltaTime;
    
    rdrSetTexture(renderer, textures[0].data->e, textures[0].width, textures[0].height);

    objects[0].model = mat4::translate({ 0.f, 0.f, -3.f });// *mat4::rotateY(time);
    rdrSetModel(renderer, objects[0].model.e);

    // Draw
    drawObject(objects[0], renderer);

    if (objects.size() <= 2)
        return;
    
    //rdrSetTexture(renderer, textures[1].data, textures[1].width, textures[1].height);
    rdrSetTexture(renderer, nullptr, textures[1].width, textures[1].height);

    color = { 1.f, 0.f, 0.f,0.5f };
    //rdrSetUniformFloatV(renderer, UT_GLOBALCOLOR, color.e);

    objects[3].model = mat4::translate({ 0.f, -5.f, 0.f });// *mat4::rotateY(time);
    rdrSetModel(renderer, objects[3].model.e);

    drawObject(objects[3], renderer);

    color = { 0.f,0.f,1.f,0.5f };
    //rdrSetUniformFloatV(renderer, UT_GLOBALCOLOR, color.e);

    objects[3].model = mat4::translate({ 0.f, -5.f, -2.f });// *mat4::rotateY(time);
    rdrSetModel(renderer, objects[3].model.e);

    drawObject(objects[3], renderer);

    color = { 0.f, 1.f, 0.f,0.5f };
    //rdrSetUniformFloatV(renderer, UT_GLOBALCOLOR, color.e);

    objects[3].model = mat4::translate({ 0.f, -5.f, -4.f });// *mat4::rotateY(time);
    rdrSetModel(renderer, objects[3].model.e);

    drawObject(objects[3], renderer);

    /*// Draw craftsman
    objects[2].scale = { scale, scale, scale };

    rdrSetModel(renderer, objects[2].getModel().e);

    drawObject(objects[2], renderer);

    // Draw ground
    rdrSetTexture(renderer, nullptr, 0, 0);

    objects[1].model = mat4::rotateX(-M_PI * 0.5f) * mat4::scale({ 3.f, 3.f, 3.f });

    rdrSetModel(renderer, objects[1].model.e);

    drawObject(objects[1], renderer);

    rdrSetUniformBool(renderer, UT_DEPTHTEST, false);

    // Draw mirror
    mat4x4 mirror = objects[2].getModel() * mat4::rotateZ(M_PI);

    rdrSetModel(renderer, mirror.e);

    drawObject(objects[2], renderer);*/
}

void scnImpl::showImGuiControls()
{
    ImGui::SliderFloat("scale", &scale, 0.f, 1.f);

    ImGui::DragFloat3("sphere position", objects[2].position.e);
}

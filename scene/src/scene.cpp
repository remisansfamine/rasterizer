
#include <imgui.h>

#include <common/maths.hpp>

#include "scene_impl.hpp"

#include <tiny_obj_loader.h>
#include <stb_image.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "common\maths.hpp"

#include <iostream>

bool loadObject(std::vector<rdrVertex>& vertices, std::string filePath, std::string mtlBasedir, float scale = 1.f)
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

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            int fv = shapes[s].mesh.num_face_vertices[f];

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
                    vertice.a = attrib.colors[3 * idx.vertex_index + 3];
                }

                if (!attrib.texcoords.empty())
                {
                    vertice.u = attrib.texcoords[2 * idx.texcoord_index + 0];
                    vertice.v = attrib.texcoords[2 * idx.texcoord_index + 1];
                }

                vertices.push_back(vertice);
            }
            index_offset += fv;

            shapes[s].mesh.material_ids[f];
        }
    }

    return 1;
}

void loadQuad(std::vector<rdrVertex>& vertices)
{
    //                          pos                   normal                    color                  uv
    vertices.push_back({ -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
    vertices.push_back({  0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.f,     1.0f, 0.0f });
    vertices.push_back({  0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     1.0f, 1.0f });

    vertices.push_back({  0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     1.0f, 1.0f });
    vertices.push_back({ -0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f });
    vertices.push_back({ -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
}

void loadTriangle(std::vector<rdrVertex>& vertices)
{
    //                          pos                   normal                  color                     uv
    vertices.push_back({-0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
    vertices.push_back({ 0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     0.5f, 0.5f });
    vertices.push_back({ 0.0f,  0.5f, 0.0f,      0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f });
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
    Texture texture1;
    texture1.data = stbi_loadf("assets/danny.jpeg", &texture1.width, &texture1.height, nullptr, STBI_rgb_alpha);

    Texture texture2;
    texture2.data = stbi_loadf("assets/Deathclaw.png", &texture2.width, &texture2.height, nullptr, STBI_rgb_alpha);

    textures.push_back(texture1);
    textures.push_back(texture2);

    Object obj1;
    loadQuad(obj1.vertices);
    objects.push_back(obj1);

    Object obj2;
    if (loadObject(obj2.vertices, "assets/car.obj", "assets", 0.005f))
        objects.push_back(obj2);
    
}

scnImpl::~scnImpl()
{
    for (Texture& texture : textures)
        stbi_image_free(texture.data);
    // HERE: Unload the scene
}

void scnImpl::update(float deltaTime, rdrImpl* renderer)
{
    Light light =
    {
        {0.f, (sin((float)time) + 1.f) * 0.5f, 0.f},
        {1.f, 0.f, 0.f, 1.f},
        true,
        1.f
    };

    Light light2 =
    {
        {0.f, (1.f - sin((float)time)) * 0.5f, 0.f},
        {0.f, 1.f, 1.f, 1.f},
        true,
        1.f
    };

    rdrSetUniformLight(renderer, 0, (rdrLight*)&light);
    rdrSetUniformLight(renderer, 1, (rdrLight*)&light2);

    rdrSetUniformFloatV(renderer, rdrUniformType::UT_TIME, (float*)&time);
    time += deltaTime;

    // HERE: Update (if needed) and display the scene
    rdrSetTexture(renderer, textures[0].data, textures[0].width, textures[0].height);

    objects[0].model = mat4::translate({ 0.f, 0.f, -3.f }) * mat4::rotateY(time);
    rdrSetModel(renderer, objects[0].model.e);

    // Draw
    rdrDrawTriangles(renderer, objects[0].vertices.data(), (int)objects[0].vertices.size());

    if (objects.size() <= 1)
        return;

    rdrSetTexture(renderer, textures[1].data, textures[1].width, textures[1].height);

    objects[1].model = mat4::identity();

    rdrSetModel(renderer, objects[1].model.e);

    // Draw
    rdrDrawTriangles(renderer, objects[1].vertices.data(), (int)objects[1].vertices.size());
}

void scnImpl::showImGuiControls()
{
    ImGui::SliderFloat("scale", &scale, 0.f, 1.f);
}

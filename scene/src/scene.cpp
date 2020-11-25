
#include <imgui.h>

#include <common/maths.hpp>

#include "scene_impl.hpp"

#include <tiny_obj_loader.h>
#include <stb_image.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "common\maths.hpp"

void loadObject(std::vector<rdrVertex>& vertices, std::string filePath, std::string mtlBasedir, float scale = 1.f)
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
        exit(1);
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

                vertices.push_back({
                    attrib.vertices[3 * idx.vertex_index + 0] * scale,
                    attrib.vertices[3 * idx.vertex_index + 1] * scale,
                    attrib.vertices[3 * idx.vertex_index + 2] * scale,
                    attrib.normals[3 * idx.normal_index + 0],
                    attrib.normals[3 * idx.normal_index + 1],
                    attrib.normals[3 * idx.normal_index + 2],
                    attrib.colors[3 * idx.vertex_index + 0],
                    attrib.colors[3 * idx.vertex_index + 1],
                    attrib.colors[3 * idx.vertex_index + 2],
                    1.f,
                    attrib.texcoords[2 * idx.texcoord_index + 0],
                    attrib.texcoords[2 * idx.texcoord_index + 1],
                });
            }
            index_offset += fv;

            shapes[s].mesh.material_ids[f];
        }
    }
}

void loadQuad(std::vector<rdrVertex>& vertices)
{
    //                          pos                   normal                    color                  uv
    vertices.push_back({ -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 0.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
    vertices.push_back({  0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 0.0f,      0.0f, 0.0f, 1.0f, 1.f,     1.0f, 0.0f });
    vertices.push_back({  0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 0.0f,      0.0f, 1.0f, 0.0f, 1.f,     1.0f, 1.0f });

    vertices.push_back({ 0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 0.0f,       0.0f, 1.0f, 0.0f, 1.f,     1.0f, 1.0f });
    vertices.push_back({ -0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 0.0f,      1.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f });
    vertices.push_back({ -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 0.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
}

void loadTriangle(std::vector<rdrVertex>& vertices)
{
    //                          pos                   normal                  color                     uv
    vertices.push_back({-0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 0.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
    vertices.push_back({ 0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 0.0f,      0.0f, 1.0f, 0.0f, 1.f,     0.5f, 0.5f });
    vertices.push_back({ 0.0f,  0.5f, 0.0f,      0.0f, 0.0f, 0.0f,      0.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f });
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

    Object obj2;
    loadObject(obj2.vertices, "assets/Deathclaw.obj", "assets", 0.005f);

    objects.push_back(obj1);
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
    // HERE: Update (if needed) and display the scene
    rdrSetTexture(renderer, textures[0].data, textures[0].width, textures[0].height);

    objects[0].model = mat4::translate({ 0.f, 0.f, -1.f })* mat4::scale({ 0.5f, 0.5f, 0.5f });
    rdrSetModel(renderer, objects[0].model.e);

    // Draw
    rdrDrawTriangles(renderer, objects[0].vertices.data(), (int)objects[0].vertices.size());

    rdrSetTexture(renderer, textures[1].data, textures[1].width, textures[1].height);

    objects[1].model = mat4::identity();

    rdrSetModel(renderer, objects[1].model.e);

    // Draw
    rdrDrawTriangles(renderer, objects[1].vertices.data(), (int)objects[1].vertices.size());
    
    time += deltaTime;
}

void scnImpl::showImGuiControls()
{
    ImGui::SliderFloat("scale", &scale, 0.f, 1.f);
}

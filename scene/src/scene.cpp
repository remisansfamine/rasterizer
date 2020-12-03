
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
    texture.data = stbi_loadf(filePath, &texture.width, &texture.height, nullptr, STBI_rgb_alpha);

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

    if (!materials.empty())
    for (size_t m = 0; m < materials.size(); m++)
    {
        tinyobj::material_t& mat = materials[m];

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
                    vertice.a = attrib.colors[3 * idx.vertex_index + 3];
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

void loadQuad(std::vector<Face>& faces)
{
    Face face1;

    //                          pos                   normal                    color                  uv
    face1.vertices.push_back({ -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });
    face1.vertices.push_back({  0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 0.0f, 1.0f, 1.f,     1.0f, 0.0f });
    face1.vertices.push_back({  0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     1.0f, 1.0f });

    faces.push_back(face1);
    Face face2;

    face2.vertices.push_back({  0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      0.0f, 1.0f, 0.0f, 1.f,     1.0f, 1.0f });
    face2.vertices.push_back({ -0.5f,  0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 1.0f, 1.f,     0.0f, 1.0f });
    face2.vertices.push_back({ -0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f,      1.0f, 0.0f, 0.0f, 1.f,     0.0f, 0.0f });

    faces.push_back(face2);
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
    loadTexture(textures, "assets/danny.jpeg");
    //loadTexture(this, "assets/Deathclaw.png");

    Object obj1;
    loadQuad(obj1.faces);
    objects.push_back(obj1);

    Object obj2;
    loadTriangle(obj2.faces);
    //if (loadObject(obj2.vertices, "assets/sphere.obj", "assets", 0.5f))
        objects.push_back(obj2);
    
    Object obj3;
        loadObject(obj3.faces, textures, "assets/sponza-master/sponza.obj", "assets/sponza-master/", 0.005f);
        objects.push_back(obj3);

    lights[0].isEnable = true;
    lights[0].lightPos.w = 0.f;
    lights[0].lightPos.xyz = {0.f, -1.f, 0.f};
    lights[0].ambient = { 0.f, 0.f, 0.f, 1.f };
    lights[0].diffuse = { 0.f, 0.f, 0.f, 1.f };
    lights[0].specular = { 1.f, 0.f, 0.f, 1.f };
    
    lights[1].isEnable = true;
    lights[1].lightPos.w = 0.f;
    lights[1].lightPos.xyz = { 0.f, 0.f, -1.f };
    lights[1].ambient = { 0.f, 0.f, 0.f, 1.f };
    lights[1].diffuse =  { 0.f, 0.f, 0.f, 1.f };
    lights[1].specular = { 1.f, 1.f, 1.f, 1.f };

    lights[2].isEnable = true;
    lights[2].lightPos.w = 0.f;
    lights[2].lightPos.xyz = { 0.f, 1.f, 0.f };
    lights[2].diffuse = { 0.f, 0.f, 0.f, 1.f };
    lights[2].ambient =  { 1.f, 0.f, 0.f, 1.f };
    lights[2].specular = { 1.f, 1.f, 0.f, 1.f };
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
            rdrSetTexture(renderer, face.texture->data, face.texture->width, face.texture->height);

        rdrDrawTriangles(renderer, face.vertices.data(), (int)face.vertices.size());
    }
}

void scnImpl::update(float deltaTime, rdrImpl* renderer)
{

    rdrSetUniformFloatV(renderer, rdrUniformType::UT_TIME, (float*)&time);
    time += deltaTime;

    rdrSetUniformLight(renderer, 0, (rdrLight*)&lights[0]);
    //rdrSetUniformLight(renderer, 1, (rdrLight*)&lights[1]);
    //rdrSetUniformLight(renderer, 2, (rdrLight*)&lights[2]);
    
    rdrSetTexture(renderer, textures[0].data, textures[0].width, textures[0].height);

    objects[0].model = mat4::translate({ 0.f, 0.f, -3.f });// *mat4::rotateY(time);
    rdrSetModel(renderer, objects[0].model.e);

    // Draw

    // Draw
    drawObject(objects[0], renderer);

    if (objects.size() <= 1)
        return;

    rdrSetTexture(renderer, nullptr, textures[1].width, textures[1].height);

    objects[1].model = mat4::translate({ 2.f, -1.f, 0.f }) * mat4::rotateX(-M_PI * 0.5f) * mat4::scale({2.f, 2.f, 2.f});

    rdrSetModel(renderer, objects[1].model.e);

    // Draw
    drawObject(objects[1], renderer);

    if (objects.size() <= 2)
        return;

    objects[2].model = mat4::translate({ -2.f, 0.f, 0.f });

    rdrSetModel(renderer, objects[2].model.e);

    drawObject(objects[2], renderer);
}

void scnImpl::showImGuiControls()
{
    ImGui::SliderFloat("scale", &scale, 0.f, 1.f);
}

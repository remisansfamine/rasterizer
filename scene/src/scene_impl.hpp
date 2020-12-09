
#include <vector>

#include <rdr/renderer.h>
#include <scn/scene.h>

struct Texture
{
    std::string fileName;
    int width = 0, height = 0;
    float* data = nullptr;
};

struct Face
{
    std::vector<rdrVertex> vertices;
    int textureIndex  = -1;
    int materialIndex = -1;
};

struct Object
{
    bool isEnable = true;
    std::vector<Face> faces;
    float3 position = { 0.f, 0.f, 0.f };
    float3 rotation = { 0.f, 0.f, 0.f };
    float3 scale = { 1.f, 1.f, 1.f };

    mat4x4 model = mat4::identity();

    Object(float3 pos = { 0.f, 0.f, 0.f }, float3 rot = { 0.f, 0.f, 0.f }, float3 scale = { 1.f, 1.f, 1.f })
        : position(pos), rotation(rot), scale(scale) {}

    // Return the object model with his transform
    mat4x4 getModel() { return mat4::rotateX(rotation.x) * mat4::rotateY(rotation.y) * mat4::translate(position) * mat4::scale(scale); }
};

struct Light
{
    bool    isEnable = false;
    float4  lightPos = { 0.f, 0.f, 0.f, 1.f };
    float4  ambient = { 0.f, 0.f, 0.f, 0.f };
    float4  diffuse = { 1.f, 1.f, 1.f, 1.f };
    float4  specular = { 0.f, 0.f, 0.f, 0.f };
    float   constantAttenuation = 1.f;
    float   linearAttenuation = 0.f;
    float   quadraticAttenuation = 0.f;
};

struct Material
{
    float4 ambientColor = { 1.f, 1.f, 1.f, 1.f };
    float4 diffuseColor = { 1.f, 1.f, 1.f, 1.f };
    float4 specularColor = { 1.f, 1.f, 1.f, 1.f };
    float4 emissionColor = { 0.f, 0.f, 0.f, 0.f };
    float shininess = 20.f;
};

struct scnImpl
{
    scnImpl();
    ~scnImpl();
    void update(float deltaTime, rdrImpl* renderer);

    void showImGuiControls();

    float4 globalAmbient = {0.2f, 0.2f, 0.2f, 1.f };

    std::vector<Object> objects;
    std::vector<Texture> textures;

    Material defaultMaterial =
    {
        { 1.f, 1.f, 1.f, 1.f },
        { 1.f, 1.f, 1.f, 1.f },
        { 1.f, 1.f, 1.f, 1.f },
        { 0.f, 0.f, 0.f, 0.f },
        20.f
    };

    std::vector<Material> materials;
    Light lights[8];

    bool subdivide = false;

    float3 cameraPos;

    private:
        void drawObject(Object object, rdrImpl* renderer);

        double time = 0.0;
};
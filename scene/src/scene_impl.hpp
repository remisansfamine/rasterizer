
#include <vector>

#include <rdr/renderer.h>
#include <scn/scene.h>

struct Texture
{
    std::string fileName;
    int width = 0, height = 0;
    float* data = nullptr;
};

struct Triangle
{
    rdrVertex vertices[3];
};

struct Mesh
{
    std::vector<Triangle> faces;
    int textureIndex = -1;
    int materialIndex = 0;

    Mesh() = default;
    Mesh(int textureIndex, int materialIndex)
        : textureIndex(textureIndex), materialIndex(materialIndex)
    {}
};

struct Object
{
    bool isEnable = true;
    std::vector<Mesh> mesh;
    float3 position = { 0.f, 0.f, 0.f };
    float3 rotation = { 0.f, 0.f, 0.f };
    float3 scale    = { 1.f, 1.f, 1.f };

    mat4x4 model = mat4::identity();

    Object(float3 pos = { 0.f, 0.f, 0.f }, float3 rot = { 0.f, 0.f, 0.f }, float3 scale = { 1.f, 1.f, 1.f })
        : position(pos), rotation(rot), scale(scale) {}

    // Return the object model with his transform
    mat4x4 getModel() const
    {
        return mat4::translate(position) * mat4::rotateX(rotation.x) * mat4::rotateY(rotation.y) * mat4::rotateZ(rotation.z) * mat4::scale(scale);
    }
};

struct Light
{
    bool    isEnable = false;

    float4  lightPos = { 0.0f, 0.0f, 0.0f, 1.f };
    float4  ambient  = { 0.0f, 0.0f, 0.0f, 1.f };
    float4  diffuse  = { 1.0f, 1.0f, 1.0f, 1.f };
    float4  specular = { 1.0f, 1.0f, 1.0f, 1.f };

    float   constantAttenuation  = 1.f;
    float   linearAttenuation    = 0.f;
    float   quadraticAttenuation = 0.f;
};

struct Material
{
    float4 ambientColor  = { 0.2f, 0.2f, 0.2f, 1.0f };
    float4 diffuseColor  = { 0.8f, 0.8f, 0.8f, 1.0f };
    float4 specularColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    float4 emissionColor = { 0.0f, 0.0f, 0.0f, 0.0f };

    float shininess = 20.f;
};

struct scnImpl
{
    scnImpl();
    ~scnImpl();


    std::vector<Object> objects;
    std::vector<Texture> textures;

    Material defaultMaterial;

    std::vector<Material> materials = { defaultMaterial };
    Light lights[8];

    float3 cameraPos = { 0.f, 0.f, 0.f };





    void update(float deltaTime, rdrImpl* renderer);

    void showImGuiControls();

    private:
        void drawObject(Object object, rdrImpl* renderer);

        int  loadTexture(const char* filePath);
        int  loadMaterial(float ambient[3], float diffuse[3], float specular[3], float emissive[3], float shininess);

        bool loadObject(Object& object, std::string filePath, std::string mtlBasedir, float scale = 1.f);
        void loadQuad(Object& object, int textureIndex = -1, int materialIndex = 0, int hRes = 1, int vRes = 1);
        void loadTriangle(Object& object, int textureIndex = -1, int materialIndex = 0);

        double time = 0.0;
};
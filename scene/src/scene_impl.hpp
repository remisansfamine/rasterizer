
#include <vector>

#include <rdr/renderer.h>
#include <scn/scene.h>

struct Face
{
    std::vector<rdrVertex> vertices;
    Texture* texture = nullptr;
};

struct Object
{
    std::vector<Face> faces;
    float3 position = { 0.f, 0.f, 0.f };
    float3 rotation = { 0.f, 0.f, 0.f };
    float3 scale = { 1.f, 1.f, 1.f };

    mat4x4 model = mat4::identity();

    mat4x4 getModel()
    {
        return mat4::rotateX(rotation.x) * mat4::rotateY(rotation.y) * mat4::translate(position) * mat4::scale(scale);
    }
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
    Light lights[8];

    bool subdivide = false;

    private:
        void drawObject(Object object, rdrImpl* renderer);

        double time = 0.0;
        float scale = 1.f;
};
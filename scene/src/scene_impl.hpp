
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
    mat4x4 model = mat4::identity();
};

struct scnImpl
{
    scnImpl();
    ~scnImpl();
    void update(float deltaTime, rdrImpl* renderer);

    void showImGuiControls();

    std::vector<Object> objects;
    std::vector<Texture> textures;
    Light lights[8];

    private:
        void drawObject(Object object, rdrImpl* renderer);

        double time = 0.0;
        float scale = 1.f;
};
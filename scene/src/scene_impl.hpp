
#include <vector>

#include <rdr/renderer.h>
#include <scn/scene.h>

struct rdrImpl;

struct Object
{
    std::vector<rdrVertex> vertices;
    mat4x4 model;
};

struct scnImpl
{
    scnImpl();
    ~scnImpl();
    void update(float deltaTime, rdrImpl* renderer);

    void showImGuiControls();

private:
    double time = 0.0;
    std::vector<Object> objects;
    float scale = 1.f;

    std::vector<Texture> textures;
};
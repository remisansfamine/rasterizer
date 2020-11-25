#pragma once

#ifdef RDR_EXPORTS
#define RDR_API __declspec(dllexport)
#else
#define RDR_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// Opaque struct to store our data privately
typedef struct rdrImpl rdrImpl;

// Vertex format (only one supported for now)
typedef struct rdrVertex
{
    float x, y, z;    // Pos
    float nx, ny, nz; // Normal
    float r, g, b, a; // Color
    float u, v;       // Texture coordinates
} rdrVertex;

// Init/Shutdown function
// Color and depth buffer have to be valid until the shutdown of the renderer
// Color buffer is RGBA, each component is a 32 bits float
// Depth buffer is a buffer of 32bits floats
RDR_API rdrImpl* rdrInit(float* colorBuffer32Bits, float* depthBuffer, int width, int height);
RDR_API void rdrShutdown(rdrImpl* renderer);

// Matrix setup
RDR_API void rdrSetProjection(rdrImpl* renderer, float* projectionMatrix);
RDR_API void rdrSetView(rdrImpl* renderer, float* viewMatrix);
RDR_API void rdrSetModel(rdrImpl* renderer, float* modelMatrix);
RDR_API void rdrSetViewport(rdrImpl* renderer, int x, int y, int width, int height);

// Texture setup
RDR_API void rdrSetTexture(rdrImpl* renderer, float* colors32Bits, int width, int height);

// Draw a list of triangles
RDR_API void rdrDrawTriangles(rdrImpl* renderer, rdrVertex* vertices, int vertexCount);

struct ImGuiContext;
RDR_API void rdrSetImGuiContext(rdrImpl* renderer, struct ImGuiContext* context);
RDR_API void rdrShowImGuiControls(rdrImpl* renderer);

#ifdef __cplusplus
}
#endif

Rasterization rendering
===
CPU Rendering library written in C++ 17 for C/C++ and scene loader (using stb, TinyObjLoader, glfw and ImGui) accompanied by a mathematics library.

**/!\\ CPU Software renderers are not performant nor efficient, it is not recommended to use them on a serious project. Use it at your own risk. /!\\**

***Renderer***
===
**_Description:_**
CPU Renderer using rasterization to display 3D models in a color buffer
(Use glfw and ImGui)


Renderer table of Contents
===
1. [Features](#rdrfeatures)
2. [Usage](#rdrusage)
3. [How does it work ?](#rdrexplications)
4. [Exemples](#rdrexemples)
5. [References](#rdrreferences)

<div id='rdrfeatures' />

# Features
* Draw triangles on the input color buffer using input vertices
* Triangle wireframe
* Triangle rasterization
* Depth test using the input depth buffer
* Triangle homogeneous clipping
* Texture support (+ bilinear filtering)
* Material support (ambient, diffuse, specular and emission)
* Lighting support using Gouraud and Phong models (ambient, diffuse, specular and attenuation)
* Blending support (+ texture with transparence and cutout)
* Gamma correction
* Post-process effect (Box blur, Gaussian blur, Light bloom, MSAA)

<div id='rdrusage' />

# Usage
Initialization
---
```c++
rdrImpl* rdrInit(float* colorBuffer, float* depthBuffer, int width, int height)
```

Set rendering parameters
---
```c++
void rdrSetUniformFloatV(rdrImpl* renderer, rdrUniformType type, float* value)
void rdrSetUniformBool(rdrImpl* renderer, rdrUniformType type, bool value)

void rdrSetModel(rdrImpl* renderer, float* modelMatrix)
void rdrSetView(rdrImpl* renderer, float* viewMatrix)
void rdrSetProjection(rdrImpl* renderer, float* projectionMatrix)

void rdrSetTexture(rdrImpl* renderer, float* colors32Bits, int width, int height)
void rdrSetUniformMaterial(rdrImpl* renderer, rdrMaterial* material)
void rdrSetUniformLight(rdrImpl* renderer, int index, rdrLight* light)
```

Call post-process effects
---
```c++
void rdrFinish(rdrImpl* renderer)
```

Shutdown
---
```c++
void rdrShutdown(rdrImpl* renderer)
```
<div id='rdrexplications' />

# How does it work ?

Summary
---
1. [Vertex shader](#vshader)
2. [Clipping](#clipping)
3. [Normalized Device Coordinates](#ndc)
4. [Screen coordinates](#screencoords)
5. [Rasterization](#rasterization)
6. [Pixel shader](#pshader)
7. [Blending](#blending)
8. [Post-process](#post-process)

<div id='vshader' />

Vertex shader
---
First of all, the renderer takes the input vertices and applies the vertex shader to them. The vertex shader computes the clip coordinates (which are homogeneous) with the Model-View-Projection matrix (it also applies the Model matrix to the normals and the local coordinates). It saves the vertex informations (like colors and UVs) in a varying (for each vertex) for further operations and to calculate lighting.

<div id='clipping' />

Outcodes and outpoints computing - Clipping
---
After that the pipeline calls two functions to check if the current triangle needs to be clipped, and how to clip it. If it should be clipped, it adds new triangles to rasterize with new varyings (for each vertex).

<div id='ndc' />

Normalized Device Coordinates
---
Then the renderer divides the homogeneous coordinates by their w component to get NDC coordinates (which are between -1 and 1). With these coordinates the renderer can ignore some faces (like back faces) using the normal of the triangle's face.

<div id='screencoords' />

Screen coordinates
---
NDC coordinates are then remapped with the viewport definition, and the varyings are interpolated with the weights of clipped coordinates. With these remapped coordinates and the interpolated varyings the main function can finally rasterize the clipped triangles (or draw them as lines with the Wireframe mode).

<div id='rasterization' />

Rasterization
---
At the start of this step the bounding box of the current triangle is calculated. For each of its pixel, his weight is computed to check if it is in the triangle or not (If MSAA is enabled, this step uses the samples of the pixel instead of using the pixel centroid). After passing this test, the depth test should be passed, it checks if there is already a pixel drawn in the color buffer at his position and if his depth is greater than its own. Then the perspective correction is occurred to avoid PS1 graphics-like and get correct weights to interpolate varyings. 

<div id='pshader' />

Pixel shader
---
After getting the interpolated varying, the fragment shader is called. The fragment shader calculates the pixel color using the differents values of the inputs. The lighting can be calculated here (If the Phong model is enabled, else it is calculated during Vertex shader). If the current triangle is textured, the fragment shader gets the appropriate color using the UVs (It can also filter the texture using bilinear interpolation). The fragment shader can also discard pixels depending on its settings.

<div id='blending' />

Blending
---
After getting the fragment color, the blending can be applied with the old color of the pixel to have a transparency effect. Then the alpha test should be passed to write in the depth buffer (to avoid non-transparent faces due to the rendering order of the vertices). These two steps are applied to each sample of the current pixel if the MSAA is enabled. After these steps the buffers (color buffer or MSAA color buffer) can be filled with the blended color.

<div id='post-process' />

Post-process
---
The final step is to apply effects on the frame buffer after getting all pixels (or of the samples) color, by traversing all the pixels of the frame buffer. If the MSAA is enabled, no pixel has a color, this color is obtained by calculating the average color of all samples of the current pixel. Then other effects can be applied like Box blur, Gaussian blur or Bloom. These effects are applied by obtaining the average of pixels around the current one with some factors. At the very end, the frame buffer is traversed once more to apply the gamma correction.

<div id='rdrexemples' />

# Exemples
<div style="text-align:center">

![Lighting](/annexes/lighting.gif)

Lighting using Gouraud and Phong shading: diffuse, specular, emissive and attenuation.

![Blending](/annexes/blending.gif)

Depth test and blending using a transparent texture.

![Face culling](/annexes/face_cull.gif)

Configurable face culling and perspective correction.

![Bilinear](/annexes/bilinear.png) 
![Nearest](/annexes/nearest.png)

10x10 Texture with a bilinear filter - The same texture without any filter.

![Post-process](/annexes/post-process.gif)

Post-processing effects: MSAA, Box blur and Gaussian blur.

</div>

<div id='rdrreferences'/>

# References

Viewport:
- Gives the formula to convert normalized device coordinates to viewport coordinates (here the viewport is the screen): https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glViewport.xhtml

Rasterization:
- Shows how to check if the current pixel is in the triangle and how to use the top-left rule: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage

Perspective correction:
- Shows the technique to correct the perspective: https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/perspective-correct-interpolation-vertex-attributes

Face culling:
- Shows the method to use the face culling: https://www.students.cs.ubc.ca/~cs-314/notes/cull.html

Lighting:
- Shows the differences between Gouraud shading and Phong shading: https://en.wikipedia.org/wiki/Gouraud_shading
- Shows the principle of Phong shading: https://en.wikipedia.org/wiki/Phong_shading
- Gives an alternative to the Phong model: https://en.wikipedia.org/wiki/Blinn%E2%80%93Phong_reflection_model
- Gives the formula of the Phong shading: https://en.wikipedia.org/wiki/Phong_reflection_model
- Shows how OpenGL uses its light systeme and how materials and light are related: https://www.glprogramming.com/red/chapter05.html

Blending:
- Shows how to use the blending and the issues with the blending (like sorting the render order): https://learnopengl.com/Advanced-OpenGL/Blending

Clipping:
- Gives the main structure of the algorithm: https://fabiensanglard.net/polygon_codec/
- Shows how to use outcodes: https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
- Shows the complexity of the algorithm: https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm

Bilinear filtering:
- Shows how to use bilinear filtering using bilinear interpolation: https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/interpolation/bilinear-filtering
- Shows the principle of bilinear filtering: https://docs.microsoft.com/en-us/windows/win32/direct3d9/bilinear-texture-filtering

Post-process effects:
- Gives some exemples of post-process effects: https://en.wikipedia.org/wiki/Kernel_(image_processing)

MSAA:
- Shows the principle of anti-aliasing: https://docs.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-rasterizer-stage-rules
- Shows how to arrange samples: https://mynameismjp.wordpress.com/2012/10/24/msaa-overview/

***Scene***
===
**_Description:_** Default scene program to load .obj, textures and materials editable via ImGui
(Use stb, TinyObjLoader and ImGui)

1. [Features](#scenefeatures)
2. [Usage](#sceneusage)
3. [Data format](#scenedataformat)


<div id='scenefeatures'/>

# Features
* Load .obj (support textures and materials), quads and triangles with TinyObjLoader
* Load textures
* Load materials
* Sort models with their transform using <algorithm>
* Fully editable lights, materials and objects from ImGui window
* Manage the function calls to the renderer

<div id='sceneusage'/>

# Usage
Initialization
---
```c++
scnImpl* scnCreate()
```
Update
---
```c++
void scnUpdate(scnImpl* scene, float deltaTime, rdrImpl* renderer)

(To sort models)
void scnSetCameraPosition(scnImpl* scene, float* cameraPos)
```
Shutdown
---
```c++
void scnDestroy(scnImpl* scene)
```

Load resources
---
```c++
void loadTriangle(...)
void loadQuad(...)
bool loadObject(...)
int  loadMaterial(...)
int  loadTexture(...)
void loadTriangle(...)
```

<div id='scenedataformat' />

# Data format

Mesh
===
```
vector of Triangles | List of triangles that all have the same texture and material
int textureIndex    | Index of the current texture
int materialIndex   | Index of the current material
```

Object
===
```
bool enable         | Current state
vector of Mesh      | List of mesh that all have the same transform
3 floats -> x, y, z | Position
3 floats -> x, y, z | Rotation
3 floats -> x, y, z | Scale
```

***Shared informations*** (Renderer and scene)
===

Data format
===
Vertex
---
```
3 floats -> x, y, z    | Position
3 floats -> nx, ny, nz | Normal
4 floats -> r, g, b, a | Color
2 floats -> u, v       | Texture coordinates
```

Light
---
```
bool     enabled                | Current state
4 floats position -> x, y, z, w | Position and light type (w)
4 floats ambient  -> r, g, b, a | Ambient color
4 floats diffuse  -> r, g, b, a | Diffuse color
4 floats specular -> r, g, b, a | Specular colors
3 floats attenuation -> c, l, q | Constant, linear and quadratic attenuations
```


Material
---
```
4 floats ambientColor  -> r, g, b, a | Ambient color
4 floats diffuseColor  -> r, g, b, a | Diffuse color
4 floats specularColor -> r, g, b, a | Specular colors
4 floats emissionColor -> r, g, b, a | Emission colors
  float shininess                    | Shininess exponent
```

Texture
---
```
2 ints -> w, h         | Texture size
4 floats -> r, g, b, a | Texture datas

(Only for the scene)
string -> filepath     | Texture file path
```
Rasterization rendering
===
CPU Rendering library written in C++ 17 for C/C++ and scene loader (using stb, TinyObjLoader, glfw and ImGui) accompanied by a mathematics library.

**/!\\ CPU Renderers are not performant nor efficient, it is not recommended to use them on a serious project. Use it at your own risk. /!\\**

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
4. [Exemples and references](#rdrexemples)


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
First of all, the renderer takes the input vertices and applies the vertex shader to them. The vertex shader compute the clip coordinates (which are homogeneous) with the Model-View-Projection matrix (it also applies the Model matrix to the normals and the local coordinates). It saves the vertex informations (like colors and UVs) in a varying (for each vertex) for further operations and to calculate lighting.

<div id='clipping' />

Outcodes and outpoints computing - Clipping
---
After that the pipeline calls two functions to check if the current triangle needs to be clipped, and how to clip it. If it shoulds be clipped, it adds new triangles to rasterize with new varyings (for each vertex).

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
At the start of this step the bounding boxe of the current triangle is calculated. For each of its pixel, his weight is computed to check if it is in the triangle or not (If MSAA is enabled, this step uses the samples of the pixel instead of using the pixel centroid). After passing this test, the depth test should be passed, it checks if there is already a pixel drawn in the color buffer at his position and if his depth is greater than its own. Then the perspective correction is occured to avoid PS1 graphics-like and get correct weights to interpolate varyings. 

<div id='pshader' />

Pixel shader
---
After getting the interpolate varying, the fragment shader is called. The fragment shader calculates the pixel color using the differents values of the inputs. The lighting can be calculated here (If the Phong model is enabled, else it is calculates during Vertex shader). If the current triangle is textured, the fragment shader get the appropriate color using the UVs (It can also filter the texture using bilinear . interpolation). The fragment shader can also discard pixels depending on its settings.

<div id='blending' />

Blending
---
After getting the fragment color, the blending can be applied with the old color of the pixel to have a transparency effect. Then the alpha test should be passed to write in the depth buffer (to avoid non-transparent face due to the rendering order of the vertices). These two steps are applied to each sample of the current pixel if the MSAA is enabled. After these steps the buffers (color buffer or MSAA color buffer) can be filled with the blended color.

<div id='post-process' />

Post-process
---
The final step is to apply effects on the frame buffer after getting all pixels (or of the samples) color, by traversing all the pixels of the frame buffer. If the MSAA is enabled, no pixel has a color, this color is obtained by calculating the average color of all samples of the current pixel. Then others effects can be applied like Box blur, Gaussian blur or Light bloom. These effects are applied by obtaining the average of pixels around the current one with some factors. At the very end, the frame buffer is traversed once more to apply the gamma correction.

<div id='rdrexemples' />

# Exemples and references

***Scene***
===
**_Description:_** Default scene program to load .obj, textures and materials editable via ImGui
(Use stb, TinyObjLoader and ImGui)

Features
===
* Load .obj (support textures and materials), quads and triangles
* Load textures
* Load materials
* Sort models with their transform
* Fully editable lights, materials and objects from ImGui window
* Manage the function calls to the renderer

Usage
===
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
4 floats position -> x, y, z, w | Position
4 floats ambient  -> r, g, b, a | Ambient color
4 floats diffuse  -> r, g, b, a | Diffuse color
4 floats specular -> r, g, b, a | Specular colors
3 floats attenuation -> c, l, q | Constant, linear and quadratic attenuations
```


Material
---
```
4 floats ambientColor  -> r, g, b, a    | Ambient color
4 floats diffuseColor  -> r, g, b, a    | Diffuse color
4 floats specularColor -> r, g, b, a    | Specular colors
4 floats emissionColor -> r, g, b, a    | Emission colors
  float shininess                       | Shininess exponent
```

Texture
---
```
2 ints -> w, h          | Texture size
4 floats -> r, g, b, a  | Texture datas

(Only for the scene)
string -> filepath      | Texture file path
```
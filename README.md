Rasterization rendering
===
CPU Rendering library written in C++ 17 for C/C++ and scene loader (using stb, TinyObjLoader, glfw and ImGui) accompanied by a mathematics library.

**/!\\ CPU Renderers are not performant nor efficient, it is not recommended to use them on a serious project. Use it at your own risk. /!\\**

***Renderer***
===
**_Description:_**
CPU Renderer using rasterization to display 3D models in a color buffer
(Use glfw and ImGui)

Features
===
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
* Post-process effect (Box blur, Gaussian blur and Light bloom)

Usage
===
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

How does it work ?
===
Vertex shader
---
First of all, the renderer takes the input vertices and applies the vertex shader to them. The vertex shader compute the clip coordinates (which are homogeneous) with the Model-View-Projection matrix (it also applies the Model matrix to the normals and the local coordinates). It saves the vertex informations (like colors and UVs) in a varying (fore ach vertex) for further operations and calculates lighting.

Outcodes and outpoints computing - Clipping
---
After that the pipeline calls two functions to check if the current triangle needs to be clipped, and how to clip it. If it shoulds be clipped, it adds new triangle to rasterize with new varying (for each vertex) values.

Normalized Device Coordinates
---
Then the renderer divides the homogeneous coordinates by their w component to get NDC coordinates (which are between -1 and 1). With these coordinates the renderer can ignore some faces (like back faces) using the normal of the triangle's face.

Screen coordinates
---
NDC coordinates are then remapped with the viewport definition, and the varyings are interpolated with the weights of clipped coordinates. With these remapped coordinates and the interpolated varyings the main function can finally rasterize the clipped triangles (or draw them as lines with the Wireframe mode).

Rasterization
---
At the start of this step the bounding boxe of the input triangle is calculated. For each of its pixel, his weight is computed to check if it is in the triangle or not. After passing this test, the depth test should be passed, it checks if there is already a pixel drawn in the color buffer at his position and if his depth is greater than its own. Then the perspective correction is occured to avoid PS1 graphics-like and get correct weights to interpolate varyings. 

Pixel shader
===
After getting varyings 

Blending
===

Post-process
===
The final step is 



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

Data format
===

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
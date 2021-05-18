//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
};

struct VertexBufferAttribute { u32 id; u32 quantity; u32 stride; };

struct VertexBufferLayout {
    std::vector<VertexBufferAttribute>  attributes;
    u32 stride;
};

struct Submesh
{
    std::vector<f32>  vertices;
    std::vector<u32>  indices;
    u32 vertexOffset;
    u32 indexOffset;
    VertexBufferLayout vertexBufferLayout;
};

struct Mesh
{
    std::vector<Submesh>  submeshes;
    std::vector<f32>  vertices;
    std::vector<u32>  indices;

    GLuint vertexArrayHandle;
    GLuint vertexBufferHandle;
    GLuint indexBufferHandle;
};

struct Material
{
    std::string name;
    vec3 albedo;
    vec3 emissive;
    f32 smoothness;
    u32 albedoTextureIdx;
    u32 emissiveTextureIdx;
    u32 specularTextureIdx;
    u32 normalsTextureIdx;
    u32 bumpTextureIdx;
};

struct Model
{
    u32 meshIdx;
    std::vector<u32> materialIdx;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Count
};

enum LightType : int
{
    L_DIRECTIONAL = 0,
    L_POINT,
    L_SPOTLIGHT
};

struct Light
{
    LightType type = L_POINT;

    // Attenuattion
    float intensity = 1.0f;
    float constant = 1.0f;
    float linear = 0.091f;
    float quadratic = 0.011f;

    // color
    vec3 diffuse; // 0.8
    float specular = 0.2f;

    // Spotlight
    float cutOff[2]; // cos(radians(12.5f))
    float outerCutOff[2]; // cos(radians(17.5f))
};

struct ModelSceneObject {
    glm::mat4x4 transform;
    u32 modelIdx;
};

struct LightSceneObject {
    vec3 position;
    vec3 direction;
    Light light;
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];

    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Program>  programs;
    std::vector<Mesh>  meshes;
    std::vector<Material>  materials;
    std::vector<Model>  models;

    std::vector<ModelSceneObject>  modelSceneObjects;
    std::vector<LightSceneObject>  lightSceneObjects;

    // program indices
    u32 texturedGeometryProgramIdx;
    
    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    // Mode
    Mode mode;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programGeoPass;
    GLuint programLightPass;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao, vbo, ebo;
};

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);


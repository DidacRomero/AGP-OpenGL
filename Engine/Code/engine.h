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

enum Mode
{
    Mode_TexturedQuad,
    Mode_Count,
    Mode_Model
};

struct OpenGLInfo 
{
    char renderer[64];
    char openGlVersion[64];
    char vendor[64];
    char glslVer[64];
};

struct VertexV3V2
{
    glm::vec3 pos;
    glm::vec2 uv;
};

const VertexV3V2 vertices[] = {
    { glm::vec3(-0.5, -0.5, 0.0), glm::vec2(0.0,0.0) },   //Bottom left vertex
    { glm::vec3(0.5, -0.5, 0.0), glm::vec2(1.0, 0.0) },   //Bottom right vertex
    { glm::vec3(0.5, 0.5, 0.0), glm::vec2(1.0, 1.0) },   //Top right vertex
    { glm::vec3(-0.5, 0.5, 0.0), glm::vec2(0.0, 1.0) },  //Top left vertex
};

const u16 indices[] = {
    0, 1, 2,
    0, 2, 3
};

struct VertexBufferAttribute
{
    u8 location;
    u8 componentCount;
    u8 offset;
};

struct VertexBufferLayout
{
    std::vector<VertexBufferAttribute> attributes;
    u8 stride;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; 
    VertexBufferLayout vertexInputLayout;
};

struct VertexShaderAttribute
{
    u8 location;
    u8 componentCount;

    std::string Name;
};

struct VertexShaderLayout
{
    std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
    GLuint handle;
    GLuint programHandle;
};

struct Submesh
{
    VertexBufferLayout vertexBufferLayout;
    std::vector<float> vertices;
    std::vector <u32 > indices;
    u32 vertexOffset;
    u32 indexOffset;

    std::vector<Vao> vaos;
};

struct Mesh
{
    std::vector<Submesh> submeshes;
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
struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    OpenGLInfo oGlI;

    ivec2 displaySize;

    //Models & materials
    std::vector<Texture> textures;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;
    std::vector<Model> models;
    std::vector<Program> programs;

    //Aux
    u32 model;

    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;
    
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
    GLuint programUniformTexture;
    GLuint texturedMeshProgram_uTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;
};

class OpenGLErrorGuard
{
public:
    OpenGLErrorGuard(const char* message) : msg(message) {
        checkGLError("BEGIN", msg);
    }

    ~OpenGLErrorGuard()
    {
        checkGLError("END", msg);
    }
    const char* msg;
    //Catching message and managing it as we want
    static void checkGLError(const char* around, const char* message);
};

OpenGLInfo GetOpenGlInfo();

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program);

u32 LoadTexture2D(App* app, const char* filepath);

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);


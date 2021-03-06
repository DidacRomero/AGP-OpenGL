//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include "../assimp_model_loading.h"

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
    

    GLint attributeCount;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    GLchar attributeName[128];
    GLsizei attributeNameLength;
    GLint attributeSize;
    GLenum attributeType;

    for (int i = 0; i < attributeCount; ++i)
    {
        glGetActiveAttrib(program.handle, i, 128,
            &attributeNameLength,
            &attributeSize,
            &attributeType,
            attributeName);

        u8 attribute= glGetAttribLocation(program.handle, attributeName);

        program.vertexInputLayout.attributes.push_back({ attribute, u8(attributeSize) });
    }

    app->programs.push_back(program);
    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

//Get the OPENGL hardware info
OpenGLInfo GetOpenGlInfo()
{
    OpenGLErrorGuard guard("GLInfo()");
    OpenGLInfo ret;
    strcpy(ret.openGlVersion, (const char*)glGetString(GL_VERSION));
    strcpy(ret.vendor, (const char*)glGetString(GL_VENDOR));
    strcpy(ret.renderer, (const char*)glGetString(GL_RENDERER));
    strcpy(ret.glslVer, (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
    //ret.gpuName = (char)glGetString(GL_VENDOR);


    return ret;
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    //Try finding a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;

    GLuint vaoHandle = 0;

    //Create a new vao for this submesh/program
    {
        glGenVertexArrays(1, &vaoHandle);
        glBindVertexArray(vaoHandle);

        glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

        //WE have to link all vertex inputs attributes to attributes in the vertex buffer
        for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
        {
            bool attributeWasLinked = false;

            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
            {
                if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
                {
                    const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                    const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                    const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset; //atribute offset + vertex offset
                    const u32 stride = submesh.vertexBufferLayout.stride;
                    glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                    glEnableVertexAttribArray(index);

                    attributeWasLinked = true;
                    break;
                }
            }

            assert(attributeWasLinked); // the submesh should provide an attribute for each vertex inputs
        }

        glBindVertexArray(0);
        
    }
    //Store it in the list of vaos for this submesh
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

void Init(App* app)
{
    OpenGLErrorGuard guard("Init: ");
    //Set GL_KHR_debug - debug callback
    if (GLVersion.major > 4 || (GLVersion.major == 4 && GLVersion.minor >= 3))
    {
        glDebugMessageCallback(OnGlError, app);
    }

    //Get the OpenGL info
     app->oGlI =  GetOpenGlInfo();

     glEnable(GL_DEPTH_TEST);
     // We only need to do this once
     glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
     glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT,&app->uniformBlockAlignment);

     GLuint bufferHandle;
     glGenBuffers(1, &bufferHandle);
     glBindBuffer(GL_UNIFORM_BUFFER, bufferHandle);
     glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
     glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // TODO: Initialize your resources here!
    // - vertex buffers
     glGenBuffers(1, &app->embeddedVertices);
     glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
     glBindBuffer(GL_ARRAY_BUFFER, 0);

     // - element/index buffers
     glGenBuffers(1, &app->embeddedElements);
     glBindBuffer(GL_ARRAY_BUFFER, app->embeddedElements);
     glBufferData(GL_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
     glBindBuffer(GL_ARRAY_BUFFER, 0);

    // - vaos
     glGenVertexArrays(1, &app->vao);
     glBindVertexArray(app->vao);
     glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
     glEnableVertexAttribArray(0);
     glVertexAttribPointer(1,2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
     glEnableVertexAttribArray(1);
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
     glBindVertexArray(0);

    // - programs (and retrieve uniform indices)
     app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
     Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
     app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

     // - textures
     app->diceTexIdx = LoadTexture2D(app, "dice.png");
     app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
     app->blackTexIdx = LoadTexture2D(app, "color_black.png");
     app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
     app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");

     //Meshes
     app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");
     app->model = LoadModel(app, "Patrick/Patrick.obj");
     
    app->mode = Mode_TexturedModel;
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::End();

    ImGui::Begin("OpenGL");
    ImGui::Text("OpenGL version: %s", app->oGlI.openGlVersion);
    ImGui::Text("OpenGL renderer: %s", app->oGlI.renderer);
    ImGui::Text("OpenGL vendor: %s", app->oGlI.vendor);
    ImGui::Text("OpenGL GLSL version: %s", app->oGlI.glslVer);
    ImGui::End();

    //Print OpenGl info
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here

    //Hot Reload
    for (u64 i = 0; i < app->programs.size(); ++i)
    {
        Program& program = app->programs[i];
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());
        if (currentTimestamp > program.lastWriteTimestamp)
        {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = currentTimestamp;
        }
    }

    /*glBindBuffer(GL_UNIFORM_BUFFER, bufferHandle);
    u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    u32 bufferHead = 0;

    memcpy( bufferData + bufferHead, glm::value_ptr( worldMatrix), sizeof(glm::mat4));
    bufferHead += sizeof(glm::mat4);

    memcpy(bufferData + bufferHead, glm::value_ptr(worldViewProjectionMatrix), sizeof(glm::mat4));
    bufferHead += sizeof(glm::mat4);

    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER,0);*/

}

void Render(App* app)
{
    OpenGLErrorGuard guard("blur()");
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                // TODO: Draw your textured quad here!
                // - clear the framebuffer
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                // - set the viewport
            glViewport(0, 0, app->displaySize.x, app->displaySize.y);
            // - bind the program 
            Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
            glUseProgram(programTexturedGeometry.handle);
            // - bind the vao
            glBindVertexArray(app->vao);
                // - set the blending state
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                // - bind the texture into unit 0
            glUniform1i(app->programUniformTexture, 0);
            glActiveTexture(GL_TEXTURE0);
            GLuint textureHandle = app->textures[app->diceTexIdx].handle;
            glBindTexture(GL_TEXTURE_2D, textureHandle);

                // - glDrawElements() !!!
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

            glBindVertexArray(0);
            glUseProgram(0);
            }
            break;

        case Mode::Mode_TexturedModel:
            {
                glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
                glUseProgram(texturedMeshProgram.handle);

                Model& model = app->models[app->model];
                Mesh& mesh = app->meshes[model.meshIdx];

                for (u32 i = 0; i < mesh.submeshes.size(); ++i)
                {
                    GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
                    glBindVertexArray(vao);

                    u32 submeshMaterialIdx = model.materialIdx[i];
                    Material& submeshMaterial = app->materials[submeshMaterialIdx];

                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
                    glUniform1i(app->texturedMeshProgram_uTexture, 0);

                    Submesh& submesh = mesh.submeshes[i];
                    glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
                }
            }
            break;

        default:;
    }
}

void OpenGLErrorGuard::checkGLError(const char* around, const char* message)
{
    GLenum error;
    do {
        error = glGetError();
        if (error != GL_NO_ERROR)
        {
            //Handling error
        }
    } while (error != GL_NO_ERROR /*&& error != GL_CONTEXT_LOST*/);
}

void OnGlError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    ELOG("OpenGL debug message: %s", message);
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:                                          ELOG(" - source: GL_DEBUG_SOURCE_API"); break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:           ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:           ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:                   ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break;
        case GL_DEBUG_SOURCE_APPLICATION:                   ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break;
        case GL_DEBUG_SOURCE_OTHER:                               ELOG(" - source: GL_DEBUG_SOURCE_OTHER"); break;
    }
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:                                                        ELOG(" - source: GL_DEBUG_TYPE_ERROR"); break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:                         ELOG(" - source: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:                           ELOG(" - source: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break;
        case GL_DEBUG_TYPE_PORTABILITY:                                           ELOG(" - source: GL_DEBUG_TYPE_PORTABILITY"); break;
        case GL_DEBUG_TYPE_PERFORMANCE:                                         ELOG(" - source: GL_DEBUG_TYPE_PERFORMANCE"); break;
        case GL_DEBUG_TYPE_MARKER:                                                    ELOG(" - source: GL_DEBUG_TYPE_MARKER"); break;
        case GL_DEBUG_TYPE_PUSH_GROUP:                                             ELOG(" - source: GL_DEBUG_TYPE_PUSH_GROUP"); break;
        case GL_DEBUG_TYPE_POP_GROUP:                                                 ELOG(" - source: GL_DEBUG_TYPE_POP_GROUP"); break;
        case GL_DEBUG_TYPE_OTHER:                                                        ELOG(" - source: GL_DEBUG_TYPE_OTHER"); break;
    }

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:                                    ELOG(" - source: GL_DEBUG_SEVERITY_HIGH"); break;
        case GL_DEBUG_SEVERITY_MEDIUM:                              ELOG(" - source: GL_DEBUG_SEVERITY_MEDIUM"); break;
        case GL_DEBUG_SEVERITY_LOW:                                     ELOG(" - source: GL_DEBUG_SEVERITY_LOW"); break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:                  ELOG(" - source: GL_DEBUG_SEVERITY_NOTIFICATION"); break;
    }
};
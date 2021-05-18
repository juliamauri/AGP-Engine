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


#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


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
    GLenum dataFormat = GL_RGB;
    GLenum dataType = GL_UNSIGNED_BYTE;

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

void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
    std::vector<float> vertices;
    std::vector<u32> indices;

    bool hasTexCoords = false;
    bool hasTangentSpace = false;

    // process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        vertices.push_back(mesh->mNormals[i].x);
        vertices.push_back(mesh->mNormals[i].y);
        vertices.push_back(mesh->mNormals[i].z);

        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            hasTexCoords = true;
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        }

        if (mesh->mTangents != nullptr && mesh->mBitangents)
        {
            hasTangentSpace = true;
            vertices.push_back(mesh->mTangents[i].x);
            vertices.push_back(mesh->mTangents[i].y);
            vertices.push_back(mesh->mTangents[i].z);

            // For some reason ASSIMP gives me the bitangents flipped.
            // Maybe it's my fault, but when I generate my own geometry
            // in other files (see the generation of standard assets)
            // and all the bitangents have the orientation I expect,
            // everything works ok.
            // I think that (even if the documentation says the opposite)
            // it returns a left-handed tangent space matrix.
            // SOLUTION: I invert the components of the bitangent here.
            vertices.push_back(-mesh->mBitangents[i].x);
            vertices.push_back(-mesh->mBitangents[i].y);
            vertices.push_back(-mesh->mBitangents[i].z);
        }
    }

    // process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // store the proper (previously proceessed) material for this mesh
    submeshMaterialIndices.push_back(baseMeshMaterialIndex + mesh->mMaterialIndex);

    // create the vertex format
    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
    vertexBufferLayout.stride = 6 * sizeof(float);
    if (hasTexCoords)
    {
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexBufferLayout.stride });
        vertexBufferLayout.stride += 2 * sizeof(float);
    }
    if (hasTangentSpace)
    {
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, vertexBufferLayout.stride });
        vertexBufferLayout.stride += 3 * sizeof(float);

        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, vertexBufferLayout.stride });
        vertexBufferLayout.stride += 3 * sizeof(float);
    }

    // add the submesh into the mesh
    Submesh submesh = {};
    submesh.vertexBufferLayout = vertexBufferLayout;
    submesh.vertices.swap(vertices);
    submesh.indices.swap(indices);
    myMesh->submeshes.push_back(submesh);
}

void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory)
{
    aiString name;
    aiColor3D diffuseColor;
    aiColor3D emissiveColor;
    aiColor3D specularColor;
    ai_real shininess;
    material->Get(AI_MATKEY_NAME, name);
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
    material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
    material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
    material->Get(AI_MATKEY_SHININESS, shininess);

    myMaterial.name = name.C_Str();
    myMaterial.albedo = vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
    myMaterial.emissive = vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
    myMaterial.smoothness = shininess / 256.0f;

    aiString aiFilename;
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        material->GetTexture(aiTextureType_DIFFUSE, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.albedoTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
    {
        material->GetTexture(aiTextureType_EMISSIVE, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.emissiveTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
    {
        material->GetTexture(aiTextureType_SPECULAR, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.specularTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
    {
        material->GetTexture(aiTextureType_NORMALS, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.normalsTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_HEIGHT) > 0)
    {
        material->GetTexture(aiTextureType_HEIGHT, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.bumpTextureIdx = LoadTexture2D(app, filepath.str);
    }

    //myMaterial.createNormalFromBump();
}

void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessAssimpMesh(scene, mesh, myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
    }

    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessAssimpNode(scene, node->mChildren[i], myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
    }
}

u32 LoadModel(App* app, const char* filename)
{
    const aiScene* scene = aiImportFile(filename,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_SortByPType);

    if (!scene)
    {
        ELOG("Error loading mesh %s: %s", filename, aiGetErrorString());
        return UINT32_MAX;
    }

    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    u32 meshIdx = (u32)app->meshes.size() - 1u;

    app->models.push_back(Model{});
    Model& model = app->models.back();
    model.meshIdx = meshIdx;
    u32 modelIdx = (u32)app->models.size() - 1u;

    String directory = GetDirectoryPart(MakeString(filename));

    // Create a list of materials
    u32 baseMeshMaterialIndex = (u32)app->materials.size();
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        app->materials.push_back(Material{});
        Material& material = app->materials.back();
        ProcessAssimpMaterial(app, scene->mMaterials[i], material, directory);
    }

    ProcessAssimpNode(scene, scene->mRootNode, &mesh, baseMeshMaterialIndex, model.materialIdx);

    aiReleaseImport(scene);

    u32 vertexBufferSize = 0;
    u32 indexBufferSize = 0;

    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
    {
        vertexBufferSize += mesh.submeshes[i].vertices.size() * sizeof(float);
        indexBufferSize += mesh.submeshes[i].indices.size() * sizeof(u32);
    }

    glGenVertexArrays(1, &mesh.vertexArrayHandle);
    glBindVertexArray(mesh.vertexArrayHandle);

    glGenBuffers(1, &mesh.vertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.indexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

    u32 indicesOffset = 0;
    u32 verticesOffset = 0;

    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
    {
        const void* verticesData = mesh.submeshes[i].vertices.data();
        const u32   verticesSize = mesh.submeshes[i].vertices.size() * sizeof(float);
        glBufferSubData(GL_ARRAY_BUFFER, verticesOffset, verticesSize, verticesData);
        mesh.submeshes[i].vertexOffset = verticesOffset;
        verticesOffset += verticesSize;

        const void* indicesData = mesh.submeshes[i].indices.data();
        const u32   indicesSize = mesh.submeshes[i].indices.size() * sizeof(u32);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indicesOffset, indicesSize, indicesData);
        mesh.submeshes[i].indexOffset = indicesOffset;
        indicesOffset += indicesSize;
    }

    if (mesh.submeshes.size() > 0)
    {
        for (u32 i = 0; i < mesh.submeshes[0].vertexBufferLayout.attributes.size(); ++i) {

            glEnableVertexAttribArray(mesh.submeshes[0].vertexBufferLayout.attributes[i].id);
            glVertexAttribPointer(mesh.submeshes[0].vertexBufferLayout.attributes[i].id, mesh.submeshes[0].vertexBufferLayout.attributes[i].quantity, GL_FLOAT, GL_FALSE, mesh.submeshes[0].vertexBufferLayout.stride, reinterpret_cast<void*>(mesh.submeshes[0].vertexBufferLayout.attributes[i].stride));
        }
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return modelIdx;
}

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
    app->programs.push_back(program);

    return app->programs.size() - 1;
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    //Deferred FBO Setup
    u32 width = app->deferredFBO.width = app->displaySize.x;
    u32 height =  app->deferredFBO.height = app->displaySize.y;
    glGenFramebuffers(1, &app->deferredFBO.ID);
    glBindFramebuffer(GL_FRAMEBUFFER, app->deferredFBO.ID);

    // position
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    app->deferredFBO.texturesID.push_back(tex);

    // normal
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, tex, 0);
    app->deferredFBO.texturesID.push_back(tex);

    // Albedo
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, tex, 0);
    app->deferredFBO.texturesID.push_back(tex);

    // Specular + Shininess + Alpha
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, tex, 0);
    app->deferredFBO.texturesID.push_back(tex);

    // Result
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, tex, 0);
    app->deferredFBO.texturesID.push_back(tex);

    // Bind Attachments
    unsigned int attachments[5] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };
    glDrawBuffers(5, attachments);


    glGenTextures(1, &app->deferredFBO.depthBufferTexture);
    glBindTexture(GL_TEXTURE_2D, app->deferredFBO.depthBufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, app->deferredFBO.depthBufferTexture, 0);

    glGenRenderbuffers(1, &app->deferredFBO.depthBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, app->deferredFBO.depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, app->deferredFBO.depthBuffer);

    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //Light Sphere
    u32 lats = 40, longs = 40;
    int i, j;
    std::vector<GLfloat> vertices;
    std::vector<GLuint> indices;
    int indicator = 0;
    for (i = 0; i <= lats; i++) {
        double lat0 = glm::pi<double>() * (-0.5 + (double)(i - 1) / lats);
        double z0 = sin(lat0);
        double zr0 = cos(lat0);

        double lat1 = glm::pi<double>() * (-0.5 + (double)i / lats);
        double z1 = sin(lat1);
        double zr1 = cos(lat1);

        for (j = 0; j <= longs; j++) {
            double lng = 2 * glm::pi<double>() * (double)(j - 1) / longs;
            double x = cos(lng);
            double y = sin(lng);

            vertices.push_back(x * zr0);
            vertices.push_back(y * zr0);
            vertices.push_back(z0);
            indices.push_back(indicator);
            indicator++;

            vertices.push_back(x * zr1);
            vertices.push_back(y * zr1);
            vertices.push_back(z1);
            indices.push_back(indicator);
            indicator++;
        }
        indices.push_back(GL_PRIMITIVE_RESTART_FIXED_INDEX);
    }

    glGenVertexArrays(1, &app->Svao);
    glBindVertexArray(app->Svao);

    glGenBuffers(1, &app->Svbo);
    glBindBuffer(GL_ARRAY_BUFFER, app->Svbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &app->Sebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->Sebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

    app->Stri = indices.size();

    //Quad buffer
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane
    glGenVertexArrays(1, &app->vao);
    glGenBuffers(1, &app->vbo);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    app->mode = Mode_TexturedQuad;


    app->cam.cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    app->cam.cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    app->cam.cameraDirection = glm::normalize(app->cam.cameraPos - app->cam.cameraTarget);


    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    app->cam.cameraRight = glm::normalize(glm::cross(up, app->cam.cameraDirection));
    app->cam.cameraFront = vec3(0.0f, 0.0f, -1.0f);
    app->cam.cameraUp = vec3(0.0f, 1.0f, 0.0f);

    u32 mLoaded = LoadModel(app, "Patrick\\Patrick.obj");

    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, vec3(0.0));

    app->modelSceneObjects.push_back(ModelSceneObject());
    ModelSceneObject& sobj = app->modelSceneObjects.back();

    sobj.modelIdx = mLoaded;
    sobj.transform = trans;

    app->lightSceneObjects.push_back(LightSceneObject());
    LightSceneObject& lsObj = app->lightSceneObjects.back();
    lsObj.position = vec3(0.0f, 5.0f, 0.0f);
    lsObj.direction = vec3(0.0f, -1.0f, 0.0f);
    lsObj.light.type = L_POINT;
    lsObj.light.intensity = 1.0f;
    lsObj.light.constant = 1.0f;
    lsObj.light.linear = 1.0f;
    lsObj.light.quadratic = 1.0f;
    lsObj.light.diffuse = vec3(0.8f);
    lsObj.light.specular = 0.2f;
    lsObj.light.cutOff[0] = 12.5f;
    lsObj.light.outerCutOff[0] = 17.5f;
    lsObj.light.cutOff[1] = glm::cos(glm::radians(lsObj.light.cutOff[0]));
    lsObj.light.outerCutOff[1] = glm::cos(glm::radians(lsObj.light.outerCutOff[0]));

    app->programGeoPass = LoadProgram(app, "GeoPassShader.glsl", "GEOMETRY_PASS");
    app->programLightPass = LoadProgram(app, "LightPassShader.glsl", "LIGHT_PASS");
    app->programquadReder = LoadProgram(app, "QuadRender.glsl", "QUAD_RENDER");

}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f/app->deltaTime);
    ImGui::Combo("Select Texture", &app->textureOutputType, "Position\0Normal\0Albedo\0Final\0Depth\0");
    ImGui::TextWrapped("Everything works correctly but the final render do not display anything");
    ImGui::End();
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
}

void Render(App* app)
{
    switch (app->mode)
    {
        case Mode_TexturedQuad:
            {
                // TODO: Draw your textured quad here!
                // - clear the framebuffer
                // - set the viewport
                // - set the blending state
                // - bind the texture into unit 0
                // - bind the program 
                //   (...and make its texture sample from unit 0)
                // - bind the vao
                // - glDrawElements() !!!

                glBindFramebuffer(GL_FRAMEBUFFER, app->deferredFBO.ID);
                glViewport(0, 0, app->deferredFBO.width, app->deferredFBO.height);

                //Geometry Pass
                glUseProgram(app->programGeoPass);

                glm::mat4 view;
                view = glm::lookAt(app->cam.cameraPos, app->cam.cameraPos + app->cam.cameraFront, app->cam.cameraUp);
                glUniformMatrix4fv(glGetUniformLocation(app->programGeoPass, "view"), 1, GL_FALSE, glm::value_ptr(view));

                glm::mat4 projection;
                projection = glm::perspective(glm::radians(90.0f), float(app->deferredFBO.width) / float(app->deferredFBO.height), 0.1f, 100.0f);
                glUniformMatrix4fv(glGetUniformLocation(app->programGeoPass, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

                for (u32 i = 0; i < app->modelSceneObjects.size(); i++)
                {
                    glUniformMatrix4fv(glGetUniformLocation(app->programGeoPass, "model"), 1, GL_FALSE, glm::value_ptr(app->modelSceneObjects[i].transform));

                    Model& mod = app->models[app->modelSceneObjects[i].modelIdx];
                    Mesh& mesh = app->meshes[mod.meshIdx];
                    

                    if (mod.materialIdx.size() > 0)
                    {
                        Material& mat = app->materials[mod.materialIdx[0]];

                        u32 texCount = 0;
                        if (mat.albedoTextureIdx > 0) 
                        {
                            glUniform1f(glGetUniformLocation(app->programGeoPass, "useTexture"), 1.0f);

                            glActiveTexture(GL_TEXTURE0 + texCount);
                            glUniform1i(glGetUniformLocation(app->programLightPass, "tdiffuse"), texCount);
                            glBindTexture(GL_TEXTURE_2D, mat.albedoTextureIdx);
                            texCount++;


                            if (mat.specularTextureIdx > 0)
                            {
                                glActiveTexture(GL_TEXTURE0 + texCount);
                                glUniform1i(glGetUniformLocation(app->programLightPass, "tspecular"), texCount);
                                glBindTexture(GL_TEXTURE_2D, mat.specularTextureIdx);
                            }
                        }
                        else
                            glUniform1f(glGetUniformLocation(app->programGeoPass, "useTexture"), 0.0f);

                        if (mat.albedo.length() > 0.0f)
                        {
                            glUniform1f(glGetUniformLocation(app->programGeoPass, "useColor"), 1.0f);
                            glUniform3f(glGetUniformLocation(app->programGeoPass, "albedo"), mat.albedo.x, mat.albedo.y, mat.albedo.z);
                            
                            if (mat.emissive.length() > 0.0f)
                                glUniform3f(glGetUniformLocation(app->programGeoPass, "emissive"), mat.emissive.x, mat.emissive.y, mat.emissive.z);
                            
                            glUniform1f(glGetUniformLocation(app->programGeoPass, "smoothness"), mat.smoothness);
                        }
                        else
                            glUniform1f(glGetUniformLocation(app->programGeoPass, "useColor"), 0.0f);

                    }
                    
                    glBindVertexArray(mesh.vertexArrayHandle);
                    glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, nullptr);
                }

                for (u32 i = 0; i < app->lightSceneObjects.size(); i++)
                {
                    if (app->lightSceneObjects[i].light.type != LightType::L_DIRECTIONAL)
                    {
                        glm::mat4 trans = glm::mat4(1.0f);
                        trans = glm::translate(trans, app->lightSceneObjects[i].position);
                        glUniformMatrix4fv(glGetUniformLocation(app->programGeoPass, "model"), 1, GL_FALSE, glm::value_ptr(trans));

                        glUniform1f(glGetUniformLocation(app->programGeoPass, "useColor"), 1.0f);
                        glUniform1f(glGetUniformLocation(app->programGeoPass, "useTexture"), 0.0f);
                        glUniform3f(glGetUniformLocation(app->programGeoPass, "albedo"), 1.0f, 0.0f, 0.0f);
                        glUniform3f(glGetUniformLocation(app->programGeoPass, "emissive"), 1.0f, 0.0f, 0.0f);
                        glUniform1f(glGetUniformLocation(app->programGeoPass, "smoothness"), 32.0f / 256.0f);

                        // draw sphere
                        glBindVertexArray(app->Svao);
                        glEnable(GL_PRIMITIVE_RESTART);
                        glPrimitiveRestartIndex(GL_PRIMITIVE_RESTART_FIXED_INDEX);
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->Sebo);
                        glDrawElements(GL_QUAD_STRIP, app->Stri, GL_UNSIGNED_INT, NULL);
                    }
                }

                //LightPass
                glUseProgram(app->programLightPass);

                glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

                //UploadLights

                // Bind Textures
                static const std::string deferred_textures[4] = { "gPosition", "gNormal", "gAlbedo", "gSpec" };
                for (unsigned int count = 0; count < 4; ++count)
                {
                    glActiveTexture(GL_TEXTURE0 + count);
                    glUniform1i(glGetUniformLocation(app->programLightPass, deferred_textures[count].c_str()), count);
                    glBindTexture(GL_TEXTURE_2D, app->deferredFBO.texturesID[count]);
                }

                u32 lCount = 0;

                std::string unif_name;
                for (u32 i = 0; i < app->lightSceneObjects.size(); i++) {
                    unif_name = "lights[" + std::to_string(lCount) + "].";

                    glUniform4f(glGetUniformLocation(app->programLightPass, (unif_name + "directionIntensity").c_str()), app->lightSceneObjects[i].direction.x, app->lightSceneObjects[i].direction.y, app->lightSceneObjects[i].direction.z, app->lightSceneObjects[i].light.intensity);
                    glUniform4f(glGetUniformLocation(app->programLightPass, (unif_name + "diffuseSpecular").c_str()), app->lightSceneObjects[i].light.diffuse.x, app->lightSceneObjects[i].light.diffuse.y, app->lightSceneObjects[i].light.diffuse.z, app->lightSceneObjects[i].light.specular);
                    
                    if (app->lightSceneObjects[i].light.type != L_DIRECTIONAL)
                    {
                        glUniform4f(glGetUniformLocation(app->programLightPass, (unif_name + "positionType").c_str()), app->lightSceneObjects[i].position.x, app->lightSceneObjects[i].position.y, app->lightSceneObjects[i].position.z, float(app->lightSceneObjects[i].light.type));
                        glUniform4f(glGetUniformLocation(app->programLightPass, (unif_name + "clq").c_str()), app->lightSceneObjects[i].light.constant, app->lightSceneObjects[i].light.linear, app->lightSceneObjects[i].light.quadratic, 0.0f);


                        if (app->lightSceneObjects[i].light.type == L_SPOTLIGHT)
                            glUniform4f(glGetUniformLocation(app->programLightPass, (unif_name + "co").c_str()), app->lightSceneObjects[i].light.cutOff[1], app->lightSceneObjects[i].light.outerCutOff[1], 0.0f, 0.0f);
                    }
                    else
                        glUniform4f(glGetUniformLocation(app->programLightPass, (unif_name + "positionType").c_str()), 0.0f, 0.0f, 0.0f, float(app->lightSceneObjects[i].light.type));


                    lCount++;
                    if (lCount == 203) break;
                }

                glUniform1i(glGetUniformLocation(app->programLightPass, "count"), lCount);
                vec3 cameraPos = app->cam.cameraPos;
                glUniform3f(glGetUniformLocation(app->programLightPass, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);

                // Render Quad
                glBindVertexArray(app->vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

                glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);

                // Show Final Texture
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glViewport(0, 0, app->displaySize.x, app->displaySize.y);

                glUseProgram(app->programquadReder);

                glActiveTexture(GL_TEXTURE0);
                glUniform1i(glGetUniformLocation(app->programLightPass, "uTexture"), 0);

                switch (app->textureOutputType)
                {
                case 0:
                    glBindTexture(GL_TEXTURE_2D, app->deferredFBO.texturesID[0]);
                    break;
                case 1:
                    glBindTexture(GL_TEXTURE_2D, app->deferredFBO.texturesID[1]);
                    break;
                case 2:
                    glBindTexture(GL_TEXTURE_2D, app->deferredFBO.texturesID[2]);
                    break;
                case 3:
                    glBindTexture(GL_TEXTURE_2D, app->deferredFBO.texturesID[4]);
                    break;
                case 4:
                    glBindTexture(GL_TEXTURE_2D, app->deferredFBO.depthBufferTexture);
                    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 0, 0, app->deferredFBO.width, app->deferredFBO.height, 0);
                    break;
                default:
                    break;
                }

                glBindVertexArray(app->vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);
                glUseProgram(0);

            }
            break;

        default:;
    }
}


#pragma once

#include "openglcontext.h"
#include "glm_includes.h"
#include "smartpointerhelp.h"
#include "stb_image.h"
#include "scene/geometry/shape.h"

// Texture slot for the 2D HDR environment map
#define ENV_MAP_FLAT_TEX_SLOT 0
// Texture slot for the 3D HDR environment cube map
#define ENV_MAP_CUBE_TEX_SLOT 1
// Texture slot for the input of the path tracer
#define PATH_TRACER_INPUT_TEX_SLOT 2
// Texture slot for the output of the path tracer
#define PATH_TRACER_OUTPUT_TEX_SLOT 3

// All handle numbers > 3 are reserved for scene Material textures

class Texture {
public:
    Texture(OpenGLContext* context, int arrayIdx, int associatedTexSlot);
    virtual ~Texture();

    virtual void create(const char *texturePath, bool wrap);
    virtual void create(bool wrap) = 0;
    void destroy();
    void bind(GLuint texSlot);

    bool m_isCreated;

    int m_uniformArrayIndex; // Which sampler2D in the shader will read from this texture
    int m_associatedTextureSlot; // Which texture slot this will be put in

protected:
    OpenGLContext* context;
    GLuint m_textureHandle;
    QString m_texturePath;
};

class Texture2D : public Texture {
public:
    Texture2D(OpenGLContext* context, int arrayIdx, int associatedTexSlot);
    ~Texture2D();

    void create(const char *texturePath, bool wrap) override;
    void create(bool wrap) override;
};

class Texture2DHDR : public Texture {
public:
    Texture2DHDR(OpenGLContext* context, int arrayIdx, int associatedTexSlot);
    ~Texture2DHDR();

    void create(const char *texturePath, bool wrap) override;
    void create(bool wrap) override;
};

class Mesh;

class TextureTriangleStorage : public Texture {
private:
    Mesh *representedMesh;
public:
    TextureTriangleStorage(OpenGLContext* context,
                           int arrayIdx, int associatedTexSlot,
                           Mesh *mesh);
    ~TextureTriangleStorage();

    void create(bool wrap) override;
};

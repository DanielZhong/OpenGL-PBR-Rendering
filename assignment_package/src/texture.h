#pragma once

#include "openglcontext.h"
#include "glm_includes.h"
#include "smartpointerhelp.h"
#include "stb_image.h"

// Texture slot for the 2D HDR environment map
#define ENV_MAP_FLAT_TEX_SLOT 0
// Texture slot for the 3D HDR environment cube map
#define ENV_MAP_CUBE_TEX_SLOT 1
// Texture slot for the 3D HDR diffuse irradiance map
#define DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT 2
// Texture slot for the 3D HDR glossy irradiance map
#define GLOSSY_IRRADIANCE_CUBE_TEX_SLOT 3
// Texture slot for the BRDF lookup texture
#define BRDF_LUT_TEX_SLOT 4

#define ALBEDO_TEX_SLOT 5
#define METALLIC_TEX_SLOT 6
#define ROUGHNESS_TEX_SLOT 7
#define AO_TEX_SLOT 8
#define NORMALS_TEX_SLOT 9
#define DISPLACEMENT_TEX_SLOT 10

class Texture {
public:
    Texture(OpenGLContext* context);
    virtual ~Texture();

    virtual void create(const char *texturePath, bool wrap) = 0;
    void destroy();
    void bind(GLuint texSlot);

    bool m_isCreated;

protected:
    OpenGLContext* context;
    GLuint m_textureHandle;
};

class Texture2D : public Texture {
public:
    Texture2D(OpenGLContext* context);
    ~Texture2D();

    void create(const char *texturePath, bool wrap) override;

private:
    uPtr<QImage> m_textureImage;
};

class Texture2DHDR : public Texture {
public:
    Texture2DHDR(OpenGLContext* context);
    ~Texture2DHDR();

    void create(const char *texturePath, bool wrap) override;
};

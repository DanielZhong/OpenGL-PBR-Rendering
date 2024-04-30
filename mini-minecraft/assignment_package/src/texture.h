#pragma once
#include <QOpenGLFunctions_3_3_Core>
#include <OpenGLContext.h>

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

#define GBUFFER_POSITION_WORLD_TEX_SLOT 5
#define GBUFFER_NORMAL_TEX_SLOT 6
#define GBUFFER_ALBEDO_TEX_SLOT 7
#define GBUFFER_METAL_ROUGH_MASK_TEX_SLOT 8
#define GBUFFER_PBR_TEX_SLOT 9
#define GBUFFER_SSR_SPECULAR_TEX_SLOT 10
#define GBUFFER_SSR_GLOSSY1_TEX_SLOT 11
#define GBUFFER_SSR_GLOSSY2_TEX_SLOT 12
#define GBUFFER_SSR_GLOSSY3_TEX_SLOT 13
#define GBUFFER_SSR_GLOSSY4_TEX_SLOT 14

// TODO: Assign new tex slots for material maps later
#define ALBEDO_TEX_SLOT 15
#define METALLIC_TEX_SLOT 16
#define ROUGHNESS_TEX_SLOT 17
#define SCENE_TEX_SLOT 18
#define TEXTURE_TEX_SLOT 19
#define NORMAL_TEX_SLOT 20
#define LIGHTING_TEX_SLOT 21
#define DEPTH_TEX_SLOT 22
#define FOG_TEX_SLOT 23
#define SHADOW_TEX_SLOT 24
#define SKY_TEX_SLOT 25
//#define AO_TEX_SLOT 8
//#define NORMALS_TEX_SLOT 9
//#define DISPLACEMENT_TEX_SLOT 10

#define BLUR_LEVEL_1_TEX_SLOT 30
#define BLUR_LEVEL_2_TEX_SLOT 29
#define BLUR_LEVEL_3_TEX_SLOT 28
#define BLUR_LEVEL_4_TEX_SLOT 27
#define BLUR_LEVEL_5_TEX_SLOT 26

class Texture
{
private:
    OpenGLContext* glContext;
    GLuint m_textureHandle;
    GLenum internalFormat, format, dataType;

public:
    Texture(OpenGLContext* context,
            GLenum internalFormat, GLenum format, GLenum dataType);
    ~Texture();

    bool isCreated;

    void reformat(GLenum internalFormat,
                  GLenum format,
                  GLenum dataType);

    void create(const char *texturePath, bool hdr, bool lerp, bool wrap, bool mipmap);

    void bufferPixelData(unsigned int width, unsigned int height,
                         GLvoid *pixels);
    void bind(int texSlot);
    void destroy();

    GLuint getHandle() const;

};

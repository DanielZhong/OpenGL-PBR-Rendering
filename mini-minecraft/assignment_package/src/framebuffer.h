#pragma once
#include "texture.h"
#include "utils.h"
#include <QOpenGLFunctions_3_3_Core>

enum class GBufferOutputType : unsigned int {
    POSITION_WORLD,   // stores world-space position
    NORMAL,           // stores world-space normal
    ALBEDO,           // stores material albedo
    METAL_ROUGH_MASK, // stores metallic, roughness, and geom mask as RGB channels
    PBR,              // stores PBR shader LTE output
    SSR,              // stores the screen-space reflection of the scene
    SSR_BLUR0,        // first level of blurred glossy reflection
    SSR_BLUR1,        // second level of blurred glossy reflection
    SSR_BLUR2,        // third level of blurred glossy reflection
    SSR_BLUR3,        // fourth level of blurred glossy reflection
    NONE,          // Used for cube map, which doesn't output to G buffer
    // TODO add more as you need them
    SCENE,
    TEXTURE,
    LIGHTING,
    DEPTH,
    SHADOW,
    FOG,
    SKY,
    SUN
};

class FrameBuffer {
protected:
    OpenGLContext *glContext;
    GLuint m_frameBuffer;
    std::unordered_map<GBufferOutputType, uPtr<Texture>> m_outputTextures;
    GLuint m_depthRenderBuffer;

    unsigned int m_width, m_height;
    float m_devicePixelRatio;
    bool m_created;

    unsigned int m_textureSlot;

public:
    FrameBuffer(OpenGLContext *context,
                unsigned int width, unsigned int height,
                float devicePixelRatio);
    // Make sure to call resize from MyGL::resizeGL
    // to keep your frame buffer up to date with
    // your screen dimensions
    void resize(unsigned int width, unsigned int height,
                float devicePixelRatio);
    // Initialize all GPU-side data required
    virtual void create(bool mipmap = false);
    // Deallocate all GPU-side data
    virtual void destroy();

    inline unsigned int width() const {
        return m_width;
    }
    inline unsigned int height() const {
        return m_height;
    }

    // Append a Texture to receive the next-lowest unused
    // GL_COLOR_ATTACHMENT as data.
    void addTexture(GBufferOutputType a);
    const Texture& getTexture(GBufferOutputType a);

    void setDrawBuffers(const std::vector<GBufferOutputType> &attachmentTypes);

    void bindFrameBuffer();
    void bindRenderBuffer(unsigned int w, unsigned int h);
    // Associate our output texture with the indicated texture slot
    virtual void bindToTextureSlot(unsigned int slot, GBufferOutputType tex);
    unsigned int getTextureSlot() const;
};

class CubeMapFrameBuffer : public FrameBuffer {
protected:
    GLuint m_outputCubeMap;

public:
    CubeMapFrameBuffer(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio);

    void create(bool mipmap = false) override;
    void destroy() override;
    void bindToTextureSlot(unsigned int slot, GBufferOutputType tex) override;

    unsigned int getCubemapHandle() const;

    void generateMipMaps();
};


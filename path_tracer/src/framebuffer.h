#pragma once
#include "openglcontext.h"
#include "glm_includes.h"

// A class representing a frame buffer in the OpenGL pipeline.
// Stores three GPU handles: one to a frame buffer object, one to
// a texture object that will store the frame buffer's contents,
// and one to a depth buffer needed to properly render to the frame
// buffer.
// Redirect your render output to a FrameBuffer by invoking
// bindFrameBuffer() before ShaderProgram::draw, and read
// from the frame buffer's output texture by invoking
// bindToTextureSlot() and then associating a ShaderProgram's
// sampler2d with the appropriate texture slot.
class FrameBuffer {
protected:
    OpenGLContext *mp_context;
    GLuint m_frameBuffer;
    GLuint m_depthRenderBuffer;

    unsigned int m_width, m_height, m_devicePixelRatio;
    bool m_created;

    unsigned int m_textureSlot;

public:
    FrameBuffer(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio);
    // Make sure to call resize from MyGL::resizeGL to keep your frame buffer up to date with
    // your screen dimensions
    void resize(unsigned int width, unsigned int height, unsigned int devicePixelRatio);
    // Initialize all GPU-side data required
    virtual void create(bool mipmap = false);
    // Deallocate all GPU-side data
    virtual void destroy();
    void bindFrameBuffer();
    void bindRenderBuffer(unsigned int w, unsigned int h);
    // Associate our output texture with the indicated texture slot
    virtual void bindToTextureSlot(unsigned int slot) = 0;
    unsigned int getTextureSlot() const;
    inline unsigned int width() const {
        return m_width;
    }
    inline unsigned int height() const {
        return m_height;
    }
};

class FrameBuffer2D : public FrameBuffer {
protected:
    unsigned int m_outputTexture;

public:
    FrameBuffer2D(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio);

    void create(bool mipmap = false) override;
    void destroy() override;
    void bindToTextureSlot(unsigned int slot) override;

    void generateMipMaps();
};

class CubeMapFrameBuffer : public FrameBuffer {
protected:
    unsigned int m_outputCubeMap;

public:
    CubeMapFrameBuffer(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio);

    void create(bool mipmap = false) override;
    void destroy() override;
    void bindToTextureSlot(unsigned int slot) override;

    unsigned int getCubemapHandle() const;

    void generateMipMaps();
};

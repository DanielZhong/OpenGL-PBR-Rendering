#include "framebuffer.h"
#include <iostream>

FrameBuffer::FrameBuffer(OpenGLContext *context,
                         unsigned int width, unsigned int height, unsigned int devicePixelRatio)
    : mp_context(context), m_frameBuffer(-1),
      /*m_outputTexture(-1), */m_depthRenderBuffer(-1),
      m_width(width), m_height(height), m_devicePixelRatio(devicePixelRatio), m_created(false)
{}

void FrameBuffer::resize(unsigned int width, unsigned int height, unsigned int devicePixelRatio) {
    m_width = width;
    m_height = height;
    m_devicePixelRatio = devicePixelRatio;
}

void FrameBuffer::create(bool mipmap) {
    // Initialize the frame buffers and render textures
    mp_context->glGenFramebuffers(1, &m_frameBuffer);
//    mp_context->glGenTextures(1, &m_outputTexture);
    mp_context->glGenRenderbuffers(1, &m_depthRenderBuffer);

    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

    // Initialize our depth buffer
    mp_context->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
    mp_context->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, m_width * m_devicePixelRatio, m_height * m_devicePixelRatio);
    mp_context->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderBuffer);

    // Sets the color output of the fragment shader to be stored in GL_COLOR_ATTACHMENT0,
    // which we previously set to m_renderedTexture
    GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    mp_context->glDrawBuffers(1, drawBuffers); // "1" is the size of drawBuffers

    m_created = true;
    if(mp_context->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        m_created = false;
        std::cout << "Frame buffer did not initialize correctly..." << std::endl;
        mp_context->printGLErrorLog();
    }
}

void FrameBuffer::destroy() {
    if(m_created) {
        m_created = false;
        mp_context->glDeleteFramebuffers(1, &m_frameBuffer);
        m_frameBuffer = -1;
        mp_context->glDeleteRenderbuffers(1, &m_depthRenderBuffer);
        m_depthRenderBuffer = -1;
    }
}

void FrameBuffer::bindFrameBuffer() {
    mp_context->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
}

void FrameBuffer::bindRenderBuffer(unsigned int w, unsigned int h) {
    mp_context->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
    mp_context->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w * m_devicePixelRatio, h * m_devicePixelRatio);
}

unsigned int FrameBuffer::getTextureSlot() const {
    return m_textureSlot;
}

FrameBuffer2D::FrameBuffer2D(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio)
    : FrameBuffer(context, width, height, devicePixelRatio), m_outputTexture(-1)
{}

CubeMapFrameBuffer::CubeMapFrameBuffer(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio)
    : FrameBuffer(context, width, height, devicePixelRatio), m_outputCubeMap(-1)
{}


void FrameBuffer2D::create(bool mipmap) {
    FrameBuffer::create();

    mp_context->glGenTextures(1, &m_outputTexture);
    mp_context->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
                 m_width, m_height, 0, GL_RGBA, GL_FLOAT, nullptr);
    mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(mipmap) {
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
        mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    mp_context->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(mipmap) {
        mp_context->glGenerateMipmap(GL_TEXTURE_2D);
    }
    mp_context->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_outputTexture, 0);
}

void CubeMapFrameBuffer::create(bool mipmap) {
    FrameBuffer::create();

    mp_context->glGenTextures(1, &m_outputCubeMap);
    mp_context->glBindTexture(GL_TEXTURE_CUBE_MAP, m_outputCubeMap);
    for (unsigned int i = 0; i < 6; ++i) {
        //              Color format is 16 bits per channel! VVVVV
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     m_width, m_height, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    mp_context->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    mp_context->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    mp_context->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if(mipmap) {
        mp_context->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
        mp_context->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    mp_context->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(mipmap) {
        mp_context->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
//    mp_context->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_outputCubeMap, 0);
}

void FrameBuffer2D::destroy() {
    if(m_created) {
        mp_context->glDeleteTextures(1, &m_outputTexture);
    }
    m_outputTexture = -1;
    FrameBuffer::destroy();
}

void CubeMapFrameBuffer::destroy() {
    if(m_created) {
        mp_context->glDeleteTextures(1, &m_outputCubeMap);
    }
    m_outputCubeMap = -1;
    FrameBuffer::destroy();
}


void FrameBuffer2D::bindToTextureSlot(unsigned int slot) {
    m_textureSlot = slot;
    mp_context->glActiveTexture(GL_TEXTURE0 + slot);
    mp_context->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
}

void CubeMapFrameBuffer::bindToTextureSlot(unsigned int slot) {
    m_textureSlot = slot;
    mp_context->glActiveTexture(GL_TEXTURE0 + slot);
    mp_context->glBindTexture(GL_TEXTURE_CUBE_MAP, m_outputCubeMap);
}


unsigned int CubeMapFrameBuffer::getCubemapHandle() const {
    return m_outputCubeMap;
}

void FrameBuffer2D::generateMipMaps() {
    mp_context->glBindTexture(GL_TEXTURE_2D, m_outputTexture);
    mp_context->glGenerateMipmap(GL_TEXTURE_2D);
}

void CubeMapFrameBuffer::generateMipMaps() {
    mp_context->glBindTexture(GL_TEXTURE_CUBE_MAP, m_outputCubeMap);
    mp_context->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

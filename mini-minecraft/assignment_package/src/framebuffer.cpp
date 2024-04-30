#include "framebuffer.h"
#include <iostream>
#include "utils.h"
#include <exception>
#include <unordered_map>

FrameBuffer::FrameBuffer(OpenGLContext *context,
                         unsigned int width, unsigned int height,
                         float devicePixelRatio)
    : glContext(context), m_frameBuffer(-1),
      m_outputTextures(), m_depthRenderBuffer(-1),
      m_width(width), m_height(height),
      m_devicePixelRatio(devicePixelRatio), m_created(false)
{}

void FrameBuffer::resize(unsigned int width, unsigned int height,
                         float devicePixelRatio) {
    m_width = width;
    m_height = height;
    m_devicePixelRatio = devicePixelRatio;
}

void FrameBuffer::create(bool mipmap) {
    // Initialize the frame buffers and render textures
    glContext->glGenFramebuffers(1, &m_frameBuffer);
    glContext->glGenRenderbuffers(1, &m_depthRenderBuffer);
//    m_outputTexture.create(nullptr, GL_RGB, GL_RGB);

    glContext->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

    // Initialize our depth buffer
    glContext->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
    glContext->glRenderbufferStorage(GL_RENDERBUFFER,
                                     GL_DEPTH_COMPONENT24,
                                     m_width * m_devicePixelRatio,
                                     m_height * m_devicePixelRatio);
    glContext->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderBuffer);

    // Sets the color output of the fragment shader to be stored in GL_COLOR_ATTACHMENT0,
    // which we previously set to m_renderedTexture
    GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    glContext->glDrawBuffers(1, drawBuffers); // "1" is the size of drawBuffers

    m_created = true;
    if(glContext->glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        m_created = false;
        std::cout << "Frame buffer did not initialize correctly..." << std::endl;
        printGLErrorLog();
    }
}

void FrameBuffer::destroy() {
    if(m_created) {
        m_created = false;
        glContext->glDeleteFramebuffers(1, &m_frameBuffer);
        glContext->glDeleteRenderbuffers(1, &m_depthRenderBuffer);
        m_outputTextures.clear();
    }
}

void FrameBuffer::addTexture(GBufferOutputType a) {
    uPtr<Texture> t;
    switch(a) {
    case GBufferOutputType::POSITION_WORLD:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::NORMAL:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::ALBEDO:
        t = mkU<Texture>(glContext,
                         GL_RGBA16F,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE);
        t->create(nullptr, true, true, false, true);
        break;
    case GBufferOutputType::METAL_ROUGH_MASK:
        t = mkU<Texture>(glContext,
                         GL_RGBA,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::PBR:
        t = mkU<Texture>(glContext,
                         GL_RGBA16F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::SSR:
        t = mkU<Texture>(glContext,
                         GL_RGBA16F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, true, false, false);
        break;
    case GBufferOutputType::SSR_BLUR0:
        t = mkU<Texture>(glContext,
                         GL_RGBA16F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::SSR_BLUR1:
        t = mkU<Texture>(glContext,
                         GL_RGBA16F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::SSR_BLUR2:
        t = mkU<Texture>(glContext,
                         GL_RGBA16F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::SSR_BLUR3:
        t = mkU<Texture>(glContext,
                         GL_RGBA16F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::TEXTURE:
        t = mkU<Texture>(glContext,
                         GL_RGBA,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::SCENE:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, true, true, false, true);
        break;
    case GBufferOutputType::LIGHTING:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, true, true, false, true);
        break;
    case GBufferOutputType::DEPTH:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, false, false, false, false);
        break;
    case GBufferOutputType::FOG:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, true, true, false, true);
        break;
    case GBufferOutputType::SHADOW:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, true, true, false, true);
        break;
    case GBufferOutputType::SKY:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, true, true, false, true);
        break;
    case GBufferOutputType::SUN:
        t = mkU<Texture>(glContext,
                         GL_RGBA32F,
                         GL_RGBA,
                         GL_FLOAT);
        t->create(nullptr, true, true, false, true);
        break;
    default:
        break;
    }
    t->bufferPixelData(m_width * m_devicePixelRatio,
                       m_height * m_devicePixelRatio,
                       nullptr);
    m_outputTextures[a] = std::move(t);
}


const Texture& FrameBuffer::getTexture(GBufferOutputType a) {
    auto it = m_outputTextures.find(a);
    if (it != m_outputTextures.end()) {
        return *(it->second); // Accessing the second element of the iterator which is the unique_ptr to Texture
    }
    throw std::out_of_range("This FrameBuffer does not contain such a Texture!");
}


void FrameBuffer::setDrawBuffers(const std::vector<GBufferOutputType> &attachmentTypes) {
    std::vector<unsigned int> colorAttachments;
    this->bindFrameBuffer();

    for(unsigned int i = 0; i < attachmentTypes.size(); ++i) {
        GBufferOutputType a = attachmentTypes[i];
        Texture &t = *(m_outputTextures.at(a));

        // if (a == GBufferOutputType::DEPTH) {
        //     // Bind the texture as a depth attachment
        //     glContext->glFramebufferTexture2D(
        //         GL_FRAMEBUFFER,
        //         GL_DEPTH_ATTACHMENT,
        //         GL_TEXTURE_2D,
        //         t.getHandle(),
        //         0);
        // } else {
            // Bind the texture as a color attachment
            glContext->glFramebufferTexture2D(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0 + i,
                GL_TEXTURE_2D,
                t.getHandle(),
                0);
            colorAttachments.push_back(GL_COLOR_ATTACHMENT0 + i);
        // }
    }

    // Specify the list of color attachments to be used as the outputs from the fragment shader
    if (!colorAttachments.empty()) {
        glContext->glDrawBuffers(colorAttachments.size(), colorAttachments.data());
    } else {
        // No color outputs needed, typically when only depth processing is performed
        GLenum none = GL_NONE;
        glContext->glDrawBuffers(1, &none);
    }
}


void FrameBuffer::bindFrameBuffer() {
    glContext->glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
}

void FrameBuffer::bindRenderBuffer(unsigned int w, unsigned int h) {
    glContext->glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderBuffer);
    glContext->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w * m_devicePixelRatio, h * m_devicePixelRatio);
}

void FrameBuffer::bindToTextureSlot(unsigned int slot,
                                    GBufferOutputType tex) {
    m_textureSlot = slot;
    m_outputTextures.at(tex)->bind(slot);
}

unsigned int FrameBuffer::getTextureSlot() const {
    return m_textureSlot;
}


CubeMapFrameBuffer::CubeMapFrameBuffer(OpenGLContext *context, unsigned int width, unsigned int height, unsigned int devicePixelRatio)
    : FrameBuffer(context, width, height, devicePixelRatio), m_outputCubeMap(-1)
{}


void CubeMapFrameBuffer::create(bool mipmap) {
    FrameBuffer::create();

    glContext->glGenTextures(1, &m_outputCubeMap);
    glContext->glBindTexture(GL_TEXTURE_CUBE_MAP, m_outputCubeMap);
    for (unsigned int i = 0; i < 6; ++i) {
        //              Color format is 16 bits per channel! VVVVV
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     m_width, m_height, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glContext->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glContext->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glContext->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if(mipmap) {
        glContext->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    else {
        glContext->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glContext->glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(mipmap) {
        glContext->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }
}

void CubeMapFrameBuffer::destroy() {
    if(m_created) {
        glContext->glDeleteTextures(1, &m_outputCubeMap);
    }
    m_outputCubeMap = -1;
    FrameBuffer::destroy();
}

void CubeMapFrameBuffer::bindToTextureSlot(unsigned int slot, GBufferOutputType tex) {
    m_textureSlot = slot;
    glContext->glActiveTexture(GL_TEXTURE0 + slot);
    glContext->glBindTexture(GL_TEXTURE_CUBE_MAP, m_outputCubeMap);
}


unsigned int CubeMapFrameBuffer::getCubemapHandle() const {
    return m_outputCubeMap;
}

void CubeMapFrameBuffer::generateMipMaps() {
    glContext->glBindTexture(GL_TEXTURE_CUBE_MAP, m_outputCubeMap);
    glContext->glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

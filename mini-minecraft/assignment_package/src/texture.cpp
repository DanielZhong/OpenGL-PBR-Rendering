#include "texture.h"
#include <QImage>
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(OpenGLContext *context,
                 GLenum internalFormat, GLenum format, GLenum dataType)
    : glContext(context), m_textureHandle(-1),
      internalFormat(internalFormat), format(format), dataType(dataType),
      isCreated(false)
{}

Texture::~Texture()
{
    destroy();
}

void Texture::reformat(GLenum internalFormat,
              GLenum format,
              GLenum dataType) {
    this->internalFormat = internalFormat;
    this->format = format;
    this->dataType = dataType;
}

void Texture::create(const char *texturePath, bool hdr, bool lerp, bool wrap, bool mipmap)
{
    isCreated = true;
    // Create a texture object on the GPU
    glContext->glGenTextures(1, &m_textureHandle);
    // Make that texture the "active texture" so that
    // any functions we call that operate on textures
    // operate on this one.
    printGLErrorLog();
    glContext->glActiveTexture(GL_TEXTURE31);
    glContext->glBindTexture(GL_TEXTURE_2D, m_textureHandle);

    // Set the image filtering and UV wrapping options for the texture.
    // These parameters need to be set for EVERY texture you create.
    // They don't always have to be set to the values given here,
    // but they do need to be set.
    printGLErrorLog();
    if(lerp) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else if(mipmap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }
    if(wrap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if(mipmap) {
        glContext->glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Only bother to set up a QImage to read the texture file
    // if there *is* a texture file. If there's not, then this
    // Texture is meant to be used as the write target for something
    // like a FrameBuffer.
    if(texturePath) {
        if(hdr) {
            stbi_set_flip_vertically_on_load(true);
            int width, height, nrComponents;
            float *data = stbi_loadf(texturePath, &width, &height, &nrComponents, 0);
            bufferPixelData(width, height,
                            data);
            stbi_image_free(data);
        }
        else {
            QImage img(texturePath);
            img.convertTo(QImage::Format_ARGB32);
            img = img.mirrored();
            bufferPixelData(img.width(), img.height(),
                            img.bits());
        }
    }
    printGLErrorLog();
}


void Texture::bufferPixelData(unsigned int width, unsigned int height,
                              GLvoid *pixels) {
    glContext->glActiveTexture(GL_TEXTURE31);
    glContext->glBindTexture(GL_TEXTURE_2D, m_textureHandle);
    glContext->glTexImage2D(GL_TEXTURE_2D, 0, internalFormat,
                            width, height,
                            0, format, dataType, pixels);
}

void Texture::bind(int texSlot = 0)
{
    glContext->glActiveTexture(GL_TEXTURE0 + texSlot);
    glContext->glBindTexture(GL_TEXTURE_2D, m_textureHandle);
}

GLuint Texture::getHandle() const {
    return m_textureHandle;
}

void Texture::destroy() {
    isCreated = false;
    glContext->glDeleteTextures(1, &m_textureHandle);
}

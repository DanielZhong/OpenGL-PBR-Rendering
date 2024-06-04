#include "texture.h"
#include <QImage>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void Texture::bind(GLuint texSlot = 0) {
    context->glActiveTexture(GL_TEXTURE0 + texSlot);
    context->glBindTexture(GL_TEXTURE_2D, m_textureHandle);
}

Texture::Texture(OpenGLContext *context, int arrayIdx, int associatedTexSlot)
    : m_isCreated(false), m_uniformArrayIndex(arrayIdx), m_associatedTextureSlot(associatedTexSlot),
      context(context), m_textureHandle(0), m_texturePath("")
{}

Texture2D::Texture2D(OpenGLContext *context, int arrayIdx, int associatedTexSlot)
    : Texture(context, arrayIdx, associatedTexSlot)
{}

Texture2DHDR::Texture2DHDR(OpenGLContext *context, int arrayIdx, int associatedTexSlot)
    : Texture(context, arrayIdx, associatedTexSlot)
{}

TextureTriangleStorage::TextureTriangleStorage(OpenGLContext *context, int arrayIdx, int associatedTexSlot, Mesh *mesh)
    : Texture(context, arrayIdx, associatedTexSlot), representedMesh(mesh)
{}

Texture::~Texture()
{}
Texture2D::~Texture2D()
{}
Texture2DHDR::~Texture2DHDR()
{}
TextureTriangleStorage::~TextureTriangleStorage()
{}

void Texture::destroy() {
    context->glDeleteTextures(1, &m_textureHandle);
    m_isCreated = false;
}

void Texture::create(const char *texturePath, bool wrap) {
    context->printGLErrorLog();
    this->m_texturePath = QString(texturePath);
    create(wrap);
}

void Texture2D::create(const char *texturePath, bool wrap) {
    Texture::create(texturePath, wrap);
}
void Texture2DHDR::create(const char *texturePath, bool wrap) {
    Texture::create(texturePath, wrap);
}

void Texture2D::create(bool wrap) {
    context->printGLErrorLog();

    QImage img;
    bool valid = img.load(m_texturePath);
    img = img.convertToFormat(QImage::Format_ARGB32);
    img = img.mirrored();

    context->glGenTextures(1, &m_textureHandle);

    context->glBindTexture(GL_TEXTURE_2D, m_textureHandle);

    // These parameters need to be set for EVERY texture you create
    // They don't always have to be set to the values given here, but they do need
    // to be set
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    if(wrap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    context->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                          img.width(), img.height(),
                          0, GL_BGRA, GL_UNSIGNED_BYTE, img.bits());
    context->printGLErrorLog();
    m_isCreated = true;
}

void Texture2DHDR::create(bool wrap) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(m_texturePath.toStdString().c_str(),
                             &width, &height, &nrComponents, 0);
    if(data) {
        glGenTextures(1, &m_textureHandle);
        glBindTexture(GL_TEXTURE_2D, m_textureHandle);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        if(wrap) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        stbi_image_free(data);
    }
    else {
        throw std::runtime_error("Failed to load HDR image!");
    }
    m_isCreated = true;
}

std::vector<float> trianglesToFloats(std::vector<Triangle> const * const tris, int size) {
    std::vector<float> result;
    result.reserve(size * 3);
    for(auto &t : *tris) {
        for(int i = 0; i < 9; ++i) {
            glm::vec3 element = t[i];
            result.push_back(element[0]);
            result.push_back(element[1]);
            result.push_back(element[2]);
        }
    }
    // Pad out the rest of the image
    int currSize = result.size();
    for(int i = currSize; i < size * 3; ++i) {
        result.push_back(0.f);
    }
    return result;
}

void TextureTriangleStorage::create(bool wrap) {
//    stbi_set_flip_vertically_on_load(true);
    int width, height;
    representedMesh->computeStorageDimensions(&width, &height);
    std::vector<Triangle> *tris = &(representedMesh->triangles);
    std::vector<float> contents = trianglesToFloats(tris, width * height);
    glGenTextures(1, &m_textureHandle);
    glBindTexture(GL_TEXTURE_2D, m_textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, contents.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    if(wrap) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    m_isCreated = true;
}

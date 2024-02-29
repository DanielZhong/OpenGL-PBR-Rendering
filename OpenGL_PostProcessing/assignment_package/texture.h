#pragma once
#include <QOpenGLFunctions_3_3_Core>

class Texture
{
private:
    QOpenGLFunctions_3_3_Core* glContext;
    GLuint m_textureHandle;

public:
    Texture(QOpenGLFunctions_3_3_Core* context);
    ~Texture();

    void create(const char *texturePath,
                GLenum internalFormat = GL_RGBA,
                GLenum format = GL_BGRA);
    void bufferPixelData(unsigned int width, unsigned int height,
                GLenum internalFormat, GLenum format, GLvoid *pixels);
    void bind(int texSlot);
    void destroy();

    GLuint getHandle() const;

};

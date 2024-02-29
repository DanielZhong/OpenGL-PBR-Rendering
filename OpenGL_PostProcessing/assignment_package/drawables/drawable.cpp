#include "drawables.h"

Drawable::Drawable(QOpenGLFunctions_3_3_Core *context)
    : glContext(context),
      bufferPosition(0),
      bufferNormal(0),
      bufferUV(0),
      bufferIndex(0),
      indexBufferLength(-1)
{}

Drawable::~Drawable()
{}

void Drawable::bindBuffer(BufferType t) {
    switch(t) {
    case POSITION:
        glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferPosition);
        break;
    case NORMAL:
        glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferNormal);
        break;
    case UV:
        glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferUV);
        break;
    case INDEX:
        glContext->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIndex);
        break;
    }
}

int Drawable::getIndexBufferLength() const {
    return indexBufferLength;
}




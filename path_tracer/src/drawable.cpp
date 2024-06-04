#include "drawable.h"
#include <glm_includes.h>

Drawable::Drawable(OpenGLContext* context)
    : count(-1), bufHandles(),
      bufGenerated{false, false, false},
      mp_context(context)
{}

Drawable::~Drawable() {
    destroy();
}


void Drawable::destroy()
{
    mp_context->glDeleteBuffers(3, &bufHandles[0]);
}

GLenum Drawable::drawMode()
{
    // Since we want every three indices in bufIdx to be
    // read to draw our Drawable, we tell that the draw mode
    // of this Drawable is GL_TRIANGLES

    // If we wanted to draw a wireframe, we would return GL_LINES

    return GL_TRIANGLES;
}

int Drawable::elemCount()
{
    return count;
}

void Drawable::generateBuffer(int buf) {
    bufGenerated[buf] = true;
    mp_context->glGenBuffers(1, &bufHandles[buf]);
}

bool Drawable::bindBuffer(int buf) {
    if(bufGenerated[buf]) {
        buf == IDX ?
        mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufHandles[buf]) :
        mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufHandles[buf]);
    }
    return bufGenerated[buf];
}

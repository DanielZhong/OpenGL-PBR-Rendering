#include "squareplane.h"
#include<glm_includes.h>
#include <iostream>

SquarePlane::SquarePlane(OpenGLContext *mp_context) : Drawable(mp_context)
{}

void SquarePlane::create()
{

    std::vector<glm::vec3> pos {glm::vec3(-1, -1, 1),
                                glm::vec3(1, -1, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(-1, 1, 1)};

    std::vector<glm::vec2> uv { glm::vec2(0, 0),
                                glm::vec2(1, 0),
                                glm::vec2(1, 1),
                                glm::vec2(0, 1)};

    std::vector<GLuint> idx {0, 1, 2, 0, 2, 3};

    count = 6; // TODO: Set "count" to the number of indices in your index VBO

    generateBuffer(IDX);
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufHandles[IDX]);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generateBuffer(POS);
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufHandles[POS]);
    mp_context->glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec3), pos.data(), GL_STATIC_DRAW);

    generateBuffer(UV);
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, bufHandles[UV]);
    mp_context->glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(glm::vec2), uv.data(), GL_STATIC_DRAW);
}

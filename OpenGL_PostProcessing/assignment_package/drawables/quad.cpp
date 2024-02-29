#include "drawables.h"
#include <vector>

Quad::Quad(QOpenGLFunctions_3_3_Core *context)
    : Drawable(context)
{}

Quad::~Quad()
{}

void Quad::initializeAndBufferGeometryData() {
    std::vector<glm::vec3> glPos { glm::vec3(-1,-1,1),
                                   glm::vec3( 1,-1,1),
                                   glm::vec3( 1, 1,1),
                                   glm::vec3(-1, 1,1) };

    std::vector<glm::vec2> glUV { glm::vec2(0,0),
                                  glm::vec2(1,0),
                                  glm::vec2(1,1),
                                  glm::vec2(0,1) };

    std::vector<GLuint> glIndex {0,1,2,0,2,3};
    indexBufferLength = 6;

    glContext->glGenBuffers(1, &bufferPosition);
    glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferPosition);
    glContext->glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * glPos.size(), glPos.data(), GL_STATIC_DRAW);

    glContext->glGenBuffers(1, &bufferUV);
    glContext->glBindBuffer(GL_ARRAY_BUFFER, bufferUV);
    glContext->glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * glUV.size(), glUV.data(), GL_STATIC_DRAW);

    glContext->glGenBuffers(1, &bufferIndex);
    glContext->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufferIndex);
    glContext->glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * glIndex.size(), glIndex.data(), GL_STATIC_DRAW);

}

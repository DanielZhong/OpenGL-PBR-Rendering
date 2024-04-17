#include "drawables.h"

Cube::Cube(QOpenGLFunctions_3_3_Core *context)
    : Drawable(context)
{}

Cube::~Cube()
{}

void Cube::initializeAndBufferGeometryData() {
    std::vector<glm::vec3> pos {glm::vec3(-1, -1, -1),
                                glm::vec3(1, -1, -1),
                                glm::vec3(1, 1, -1),
                                glm::vec3(-1, 1, -1),
                                glm::vec3(-1, -1, 1),
                                glm::vec3(1, -1, 1),
                                glm::vec3(1, 1, 1),
                                glm::vec3(-1, 1, 1)};

    std::vector<GLuint> idx {1, 0, 3, 1, 3, 2,
                             4, 5, 6, 4, 6, 7,
                             5, 1, 2, 5, 2, 6,
                             7, 6, 2, 7, 2, 3,
                             0, 4, 7, 0, 7, 3,
                             0, 1, 5, 0, 5, 4};

    indexBufferLength = 36;

    generateBuffer(BufferType::INDEX);
    bindBuffer(BufferType::INDEX);
    glContext->glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(GLuint), idx.data(), GL_STATIC_DRAW);

    generateBuffer(BufferType::POSITION);
    bindBuffer(BufferType::POSITION);
    glContext->glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec3), pos.data(), GL_STATIC_DRAW);
}

#pragma once
#include "drawable.h"

struct Triangle {
    glm::vec3 points[3];
    glm::vec3 planeNormal;
    glm::vec3 normals[3];
    glm::vec2 uvs[3];

    Triangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3);
};

class Mesh : public Drawable {
private:
    std::vector<Triangle> faces;
public:
    Mesh(OpenGLContext *context);

    void create() override;
    void destroy() override;
    void LoadOBJ(const QString &filename, const QString &local_path);
};

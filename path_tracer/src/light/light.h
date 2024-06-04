#pragma once
#include "glm_includes.h"
#include "scene/transform.h"
#include <QString>

class Light {
public:
    glm::vec3 Le;
    Transform transform;

    Light(const glm::vec3 &le, const Transform &t);
    virtual ~Light();

    virtual QString toGLSL(int ID) const = 0;
};


#define RECTANGLE_SHAPE 1
#define SPHERE_SHAPE 2

class AreaLight : public Light {
public:
    int shapeType;

    AreaLight();
    virtual ~AreaLight();
    AreaLight(glm::vec3 le, int shape, const Transform &t);
    QString toGLSL(int ID) const override;
};

class PointLight : public Light {
public:
    glm::vec3 pos;

    PointLight();
    virtual ~PointLight();
    PointLight(glm::vec3 le, glm::vec3 pos);
    QString toGLSL(int ID) const override;
};

class SpotLight : public Light {
public:
    glm::vec3 pos;
    float innerAngle, outerAngle;

    SpotLight();
    virtual ~SpotLight();
    SpotLight(glm::vec3 le, glm::vec3 pos, float inner, float outer);
    QString toGLSL(int ID) const override;
};

#include "light.h"
#include "glslwriting.h"

Light::Light(const glm::vec3 &le, const Transform &t)
    : Le(le), transform(t)
{}

Light::~Light(){}
AreaLight::~AreaLight(){}
PointLight::~PointLight(){}
SpotLight::~SpotLight(){}

AreaLight::AreaLight(glm::vec3 le, int shape, const Transform &t)
    : Light(le, t), shapeType(shape)
{}

AreaLight::AreaLight()
    : Light(glm::vec3(0.), Transform()), shapeType()
{}


PointLight::PointLight(glm::vec3 le, glm::vec3 pos)
    : Light(le, Transform()), pos(pos)
{}
PointLight::PointLight()
    : Light(glm::vec3(0.), Transform()), pos()
{}

SpotLight::SpotLight(glm::vec3 le, glm::vec3 pos, float inner, float outer)
    : Light(le, Transform()), pos(pos), innerAngle(inner), outerAngle(outer)
{}
SpotLight::SpotLight()
    : Light(glm::vec3(0.), Transform()), pos(), innerAngle(), outerAngle()
{}

QString AreaLight::toGLSL(int ID) const {
    return "AreaLight(" +
            writeVec3(Le) + ", " +
            writeInt(ID) + ", " +
            writeInt(shapeType) + ", " +
            writeTransform(transform) + ")";
}

QString PointLight::toGLSL(int ID) const {
    return "PointLight(" +
            writeVec3(Le) + ", " +
            writeInt(ID) + ", " +
            writeVec3(pos) + ")";
}

QString SpotLight::toGLSL(int ID) const {
    return "SpotLight(" +
            writeVec3(Le) + ", " +
            writeInt(ID) + ", " +
            writeVec3(pos) + ", " +
            writeFloat(innerAngle) + ", " +
            writeFloat(outerAngle) + ")";
}

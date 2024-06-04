#include "glslwriting.h"

glm::mat4 translate(glm::vec3 t) {
    return glm::mat4(1,0,0,0,
                0,1,0,0,
                0,0,1,0,
                t.x, t.y, t.z, 1);
}

float radians(float deg) {
    return deg * 3.14159 / 180.f;
}

glm::mat4 rotateX(float rad) {
    return glm::mat4(1,0,0,0,
                0,cos(rad),sin(rad),0,
                0,-sin(rad),cos(rad),0,
                0,0,0,1);
}

glm::mat4 rotateY(float rad) {
    return glm::mat4(cos(rad),0,-sin(rad),0,
                0,1,0,0,
                sin(rad),0,cos(rad),0,
                0,0,0,1);
}


glm::mat4 rotateZ(float rad) {
    return glm::mat4(cos(rad),sin(rad),0,0,
                -sin(rad),cos(rad),0,0,
                0,0,1,0,
                0,0,0,1);
}

glm::mat4 scale(glm::vec3 s) {
    return glm::mat4(s.x,0,0,0,
                0,s.y,0,0,
                0,0,s.z,0,
                0,0,0,1);
}


QString mat4ToQString(const glm::mat4 &m) {
    QString result("mat4(");
    for(int c = 0; c < 4; ++c) {
        for(int r = 0; r < 4; ++r) {
            result += QString::number(m[c][r]);
            if(c != 3 || r != 3) {
                result += ", ";
            }
        }
    }
    result += ")";
    return result;
}

QString mat3ToQString(const glm::mat3 &m) {
    QString result("mat3(");
    for(int c = 0; c < 3; ++c) {
        for(int r = 0; r < 3; ++r) {
            result += QString::number(m[c][r]);
            if(c != 2 || r != 2) {
                result += ", ";
            }
        }
    }
    result += ")";
    return result;
}

QString writeTransform(const Transform &t) {
    glm::mat4 T = translate(t.translation)
                * rotateX(radians(t.rotation.x))
                * rotateY(radians(t.rotation.y))
                * rotateZ(radians(t.rotation.z))
                * scale(t.scale);

    return QString("Transform(") + mat4ToQString(T) +
                   ", " + mat4ToQString(glm::inverse(T)) + ", " +
                   mat3ToQString(glm::inverse(glm::transpose(glm::mat3(T)))) +
                   ", " + writeVec3(t.scale) + ")";
}

QString writeVec3(const glm::vec3 &v) {
    return "vec3(" + QString::number(v.x) + ", " +
            QString::number(v.y) + ", " +
            QString::number(v.z) + ")";
}

QString writeVec2(const glm::vec2 &v) {
    return "vec2(" + QString::number(v.x) + ", " +
            QString::number(v.y) + ")";
}

QString writeInt(int x) {
    return QString::number(x);
}
QString writeFloat(float x) {
    return QString::number(x);
}

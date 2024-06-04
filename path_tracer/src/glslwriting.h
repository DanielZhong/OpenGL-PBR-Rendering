#pragma once
#include <QString>
#include "scene/transform.h"
#include "glm_includes.h"

QString writeTransform(const Transform &t);
QString writeVec3(const glm::vec3 &v);
QString writeVec2(const glm::vec2 &v);
QString writeInt(int x);
QString writeFloat(float x);

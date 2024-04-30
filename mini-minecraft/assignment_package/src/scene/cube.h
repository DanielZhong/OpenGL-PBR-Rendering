#pragma once

#include "drawable.h"
#include <glm_includes.h>

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class Cube : public InstancedDrawable
{
public:
    Cube(OpenGLContext* context) : InstancedDrawable(context){}
    virtual ~Cube(){}
    void createVBOdata() override;
    void createInstancedVBOdata(std::vector<glm::vec3> &offsets, std::vector<glm::vec3> &colors) override;
};

#pragma once

#include "drawable.h"
#include <la.h>

#include <QOpenGLContext>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

class SquarePlane : public Drawable
{
public:
    SquarePlane(OpenGLContext* mp_context);
    virtual void create();
};

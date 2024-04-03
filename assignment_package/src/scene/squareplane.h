#pragma once

#include "drawable.h"
#include <glm_includes.h>

class SquarePlane : public Drawable
{
public:
    SquarePlane(OpenGLContext* mp_context);
    virtual void create();
};

#pragma once

#include "drawable.h"
#include <glm_includes.h>

class WorldAxes : public Drawable
{
public:
    WorldAxes(OpenGLContext* context) : Drawable(context){}
    virtual ~WorldAxes() override;
    void createVBOdata() override;
    GLenum drawMode() override;
};

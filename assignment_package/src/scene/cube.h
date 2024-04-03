#pragma once
#include "drawable.h"

class Cube : public Drawable {
public:
    Cube(OpenGLContext* mp_context);
    virtual void create();
};

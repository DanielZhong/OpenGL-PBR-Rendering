#pragma once
#include <QDir>
#include <QString>
#include <QOpenGLFunctions>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

/// The following function should be used to get the direct filepath
/// to this Qt project so that you can read the GLSL files used for
/// your shader
QString getCurrentPath();

// You should call this function in order to figure out which of your
// OpenGL function calls are causing an OpenGL pipeline error.
// Just invoke it after each GL function call you think is causing an error,
// then put a breakpoint in it to see which call triggers it.
void printGLErrorLog();

#define mkU std::make_unique
#define uPtr std::unique_ptr

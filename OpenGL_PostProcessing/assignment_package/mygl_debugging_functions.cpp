#include "mygl.h"
#include <iostream>
#include <QApplication>
#include <QProcessEnvironment>
#include <QOpenGLContext>
#include <QDebug>


inline const char *glGS(GLenum e)
{
    return reinterpret_cast<const char *>(glGetString(e));
}

void MyGL::debugContextVersion()
{
    QOpenGLContext *ctx = context();
    QSurfaceFormat form = format();
    QSurfaceFormat ctxform = ctx->format();
    QSurfaceFormat::OpenGLContextProfile prof = ctxform.profile();

    const char *profile =
        prof == QSurfaceFormat::CoreProfile ? "Core" :
        prof == QSurfaceFormat::CompatibilityProfile ? "Compatibility" :
        "None";
    int ctxmajor = ctxform.majorVersion();
    int ctxminor = ctxform.minorVersion();
    bool valid = ctx->isValid();
    int formmajor = form.majorVersion();
    int formminor = form.minorVersion();
    const char *vendor = glGS(GL_VENDOR);
    const char *renderer = glGS(GL_RENDERER);
    const char *version = glGS(GL_VERSION);
    const char* s_glsl = glGS(GL_SHADING_LANGUAGE_VERSION);

    printf("Widget version: %d.%d\n", ctxmajor, ctxminor);
    printf("Context valid: %s\n", valid ? "yes" : "NO");
    printf("Format version: %d.%d\n", formmajor, formminor);
    printf("Profile: %s\n", profile);
    printf("  Vendor:   %s\n", vendor);
    printf("  Renderer: %s\n", renderer);
    printf("  Version:  %s\n", version);
    printf("  GLSL:     %s\n", s_glsl);

    QString glsl = s_glsl;
    if (ctxmajor < 3 || glsl.startsWith("1.10") || glsl.startsWith("1.20")) {
        printf("ERROR: "
               "Unable to get an OpenGL 3.x context with GLSL 1.30 or newer. "
               "If your hardware should support it, update your drivers. "
               "If you have switchable graphics, make sure that you are using the discrete GPU.\n");
        QApplication::exit();
    } else if ((ctxmajor == 3 && ctxminor < 2) || glsl.startsWith("1.30") || glsl.startsWith("1.40")) {
        printf("WARNING: "
               "Enable to get an OpenGL 3.2 context with GLSL 1.50. "
               "If your hardware should support it, update your drivers. "
               "If you have switchable graphics, make sure that you are using the discrete GPU. "
               "If you cannot get 3.2 support, it is possible to port this project....");

        // Note: doing this requires at least the following actions:
        // * Change the header and base class in glwidget277.h to 3.0/3.1 instead of 3.2 Core.
        // * Change the shaders to require GLSL 1.30 or 1.40.
    }
}



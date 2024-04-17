QT       += core gui widgets openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++2a
win32 {
    LIBS += -lopengl32
    LIBS += -lglu32
}

INCLUDEPATH += $$PWD/include

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    camera.cpp \
    drawables/cube.cpp \
    drawables/drawable.cpp \
    drawables/mesh.cpp \
    drawables/quad.cpp \
    framebuffer.cpp \
    main.cpp \
    mainwindow.cpp \
    mygl.cpp \
    mygl_debugging_functions.cpp \
    shaderprogram.cpp \
    texture.cpp \
    tinyobj/tiny_obj_loader.cc \
    utils.cpp

HEADERS += \
    camera.h \
    drawables/drawables.h \
    framebuffer.h \
    mainwindow.h \
    mygl.h \
    shaderprogram.h \
    stb_image.h \
    texture.h \
    tinyobj/tiny_obj_loader.h \
    utils.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../glsl/blinn_phong.vert.glsl \
    ../glsl/blur.frag.glsl \
    ../glsl/blur.frag.glsl \
    ../glsl/combine.frag.glsl \
    ../glsl/custom_post.frag.glsl \
    ../glsl/custom_surface.frag.glsl \
    ../glsl/custom_surface.vert.glsl \
    ../glsl/g-buffer.frag.glsl \
    ../glsl/g-buffer.vert.glsl \
    ../glsl/lambert.frag.glsl \
    ../glsl/lambert.vert.glsl \
    ../glsl/matcap.frag.glsl \
    ../glsl/matcap.vert.glsl \
    ../glsl/noOp.frag.glsl \
    ../glsl/passthrough.vert.glsl \
    ../glsl/screenSpaceReflection.frag.glsl \
    ../glsl/sobel.frag.glsl

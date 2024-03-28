#ifndef MYGL_H
#define MYGL_H

#include <openglcontext.h>
#include <utils.h>
#include <shaderprogram.h>
#include "scene/squareplane.h"
#include "scene/mesh.h"
#include "camera.h"

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>


class MyGL
    : public OpenGLContext
{
    Q_OBJECT
private:
    SquarePlane m_geomSquare;
    Mesh m_geomSphere;
    ShaderProgram m_progPBR;

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
                // Don't worry too much about this. Just know it is necessary in order to render geometry.

    Camera m_glCamera;
    glm::vec2 m_mousePosPrev;

    glm::vec3 m_albedo;


public:
    explicit MyGL(QWidget *parent = nullptr);
    ~MyGL();

    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void initPBRunifs();

protected:
    void keyPressEvent(QKeyEvent *e);
    void mousePressEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
    void wheelEvent(QWheelEvent* e);

public slots:
    void slot_setRed(int);
    void slot_setGreen(int);
    void slot_setBlue(int);
    void slot_setMetallic(int);
    void slot_setRoughness(int);
    void slot_setAO(int);
    void slot_setRustEffect(bool);
};


#endif // MYGL_H

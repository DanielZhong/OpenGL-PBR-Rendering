#include "mygl.h"
#include <la.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QDir>


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      m_geomSquare(this), m_geomSphere(this),
      m_progPBR(this),
      m_glCamera(), m_mousePosPrev(), m_albedo(0.5f, 0.f, 0.f)
{
    setFocusPolicy(Qt::StrongFocus);
}

MyGL::~MyGL()
{
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    m_geomSquare.destroy();
}

void MyGL::initializeGL()
{
    // Create an OpenGL context using Qt's QOpenGLFunctions_3_2_Core class
    // If you were programming in a non-Qt context you might use GLEW (GL Extension Wrangler)instead
    initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();

    // Set a few settings/modes in OpenGL rendering
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
    // Set the size with which points should be rendered
    glPointSize(5);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.5, 0.5, 0.5, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instances of Cylinder and Sphere.
    m_geomSquare.create();
    QString path = QDir::currentPath();
    path = path.left(path.lastIndexOf("/"));
#ifdef __APPLE__
    path = path.left(path.lastIndexOf("/"));
    path = path.left(path.lastIndexOf("/"));
    path = path.left(path.lastIndexOf("/"));
#endif
    path.append("/assignment_package/objs/");
    m_geomSphere.LoadOBJ("sphere.obj", path);
    m_geomSphere.create();

    // Create and set up the diffuse shader
    m_progPBR.create(":/glsl/pbr.vert.glsl", ":/glsl/pbr.frag.glsl");

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    initPBRunifs();
}

void MyGL::resizeGL(int w, int h)
{
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_glCamera = Camera(w, h);
    glm::mat4 viewproj = m_glCamera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progPBR.setViewProjMatrix(viewproj);

    printGLErrorLog();
}

//This function is called by Qt any time your GL window is supposed to update
//For example, when the function update() is called, paintGL is called implicitly.
void MyGL::paintGL()
{
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_progPBR.setViewProjMatrix(m_glCamera.getViewProj());
    m_progPBR.setCamPos(m_glCamera.eye);

    m_progPBR.setModelMatrix(glm::mat4());
    m_progPBR.draw(m_geomSphere);


}

void MyGL::initPBRunifs() {
    m_progPBR.setAlbedo(m_albedo);
    m_progPBR.setAO(1.f);
    m_progPBR.setMetallic(0.5f);
    m_progPBR.setRoughness(0.5f);
}

void MyGL::slot_setRed(int r) {
    m_albedo.r = r / 100.f;
    m_progPBR.setAlbedo(m_albedo);
    update();
}
void MyGL::slot_setGreen(int g) {
    m_albedo.g = g / 100.f;
    m_progPBR.setAlbedo(m_albedo);
    update();
}
void MyGL::slot_setBlue(int b) {
    m_albedo.b = b / 100.f;
    m_progPBR.setAlbedo(m_albedo);
    update();
}

void MyGL::slot_setMetallic(int m) {
    m_progPBR.setMetallic(m / 100.f);
    update();
}
void MyGL::slot_setRoughness(int r) {
    m_progPBR.setRoughness(r / 100.f);
    update();
}
void MyGL::slot_setAO(int a) {
    m_progPBR.setAO(a / 100.f);
    update();
}

void MyGL::slot_setRustEffect(bool enabled) {
    m_progPBR.setRustToggle(enabled);
    update();
}



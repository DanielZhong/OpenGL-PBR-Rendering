#include "mygl.h"
#include <QDir>
#include <string.h>
#include <string>
#include <iostream>
#include "utils.h"
#include <glm/gtx/string_cast.hpp>
#include <tinyobj/tiny_obj_loader.h>
#include "utils.h"
#include "glm/gtc/matrix_transform.hpp"

MyGL::MyGL(QWidget *parent)
    : QOpenGLWidget(parent),
      vao(0),
      progLambert(&glContext),
      progBlinnPhong(&glContext),
      progMatcap(&glContext),
      progCustomSurface(&glContext),
      progPostProcessNoOp(&glContext),
      progPostProcessSobel(&glContext),
      progPostProcessBlur(&glContext),
      progPostProcessCustom(&glContext),
      progPostProcessMy(&glContext),
      selectedSurfaceShader(&progLambert),
      selectedPostProcessShader(&progPostProcessNoOp),
      meshDrawable(&glContext, (getCurrentPath() + "/objs/wahoo.obj").toStdString()),
      quadDrawable(&glContext),
      postProcessFrameBuffer(&glContext, this->width(), this->height(), this->devicePixelRatio()),
      meshTexture(&glContext),
      bgTexture(&glContext),
      matcapTextures(),
      currentMatcapTex(nullptr),
      camera(width(), height()),
      m_mousePosPrev()
{
    // Allow Qt to trigger mouse events
    // even when a mouse button is not held.
    setMouseTracking(true);

    connect(&timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    timer.start(16);
}


MyGL::~MyGL(){}

void MyGL::tick() {

    ++currTime;
    if(selectedSurfaceShader->isUniformHandleValid("u_Time")) {
        selectedSurfaceShader->useProgram();
        GLuint handle = selectedSurfaceShader->getUniformHandle("u_Time");
        glContext.glUniform1f(handle, currTime);
    }
    if(selectedPostProcessShader->isUniformHandleValid("u_Time")) {
        selectedPostProcessShader->useProgram();
        GLuint handle = selectedPostProcessShader->getUniformHandle("u_Time");
        glContext.glUniform1f(handle, currTime);
    }

    update();
}


void MyGL::initializeGL() {
    // Create an OpenGL context using Qt's
    // QOpenGLFunctions class.
    // If you were programming in a non-Qt context
    // you might use GLEW (GL Extension Wrangler)
    // or GLFW (Graphics Library Framework) instead.
    glContext.initializeOpenGLFunctions();
    // Print out some information about the current OpenGL context
    debugContextVersion();
    // Create a Vertex Array Object so that we can render
    // our geometry using Vertex Buffer Objects.
    glContext.glGenVertexArrays(1, &vao);
    // We have to have a VAO bound in OpenGL 3.2 Core.
    // But if we're not using multiple VAOs, we
    // can just bind one once.
    glContext.glBindVertexArray(vao);

    progLambert.createAndCompileShaderProgram("lambert.vert.glsl",
                                              "lambert.frag.glsl");
    printGLErrorLog();
    progBlinnPhong.createAndCompileShaderProgram("blinn_phong.vert.glsl",
                                                 "blinn_phong.frag.glsl");
    printGLErrorLog();
    progMatcap.createAndCompileShaderProgram("matcap.vert.glsl",
                                             "matcap.frag.glsl");
    printGLErrorLog();
    progCustomSurface.createAndCompileShaderProgram("custom_surface.vert.glsl",
                                                    "custom_surface.frag.glsl");
    printGLErrorLog();

    progPostProcessNoOp.createAndCompileShaderProgram("passthrough.vert.glsl",
                                                      "noOp.frag.glsl");
    printGLErrorLog();
    progPostProcessSobel.createAndCompileShaderProgram("passthrough.vert.glsl",
                                                       "sobel.frag.glsl");
    printGLErrorLog();
    progPostProcessBlur.createAndCompileShaderProgram("passthrough.vert.glsl",
                                                      "blur.frag.glsl");
    printGLErrorLog();
    progPostProcessCustom.createAndCompileShaderProgram("passthrough.vert.glsl",
                                                        "custom_post.frag.glsl");
    printGLErrorLog();
    progPostProcessMy.createAndCompileShaderProgram("passthrough.vert.glsl",
                                                        "my_post.frag.glsl");
    printGLErrorLog();

    meshDrawable.initializeAndBufferGeometryData();
    printGLErrorLog();
    quadDrawable.initializeAndBufferGeometryData();
    printGLErrorLog();

    postProcessFrameBuffer.create();
    printGLErrorLog();

    QString texturePath = getCurrentPath() + "/textures/wahoo.png";
    meshTexture.create(texturePath.toStdString().c_str());
    texturePath = getCurrentPath() + "/textures/smb.jpg";
    bgTexture.create(texturePath.toStdString().c_str());

    createMatcapTextures();

    // Enable depth sorting in the GL pipeline
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
}

void MyGL::resizeGL(int w, int h) {
    postProcessFrameBuffer.resize(w, h, this->devicePixelRatio());
    postProcessFrameBuffer.destroy();
    postProcessFrameBuffer.create();

    camera.recomputeAspectRatio(w, h);
}

void MyGL::assignSurfaceShaderUniforms() {
    selectedSurfaceShader->useProgram();

    if(selectedSurfaceShader->isUniformHandleValid("u_MVP")) {
        glm::mat4 mvp = camera.getViewProj();
        glContext.glUniformMatrix4fv(selectedSurfaceShader->getUniformHandle("u_MVP"),
                                     1,
                                     GL_FALSE,
                                     &mvp[0][0]);
        printGLErrorLog();
    }
    if(selectedSurfaceShader->isUniformHandleValid("u_View")) {
        glm::mat4 m = camera.getView();
        glContext.glUniformMatrix4fv(selectedSurfaceShader->getUniformHandle("u_View"),
                                     1,
                                     GL_FALSE,
                                     &m[0][0]);
        printGLErrorLog();
    }
    if(selectedSurfaceShader->isUniformHandleValid("u_Proj")) {
        glm::mat4 m = camera.getProj();
        glContext.glUniformMatrix4fv(selectedSurfaceShader->getUniformHandle("u_Proj"),
                                     1,
                                     GL_FALSE,
                                     &m[0][0]);
        printGLErrorLog();
    }
    // Pass the camera's forward vector to the current surface
    // shader if the shader uses it. For instance, we use
    // the camera's forward vector as the light direction
    // in the Lambert surface shader.
    if(selectedSurfaceShader->isUniformHandleValid("u_CamLook")) {
        selectedSurfaceShader->useProgram();
        GLuint handle = selectedSurfaceShader->getUniformHandle("u_CamLook");
        glContext.glUniform3f(handle,
                              camera.forward.x,
                              camera.forward.y,
                              camera.forward.z);
    }
    if(selectedSurfaceShader->isUniformHandleValid("u_CamPos")) {
        selectedSurfaceShader->useProgram();
        GLuint handle = selectedSurfaceShader->getUniformHandle("u_CamPos");
        glContext.glUniform3f(handle,
                              camera.eye.x,
                              camera.eye.y,
                              camera.eye.z);
    }

    // Bind the Matcap texture if we're rendering the
    // Matcap shader, otherwise bind the Mario texture
    if(selectedSurfaceShader == &progMatcap) {
        currentMatcapTex->bind(1);
        glContext.glUniform1i(selectedSurfaceShader->getUniformHandle("u_Texture"),
                              1);
    }
    else {
        meshTexture.bind(1);
        glContext.glUniform1i(selectedSurfaceShader->getUniformHandle("u_Texture"),
                              1);
    }
}

void MyGL::assignPostProcessShaderUniforms() {
    // TODO: Set the values of any uniform variables
    // in your post-process shaders.

    if(selectedPostProcessShader->isUniformHandleValid("u_Dimensions")) {
        selectedPostProcessShader->useProgram();
        GLuint handle = selectedPostProcessShader->getUniformHandle("u_Dimensions");
        glContext.glUniform2i(handle, width(), height());
    }
}

void MyGL::render3dScene() {
    // TODO: (Before you attempt the post-process shaders)
    // Bind the post-process frame buffer,
    // diverting the output of your render
    // calls to the texture within the frame buffer
    // rather than your MainWindow display.
    // After binding the frame buffer, use
    // glViewport to set your screen origin
    // and width + height. The width and height should
    // use MyGL's width() and height() multiplied by
    // MyGL's devicePixelRatio() (relevant for retina displays).

    postProcessFrameBuffer.bindFrameBuffer();
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());

    // Clear the screen so that we only see newly drawn images
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Draw the background of Super Mario Bros 1-1
    // as a screen-spanning quadrangle
    bgTexture.bind(2);
    progPostProcessNoOp.useProgram();
    glContext.glUniform1i(progPostProcessNoOp.getUniformHandle("u_Texture"),
                          2);
    progPostProcessNoOp.draw(quadDrawable);

    selectedSurfaceShader->draw(meshDrawable);

}

void MyGL::renderPostProcess() {
    // TODO: (Before you attempt the post-process shaders)
    // Use glBindFramebuffer to set the render output back
    // to your GL window. Normally, you would use 0 as the target
    // argument for glBindFramebuffer to do this, but when working
    // in a Qt environment you actually want to use
    // this->defaultFramebufferObject().
    // As in render3dScene, also set the glViewport.

    glContext.glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glContext.glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    printGLErrorLog();
    // Place the texture that stores the image of the 3D render
    // into texture slot 0
    postProcessFrameBuffer.bindToTextureSlot(0);
    printGLErrorLog();

    // Set the sampler2D in the post-process shader to
    // read from the texture slot that we set the
    // texture into
    selectedPostProcessShader->useProgram();
    glContext.glUniform1i(selectedPostProcessShader->getUniformHandle("u_Texture"),
                          postProcessFrameBuffer.getTextureSlot());
    // draw quad with post shader
    selectedPostProcessShader->draw(quadDrawable);
}

void MyGL::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    assignSurfaceShaderUniforms();
    assignPostProcessShaderUniforms();

    render3dScene();
#if 1
    renderPostProcess();
#endif
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if(e->buttons() & (Qt::LeftButton | Qt::RightButton))
    {
        m_mousePosPrev = glm::vec2(e->pos().x(), e->pos().y());
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    glm::vec2 pos(e->pos().x(), e->pos().y());
    if(e->buttons() & Qt::LeftButton)
    {
        // Rotation
        glm::vec2 diff = 0.2f * (pos - m_mousePosPrev);
        m_mousePosPrev = pos;
        camera.RotateAboutGlobalUp(-diff.x);
        camera.RotateAboutLocalRight(-diff.y);
    }
    else if(e->buttons() & Qt::RightButton)
    {
        // Panning
        glm::vec2 diff = 0.05f * (pos - m_mousePosPrev);
        m_mousePosPrev = pos;
        camera.PanAlongRight(-diff.x);
        camera.PanAlongUp(diff.y);
    }
}

void MyGL::wheelEvent(QWheelEvent *e) {
    camera.Zoom(e->angleDelta().y() * 0.02f);
}

void MyGL::slot_ChangeSurfaceShader(int idx) {
    switch(idx) {
    case 0:
        selectedSurfaceShader = &progLambert;
        break;
    case 1:
        selectedSurfaceShader = &progBlinnPhong;
        break;
    case 2:
        selectedSurfaceShader = &progMatcap;
        break;
    case 3:
        selectedSurfaceShader = &progCustomSurface;
        break;
    default:
        selectedSurfaceShader = &progLambert;
        break;
    }
}

void MyGL::slot_ChangePostShader(int idx) {
    switch(idx) {
    case 0:
        selectedPostProcessShader = &progPostProcessNoOp;
        break;
    case 1:
        selectedPostProcessShader = &progPostProcessSobel;
        break;
    case 2:
        selectedPostProcessShader = &progPostProcessBlur;
        break;
    case 3:
        selectedPostProcessShader = &progPostProcessCustom;
        break;
    case 4:
        selectedPostProcessShader = &progPostProcessMy;
        break;
    default:
        selectedPostProcessShader = &progPostProcessNoOp;
        break;
    }
}

void MyGL::slot_ChangeMatcap(int idx) {
    currentMatcapTex = matcapTextures[idx].get();
}

void MyGL::createMatcapTextures() {
    uPtr<Texture> t = mkU<Texture>(&glContext);
    currentMatcapTex = t.get();
    QString texturePath = getCurrentPath() + "/textures/red_wax.png";
    t->create(texturePath.toStdString().c_str());
    matcapTextures.push_back(std::move(t));

    t = mkU<Texture>(&glContext);
    texturePath = getCurrentPath() + "/textures/silver.jpg";
    t->create(texturePath.toStdString().c_str());
    matcapTextures.push_back(std::move(t));

    t = mkU<Texture>(&glContext);
    texturePath = getCurrentPath() + "/textures/plastic.png";
    t->create(texturePath.toStdString().c_str());
    matcapTextures.push_back(std::move(t));

    t = mkU<Texture>(&glContext);
    texturePath = getCurrentPath() + "/textures/gold.png";
    t->create(texturePath.toStdString().c_str());
    matcapTextures.push_back(std::move(t));

    t = mkU<Texture>(&glContext);
    texturePath = getCurrentPath() + "/textures/haunted.png";
    t->create(texturePath.toStdString().c_str());
    matcapTextures.push_back(std::move(t));

    t = mkU<Texture>(&glContext);
    texturePath = getCurrentPath() + "/textures/normals.jpg";
    t->create(texturePath.toStdString().c_str());
    matcapTextures.push_back(std::move(t));
}

#ifndef MYGL_H
#define MYGL_H

#include <openglcontext.h>
#include <utils.h>
#include <shaderprogram.h>
#include "scene/squareplane.h"
#include "scene/mesh.h"
#include "scene/cube.h"
#include "camera.h"
#include "framebuffer.h"
#include "texture.h"

#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>

 // -X, +X, -Y, +Y, -Z, +Z
const static std::array<glm::mat4, 6> views =
{
glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
};


class MyGL
    : public OpenGLContext
{
    Q_OBJECT
private:
    SquarePlane m_geomSquare;
    Mesh m_geomMesh;
    Texture2D m_textureAlbedo;
    Texture2D m_textureMetallic;
    Texture2D m_textureNormals;
    Texture2D m_textureRoughness;
    Texture2D m_textureAO;
    Texture2D m_textureDisplacement;

    // Used for rendering each face of the cubemap to a frame buffer
    // Also used to render the cube map as a background
    Cube m_geomCube;
    // Used for loading the HDR environment map from disk to the GPU
    Texture2DHDR m_hdrEnvMap;
    // Used for converting the 2D HDR environment map to an OpenGL cubemap
    CubeMapFrameBuffer m_environmentCubemapFB;
    // Used for convoluting the 3D HDR environment cubemap into a diffuse irradiance cubemap
    CubeMapFrameBuffer m_diffuseIrradianceFB;
    // Used for convoluting the 3D HDR environment cubemap into a mip-mapped glossy irradiance map
    CubeMapFrameBuffer m_glossyIrradianceFB;
    // Used for the split-sum approximation of glossy reflection
    Texture2D m_brdfLookupTexture;

    ShaderProgram m_progPBR;
    ShaderProgram m_progCubemapConversion;
    ShaderProgram m_progCubemapDiffuseConvolution;
    ShaderProgram m_progCubemapGlossyConvolution;
    ShaderProgram m_progEnvMap;

    GLuint vao; // A handle for our vertex array object. This will store the VBOs created in our geometry classes.
                // Don't worry too much about this. Just know it is necessary in order to render geometry.

    Camera m_glCamera;
    glm::vec2 m_mousePosPrev;

    glm::vec3 m_albedo;

    bool m_cubemapsNotGenerated;

    QString getCurrentPath() const;


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

    void renderCubeMapToTexture();
    void renderConvolvedDiffuseCubeMapToTexture();
    void renderConvolvedGlossyCubeMapToTexture();
    void renderEnvironmentMap();

    void setupShaderHandles();

public slots:
    void slot_setRed(int);
    void slot_setGreen(int);
    void slot_setBlue(int);
    void slot_setMetallic(int);
    void slot_setRoughness(int);
    void slot_setAO(int);
    void slot_setDisplacement(double);
    void slot_loadEnvMap();
    void slot_loadScene();
    void slot_loadOBJ();
    void slot_revertToSphere();
};


#endif // MYGL_H

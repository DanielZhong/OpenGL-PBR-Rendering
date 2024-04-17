#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QString>
#include <unordered_map>
#include <QMouseEvent>
#include "shaderprogram.h"
#include "drawables/drawables.h"
#include "framebuffer.h"
#include "camera.h"
#include <QTimer>

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

class MyGL : public QOpenGLWidget
{
    Q_OBJECT

private:
    QOpenGLFunctions_3_3_Core glContext;

    QTimer timer;
    float currTime;

    GLuint vao;

    // Renders various surface attributes
    // to different textures via a frame buffer
    // known collectively as the Geometry Buffer
    ShaderProgram progMeshToGBuffer;

    // Reads the G-Buffer and renders the screen-space
    // reflection of the scene to another attachment in
    // the G-Buffer.
    ShaderProgram progComputeScreenSpaceReflection;

    // Reads the SSR image in the G-Buffer and computes
    // different levels of blur of it. Results are stored
    // in
    ShaderProgram progBlurScreenReflection;

    // Combines the G-buffer textures into a
    // final image
    ShaderProgram progFinalOutput;

    // Used for setting up environment map convolution
    ShaderProgram progCubemapConversion;
    ShaderProgram progCubemapDiffuseConvolution;
    ShaderProgram progCubemapGlossyConvolution;
    ShaderProgram progEnvMap;

    bool cubemapsNotGenerated;

    Mesh meshDrawable;
    Quad quadDrawable;
    Cube cubeDrawable;

    // OBJ file material properties
    Texture textureAlbedo;
    Texture textureMetallic;
    Texture textureNormals;
    Texture textureRoughness;
    Texture textureAO;
    Texture textureDisplacement;

    // 2D representation of HDR environment map
    Texture textureHDREnvMap;
    // Used for the split-sum approximation of glossy reflection
    Texture textureBrdfLookup;

    // Used for converting the 2D HDR environment map to an OpenGL cubemap
    CubeMapFrameBuffer environmentCubemapFB;
    // Used for convoluting the 3D HDR environment cubemap into a diffuse irradiance cubemap
    CubeMapFrameBuffer diffuseIrradianceFB;
    // Used for convoluting the 3D HDR environment cubemap into a mip-mapped glossy irradiance map
    CubeMapFrameBuffer glossyIrradianceFB;

    FrameBuffer geometryBuffer;

    Texture gaussianKernel;

    Camera camera;
    // A variable used to track the mouse's previous position when
    // clicking and dragging on the GL viewport. Used to move the camera
    // in the scene.
    glm::vec2 m_mousePosPrev;

    void setupGBuffer();
    // Add code to this function to draw the Mesh stored in MyGL.
    // When you go to implement the post-process portion of this
    // assignment, remember to bind the frame buffer you'll use
    // to store the render results.
    void renderSceneToGBuffer();

    // Read the G-Buffer to compute the screen-space reflection
    // of the scene, then write that SSR to the G-Buffer
    // so that it can be blurred at varying levels. The blurred
    // versions of the reflection will be read in combineGBufferIntoImage()
    // as the reflection shown in varying levels of surface roughness.
    void intermediateScreenSpaceReflToGBuffer();

    // Read the SSR texture in the G-buffer and create several
    // different levels of Gaussian blur to it, to act as faked
    // microfacet reflections of the scene.
    void intermediateBlurSSR();

    // Add code to this function to draw the Quad geometry stored
    // in MyGL, using a post-process shader so that it displays
    // the texture written to by the frame buffer used in render3dScene.
    void combineGBufferIntoImage();

    std::vector<GLfloat> computeGaussianKernel(int radius);

public:
    MyGL(QWidget *parent);
    ~MyGL();

    void initializeGL() override;

    void resizeGL(int w, int h) override;

    void paintGL() override;

    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

    void renderCubeMapToTexture();
    void renderConvolvedDiffuseCubeMapToTexture();
    void renderConvolvedGlossyCubeMapToTexture();
    void renderEnvironmentMap();

    void loadEnvMap();
    void loadScene();

    // Prints out the highest version of OpenGL supported by your hardware
    void debugContextVersion();

public slots:
    void tick();
};

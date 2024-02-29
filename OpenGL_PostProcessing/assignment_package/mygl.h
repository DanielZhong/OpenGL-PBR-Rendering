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

class MyGL : public QOpenGLWidget
{
    Q_OBJECT

private:
    // In order for multiple different classes to have
    // access to the OpenGL API using Qt's QOpenGLFunctions
    // class, we need to make an instance of QOpenGLFunctions
    // and pass a pointer to it to every object that needs it.
    // So, MyGL holds the object and passes it to every
    // ShaderProgram and Drawable.
    QOpenGLFunctions_3_3_Core glContext;

    // A timer that is set up to fire every 16 milliseconds,
    // and causes MyGL::tick() to be called each time.
    // You can use tick() to update your shader's time variable
    // if you want to animate anything.
    QTimer timer;
    float currTime;

    // A handle for our Vertex Array Object.
    // This will store the Vertex Buffer Objects
    // that we use to represent our geometry data.
    // We've already written all the code needed to
    // interact with this.
    GLuint vao;

    /// Surface shaders
    // Applies the Lambertian reflection model to
    // the drawn mesh
    ShaderProgram progLambert;
    // Applies the Blinn-Phong reflection model to
    // the drawn mesh
    ShaderProgram progBlinnPhong;
    // Applies the matcap / lit sphere reflection
    // model to the drawn mesh
    ShaderProgram progMatcap;
    // Applies a non-uniform deformation to the
    // mesh's vertices, and a non-uniform coloration
    // to its fragments.
    ShaderProgram progCustomSurface;

    /// Post-process shaders
    // A shader for performing the post-process render pass
    // with the Quad geometry. Just renders the texture
    // with no alterations.
    ShaderProgram progPostProcessNoOp;
    // Applies a Sobel edge-detection filter to
    // the image
    ShaderProgram progPostProcessSobel;
    // Applies a Gaussian blur to the image
    ShaderProgram progPostProcessBlur;
    // Applies a custom post-process effect
    ShaderProgram progPostProcessCustom;

    ShaderProgram progPostProcessMy;

    // Pointers to the shaders currently chosen in the
    // shader controls GUI.
    ShaderProgram *selectedSurfaceShader,
                  *selectedPostProcessShader;

    Mesh meshDrawable;
    Quad quadDrawable;

    FrameBuffer postProcessFrameBuffer;

    Texture meshTexture;
    Texture bgTexture;
    std::vector<uPtr<Texture>> matcapTextures;
    Texture *currentMatcapTex;
    void createMatcapTextures();


    Camera camera;
    // A variable used to track the mouse's previous position when
    // clicking and dragging on the GL viewport. Used to move the camera
    // in the scene.
    glm::vec2 m_mousePosPrev;

    // Called from render3dScene(). Passes the appropriate
    // values into the uniform variables read by the currently
    // selected surface shader program.
    void assignSurfaceShaderUniforms();
    void assignPostProcessShaderUniforms();
    // Add code to this function to draw the Mesh stored in MyGL.
    // When you go to implement the post-process portion of this
    // assignment, remember to bind the frame buffer you'll use
    // to store the render results.
    void render3dScene();
    // Add code to this function to draw the Quad geometry stored
    // in MyGL, using a post-process shader so that it displays
    // the texture written to by the frame buffer used in render3dScene.
    void renderPostProcess();

public:
    MyGL(QWidget *parent);
    ~MyGL();

    /// The following three functions are automatically called
    /// by QOpenGLWidget (MyGL's parent class)

    // Use this function to initialize all of your OpenGL
    // components, like your shader program(s), vertex buffers,
    // shader variable handles, etc.
    // Called automatically by QOpenGLWidget one time only,
    // just after MyGL is constructed.
    void initializeGL() override;
    // Use this function to pass any window size information to
    // your shaders.
    // Called automatically whenever the OpenGL window is resized,
    // including when MyGL is first initialized then sized up to the
    // dimensions set for it in the UI editor.
    void resizeGL(int w, int h) override;
    // Make all of your draw calls in this function; doing them
    // anywhere else won't work. Called *once* automatically
    // by MyGL's initialization, then you have to manually call it
    // using QOpenGLWidget's update() function. DO NOT CALL paintGL()
    // directly! Only call it by calling update()!
    void paintGL() override;

    /// The following function is automatically called whenever Qt detects
    /// a mouse movement. We will use it to pass out cursor's current location
    /// to our shader.
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void wheelEvent(QWheelEvent *e) override;

    /// The functions below are useful for determining what's going wrong
    /// with your OpenGL code

    // Prints out the highest version of OpenGL supported by your hardware
    void debugContextVersion();

public slots:
    void slot_ChangeSurfaceShader(int);
    void slot_ChangePostShader(int);
    void slot_ChangeMatcap(int);
    void tick();
};

#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QDir>
#include "texture.h"
#include <QFileDialog>

#include "scene/jsonreader.h"


MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      jsonReader(this),
      m_geomSquare(this),// m_geomCube(this),
      m_hdrEnvMap(this, 0, ENV_MAP_FLAT_TEX_SLOT),
      m_environmentCubemapFB(this, 1024, 1024, 1.f),
      m_progPathTracer(this), m_progDisplay(this), m_progCubemapConversion(this),
      m_renderPassOutputFBs{FrameBuffer2D(this, this->width(), this->height(), this->devicePixelRatio()),
                            FrameBuffer2D(this, this->width(), this->height(), this->devicePixelRatio())},
      m_glCamera(), m_mousePosPrev(), m_iterations(0), m_timer(),
      m_cubemapsNotGenerated(true), m_scene()
{
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
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

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Set a few settings/modes in OpenGL rendering
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.5, 0.5, 0.5, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instances of Cylinder and Sphere.
    m_geomSquare.create();
//    m_geomCube.create();

    m_scene.CreateTestScene();
    QString vertASCII = qTextFileRead(":/glsl/passthrough.vert.glsl");
    QString fragASCII = writeFullShaderFile();

    m_progPathTracer.create(vertASCII, fragASCII);

    m_progDisplay.create(":/glsl/passthrough.vert.glsl", ":/glsl/noOp.frag.glsl");
    m_progCubemapConversion.create(":/glsl/cubemap.vert.glsl", ":/glsl/cubemap_uv_conversion.frag.glsl");

    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    initShaderHandles(false);
    m_progPathTracer.m_isReloading = false;

    QString path = getCurrentPath();
    path.append("/path_tracer/environment_maps/Frozen_Waterfall_Ref.hdr");
    m_hdrEnvMap.create(path.toStdString().c_str(), false);

    m_environmentCubemapFB.create(true);

    m_timer.start(16);
}

void MyGL::resizeGL(int w, int h)
{
    m_glCamera = Camera(w, h);
    m_glCamera.RecomputeAttributes();
    m_progPathTracer.setUnifVec3("u_Eye", m_glCamera.eye);
    m_progPathTracer.setUnifVec3("u_Forward", m_glCamera.look);
    m_progPathTracer.setUnifVec3("u_Right", m_glCamera.right);
    m_progPathTracer.setUnifVec3("u_Up", m_glCamera.up);

    m_renderPassOutputFBs[0].resize(width(), height(), devicePixelRatio());
    m_renderPassOutputFBs[1].resize(width(), height(), devicePixelRatio());

    m_renderPassOutputFBs[0].destroy();
    m_renderPassOutputFBs[1].destroy();

    m_renderPassOutputFBs[0].create();
    m_renderPassOutputFBs[1].create();

    m_progPathTracer.setUnifVec2("u_ScreenDims", glm::vec2(w, h));
    m_progDisplay.setUnifVec2("u_ScreenDims", glm::vec2(w, h));

    printGLErrorLog();
}

//This function is called by Qt any time your GL window is supposed to update
//For example, when the function update() is called, paintGL is called implicitly.
void MyGL::paintGL() {
    if(m_progPathTracer.m_isReloading) return;

    int currBufferID = m_iterations % 2;
    int prevBufferID = (m_iterations + 1) % 2;

    // Render the next iteration of the path tracer to texture
    m_renderPassOutputFBs[currBufferID].bindFrameBuffer();
    glViewport(0,0,
               m_renderPassOutputFBs[currBufferID].width(),
               m_renderPassOutputFBs[currBufferID].height());
    glClearColor(1, 0.5, 1, 1);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_renderPassOutputFBs[prevBufferID].bindToTextureSlot(PATH_TRACER_INPUT_TEX_SLOT);
    m_progPathTracer.setUnifInt("u_AccumImg", PATH_TRACER_INPUT_TEX_SLOT);

    // Allow the path tracer shader to read the loaded environment
    // cube map
    m_hdrEnvMap.bind(ENV_MAP_FLAT_TEX_SLOT);
    m_progPathTracer.setUnifInt("u_EnvironmentMap", ENV_MAP_FLAT_TEX_SLOT);
    m_progPathTracer.setUnifInt("u_Iterations", ++m_iterations);

    // Bind any 2D textured used by materials in the scene
    for(unsigned int i = 0; i < m_scene.textures.size(); ++i) {
        Texture &t = *(m_scene.textures[i]);
        t.bind(t.m_associatedTextureSlot);
        m_progPathTracer.setUnifArrayInt("u_TexSamplers", i, t.m_associatedTextureSlot);
    }

    // Bind any textures used to store Mesh triangles
    for(unsigned int i = 0; i < m_scene.meshes.size(); ++i) {
        auto &m = m_scene.meshes[i];
        m->trianglesAsTexture->bind(m->trianglesAsTexture->m_associatedTextureSlot);
        m_progPathTracer.setUnifArrayInt("u_TriangleStorageSamplers", i, m->trianglesAsTexture->m_associatedTextureSlot);
    }

    m_progPathTracer.draw(m_geomSquare);

    // Display the just-rendered iteration
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0,0,this->width() * this->devicePixelRatio(), this->height() * this->devicePixelRatio());
    glClearColor(0.5, 0.5, 0.5, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_renderPassOutputFBs[currBufferID].bindToTextureSlot(PATH_TRACER_OUTPUT_TEX_SLOT);
    m_progDisplay.setUnifInt("u_Texture", PATH_TRACER_OUTPUT_TEX_SLOT);
    m_progDisplay.setUnifInt("u_Iterations", m_iterations);

    m_progDisplay.draw(m_geomSquare);
}

void MyGL::initShaderHandles(bool pathtracerProgOnly) {
    // Shader for path tracing
    m_progPathTracer.addAttrib("vs_Pos");
    m_progPathTracer.addAttrib("vs_UV");

    m_progPathTracer.addUniform("u_Eye");
    m_progPathTracer.addUniform("u_Forward");
    m_progPathTracer.addUniform("u_Right");
    m_progPathTracer.addUniform("u_Up");
    m_progPathTracer.addUniform("u_AccumImg");
    m_progPathTracer.addUniform("u_ScreenDims");
    m_progPathTracer.addUniform("u_Iterations");
    m_progPathTracer.addUniform("u_EnvironmentMap");
    m_progPathTracer.addUniform("u_TriangleStorageSamplers");

    if(!pathtracerProgOnly) {
        // Shader for displaying the sum of all PT iterations.
        // Also applies color correction & filtering.
        m_progDisplay.addAttrib("vs_Pos");
        m_progDisplay.addAttrib("vs_UV");

        m_progDisplay.addUniform("u_Texture");
        m_progDisplay.addUniform("u_Iterations");
        m_progDisplay.addUniform("u_ScreenDims");

        // Shader for rendering the 2D environment map
        // to a cubemap
        m_progCubemapConversion.addAttrib("vs_Pos");

        m_progCubemapConversion.addUniform("u_EquirectangularMap");
        m_progCubemapConversion.addUniform("u_ViewProj");
    }
}

QString qTextFileRead(const char *fileName) {
    QString text;
    QFile file(fileName);
    if(file.open(QFile::ReadOnly))
    {
        QTextStream in(&file);
        text = in.readAll();
        text.append('\0');
    }
    return text;
}

QString MyGL::writeFullShaderFile() const {
    std::vector<const char*> fragfile_sections = {
        ":/glsl/pathtracer.sampleWarping.glsl",
        ":/glsl/pathtracer.bsdf.glsl",
        ":/glsl/pathtracer.intersection.glsl",
        ":/glsl/pathtracer.light.glsl",
        ":/glsl/pathtracer.frag.glsl"
    };

    QString qFragSource = qTextFileRead(":/glsl/pathtracer.defines.glsl");
    qFragSource.chop(1); // Must remove the \0 at the end of the previous section
    qFragSource = qFragSource + "\n" + this->m_scene.toGLSL() + "\n";

    int lineCount = 0;
    for(auto &c : fragfile_sections) {
        QString section = qTextFileRead(c);
        lineCount += section.count("\n");
        qFragSource.chop(1); // Must remove the \0 at the end of the previous section
        qFragSource = qFragSource + "\n" + section;
    }

    // Write the entire frag source to a single file so that
    // we the programmer can read & debug our code in one file
    QString path = getCurrentPath();
    QFile entireShader(path + "/path_tracer/glsl/pathtracer.all.glsl");
    if (entireShader.open(QIODevice::ReadWrite)) {
        entireShader.resize(0); // Clear out the old file contents
        QTextStream stream(&entireShader);
        stream << qFragSource << Qt::endl;
    }
    entireShader.close();
    return qFragSource;
}

void MyGL::resetPathTracer() {
    m_iterations = 0;
    m_progPathTracer.setUnifInt("u_Iterations", m_iterations);
    m_progDisplay.setUnifInt("u_Iterations", m_iterations);
}

void MyGL::tick() {
    update();
}


void MyGL::loadEnvMap() {
    QString path = getCurrentPath();
    path.append("/path_tracer/environment_maps/");
    QString filepath = QFileDialog::getOpenFileName(
                        0, QString("Load Environment Map"),
                        path, tr("*.hdr"));
    Texture2DHDR tex(this, 0, ENV_MAP_FLAT_TEX_SLOT);
    try {
        tex.create(filepath.toStdString().c_str(), false);
    }
    catch(std::exception &e) {
        std::cout << "Error: Failed to load HDR image" << std::endl;
        return;
    }
    this->m_hdrEnvMap.destroy();
    this->m_hdrEnvMap = tex;
    resetPathTracer();
    update();
}

void MyGL::loadJSON() {
    QString path = getCurrentPath();
    path.append("/path_tracer/jsons/");
    QString filepath = QFileDialog::getOpenFileName(
                        0, QString("Load JSON Scene"),
                        path, tr("*.json"));

    QFile file(filepath);
    // TODO: Use the JSONReader class to parse the file
    // Then destroy() the pathtracer shader, write a new pathtracer.all.glsl,
    // then create() the pathtracer shader again.

    JSONReader reader(this);
    if(reader.LoadSceneFromFile(file, path, m_scene, m_glCamera)) {
        QString fragASCII = writeFullShaderFile();
        QString vertASCII = qTextFileRead(":/glsl/passthrough.vert.glsl");
        m_progPathTracer.m_isReloading = true;
        m_progPathTracer.destroy();
        m_progPathTracer.create(vertASCII, fragASCII);
        initShaderHandles(true);
        m_progPathTracer.setUnifVec2("u_ScreenDims", glm::vec2(width(), height()));
        m_progPathTracer.m_isReloading = false;

        m_glCamera.RecomputeAttributes();
        m_progPathTracer.setUnifVec3("u_Eye", m_glCamera.eye);
        m_progPathTracer.setUnifVec3("u_Forward", m_glCamera.look);
        m_progPathTracer.setUnifVec3("u_Right", m_glCamera.right);
        m_progPathTracer.setUnifVec3("u_Up", m_glCamera.up);
        if(m_scene.textures.size() > 0) {
            m_progPathTracer.addUniform("u_TexSamplers");
        }
        resetPathTracer();
        update();
    }
    else {
        std::cout << "Could not load JSON scene" << std::endl;
    }
}


QString MyGL::getCurrentPath() const {
    QString path = QDir::currentPath();
    path = path.left(path.lastIndexOf("/"));
#ifdef __APPLE__
    path = path.left(path.lastIndexOf("/"));
    path = path.left(path.lastIndexOf("/"));
    path = path.left(path.lastIndexOf("/"));
#endif
    return path;
}

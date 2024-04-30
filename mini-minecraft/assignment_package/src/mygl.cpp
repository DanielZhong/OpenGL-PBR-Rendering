#include "mygl.h"
#include <glm_includes.h>
#include <QTime>
#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

glm::vec3 lightDir = glm::vec3(3, -3, 0.75);

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
   //  m_worldAxes(this),
    m_progGbuffer(this), m_progFlat(this), m_progInstanced(this),
    m_terrain(this), m_player(glm::vec3(148.778f, 160.f, 140.32f), m_terrain),
    m_progLighting(this), m_progPostProcessing(this), m_texture(this, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    m_textureNormal(this, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE), quadDrawable(this), init_terrain(false),
    geometryBuffer(this,
                   this->width(), this->height(),
                     this->devicePixelRatio()),     ShadowMapBuffer(this,
                     this->width(), this->height(),
                     this->devicePixelRatio()), m_progWater(this), m_progLava(this), m_progShadow(this), m_progSky(this)
{
    // Connect the timer to a function so that when the timer ticks the function is executed
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
    // Tell the timer to redraw 60 times per second
    m_timer.start(16);
    setFocusPolicy(Qt::ClickFocus);

    setMouseTracking(true); // MyGL will track the mouse's movements even if a mouse button is not pressed
    setCursor(Qt::BlankCursor); // Make the cursor invisible
}

MyGL::~MyGL() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    geometryBuffer.destroy();
    ShadowMapBuffer.destroy();
    quadDrawable.destroyVBOdata();
}


void MyGL::moveMouseToCenter() {
    QCursor::setPos(this->mapToGlobal(QPoint(width() / 2, height() / 2)));
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
    glDepthFunc(GL_LEQUAL);
    // Set the color with which the screen is filled at the start of each render call.
    //glClearColor(0.37f, 0.74f, 1.0f, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    //Create the instance of the world axes
    // m_worldAxes.createVBOdata();

    quadDrawable.createVBOdata();

    setupGBuffer();

    // Create and set up the diffuse shader
    m_progGbuffer.create(":/glsl/gbuffer.vert.glsl", ":/glsl/gbuffer.frag.glsl");
    // Create and set up the flat lighting shader
    m_progFlat.create(":/glsl/flat.vert.glsl", ":/glsl/flat.frag.glsl");
    m_progSky.create(":/glsl/sky.vert.glsl", ":/glsl/sky.frag.glsl");
    // m_progInstanced.create(":/glsl/instanced.vert.glsl", ":/glsl/lambert.frag.glsl");
    m_progWater.create(":/glsl/passthrough.vert.glsl", ":/glsl/water.frag.glsl");
    m_progLava.create(":/glsl/passthrough.vert.glsl", ":/glsl/lava.frag.glsl");
    m_progPostProcessing.create(":/glsl/passthrough.vert.glsl", ":/glsl/passthrough.frag.glsl");
    m_progLighting.create(":/glsl/lighting.vert.glsl", ":/glsl/lighting.frag.glsl");
    m_progShadow.create(":/glsl/shadow.vert.glsl", ":/glsl/shadow.frag.glsl");

    QString localPath = getCurrentPath();
    QString texturePath = localPath + "/textures/minecraft_textures_all.png";
    m_texture.create(texturePath.toStdString().c_str(), false, false, true, false);

    texturePath = localPath + "/textures/minecraft_normals_all.png";
    m_textureNormal.create(texturePath.toStdString().c_str(), false, false, false, false);


    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    // m_terrain.CreateTestScene();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void MyGL::resizeGL(int w, int h) {
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_player.setCameraWidthHeight(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progGbuffer.setUnifMat4("u_ViewProj", viewproj);
    m_progFlat.setUnifMat4("u_ViewProj", viewproj);
    m_progSky.setUnifMat4("u_ViewProj", glm::inverse(viewproj));
    // m_progInstanced.setUnifMat4("u_ViewProj", viewproj);

    m_progSky.useMe();
    this->glUniform2i(m_progSky.m_unifs["u_Dimensions"], width() * this->devicePixelRatio(), height() * this->devicePixelRatio());

    geometryBuffer.resize(w, h, this->devicePixelRatio());
    geometryBuffer.destroy();
    geometryBuffer.create();


    ShadowMapBuffer.resize(w, h, this->devicePixelRatio());
    ShadowMapBuffer.destroy();
    ShadowMapBuffer.create();
    setupGBuffer();

    printGLErrorLog();
}

void MyGL::setupGBuffer() {
    printGLErrorLog();
    geometryBuffer.resize(width(), height(), devicePixelRatio());
    geometryBuffer.create();
    ShadowMapBuffer.resize(4096, 4096, 1);
    ShadowMapBuffer.create();
    printGLErrorLog();
    geometryBuffer.addTexture(GBufferOutputType::ALBEDO);
    geometryBuffer.addTexture(GBufferOutputType::NORMAL);
    geometryBuffer.addTexture(GBufferOutputType::LIGHTING);
    geometryBuffer.addTexture(GBufferOutputType::SCENE);
    ShadowMapBuffer.addTexture(GBufferOutputType::DEPTH);
    geometryBuffer.addTexture(GBufferOutputType::POSITION_WORLD);
    geometryBuffer.addTexture(GBufferOutputType::SHADOW);
    geometryBuffer.addTexture(GBufferOutputType::FOG);
    geometryBuffer.addTexture(GBufferOutputType::SKY);
    printGLErrorLog();
}


// MyGL's constructor links tick() to a timer that fires 60 times per second.
// We're treating MyGL as our game engine class, so we're going to perform
// all per-frame actions here, such as performing physics updates on all
// entities in the scene.
void MyGL::tick() {
    auto prePos = m_player.mcr_position;
    qint64 newTime = QDateTime::currentMSecsSinceEpoch();
    float dT = (QDateTime::currentMSecsSinceEpoch() - m_lastFrameTimeMS) / 1000.f;
    m_lastFrameTimeMS = newTime;
    if(init_terrain == true) {
        m_player.tick(dT, m_inputs);

    }
    m_terrain.tryExpand(m_player.mcr_position[0], m_player.mcr_position[2], prePos[0], prePos[2], 8, init_terrain);
    //check if intial terrain loaded
        //if true call player tick
    if(init_terrain == true) {
        reset();
        update();
    }
    // Calls paintGL() as part of a larger QOpenGLWidget pipeline
    sendPlayerDataToGUI(); // Updates the info in the secondary window displaying player data
    //inital terrain loaded = true
    init_terrain = true; // Updates the info in the secondary window displaying player data
}

void MyGL::reset() {
    m_inputs.mouseX = 0;
    m_inputs.mouseY = 0;
}

void MyGL::sendPlayerDataToGUI() const {
    emit sig_sendPlayerPos(m_player.posAsQString());
    emit sig_sendPlayerVel(m_player.velAsQString());
    emit sig_sendPlayerAcc(m_player.accAsQString());
    emit sig_sendPlayerLook(m_player.lookAsQString());
    glm::vec2 pPos(m_player.mcr_position.x, m_player.mcr_position.z);
    glm::ivec2 chunk(16 * glm::ivec2(glm::floor(pPos / 16.f)));
    glm::ivec2 zone(64 * glm::ivec2(glm::floor(pPos / 64.f)));
    emit sig_sendPlayerChunk(QString::fromStdString("( " + std::to_string(chunk.x) + ", " + std::to_string(chunk.y) + " )"));
    emit sig_sendPlayerTerrainZone(QString::fromStdString("( " + std::to_string(zone.x) + ", " + std::to_string(zone.y) + " )"));
}

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& view)
{
    const auto inv = glm::inverse(view);

    std::vector<glm::vec4> frustumCorners;
    for (unsigned int x = 0; x < 2; ++x)
    {
        for (unsigned int y = 0; y < 2; ++y)
        {
            for (unsigned int z = 0; z < 2; ++z)
            {
                const glm::vec4 pt =
                    inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f,
                        1.0f);
                frustumCorners.push_back(pt / pt.w);
            }
        }
    }

    return frustumCorners;
}

glm::mat4 getLightSpaceMatrix(const glm::mat4 view)
{
    const auto corners = getFrustumCornersWorldSpace(view);

    glm::vec3 center = glm::vec3(0, 0, 0);
    for (const auto& v : corners)
    {
        center += glm::vec3(v);
    }
    center /= corners.size();

    const auto lightView = glm::lookAt(center - lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    float minZ = std::numeric_limits<float>::max();
    float maxZ = std::numeric_limits<float>::lowest();
    for (const auto& v : corners)
    {
        const auto trf = lightView * v;
        minX = std::min(minX, trf.x);
        maxX = std::max(maxX, trf.x);
        minY = std::min(minY, trf.y);
        maxY = std::max(maxY, trf.y);
        minZ = std::min(minZ, trf.z);
        maxZ = std::max(maxZ, trf.z);
    }

    // Tune this parameter according to the scene
    constexpr float zMult = 1.0f;
    if (minZ < 0)
    {
        minZ *= zMult;
    }
    else
    {
        minZ /= zMult;
    }
    if (maxZ < 0)
    {
        maxZ /= zMult;
    }
    else
    {
        maxZ *= zMult;
    }

    const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    return lightProjection * lightView;
}

glm::vec3 rotateX(glm::vec3 p, float a) {
    return glm::vec3(p.x, cos(a) * p.y + -sin(a) *p.z, sin(a) * p.y +cos(a) * p.z);
}

void MyGL::bindProgramUniform() {
    glm::mat4 viewproj = m_player.mcr_camera.getViewProj();
    m_progGbuffer.setUnifMat4("u_ViewProj", viewproj);
    m_progFlat.setUnifMat4("u_ViewProj", viewproj);
    m_progShadow.setUnifMat4("u_ViewProj", viewproj);
    m_progSky.setUnifMat4("u_ViewProj", glm::inverse(viewproj));

    // m_progInstanced.setUnifMat4("u_ViewProj", viewproj);
    m_progGbuffer.setUnifFloat("u_Time", m_totalTime);
    m_progSky.setUnifFloat("u_Time", m_totalTime * 0.02);
    m_progWater.setUnifFloat("u_Time", m_totalTime);
    m_progLava.setUnifFloat("u_Time", m_totalTime);
    m_totalTime++;

    // glm::mat4 lightSpaceMatrix = getLightSpaceMatrix(m_player.mcr_camera.getViewProj());
    float near_plane = 1.0f, far_plane = 100.f;
    lightDir = -glm::normalize(rotateX(glm::normalize(glm::vec3(0, 0, -1.0)), m_totalTime * 0.05 * 0.02));
    if (lightDir.y > 0.01f) {
        lightDir = glm::vec3(0.f);
    }
    glm::mat4 depthProjectionMatrix = glm::ortho<float>(-100.f, 100.f, -100.f, 100.f, near_plane, far_plane);
    glm::mat4 depthViewMatrix = glm::lookAt( 20.f * -lightDir +  m_player.mcr_camera.m_position, m_player.mcr_camera.m_position, glm::vec3(0, 1, 0));
    glm::mat4 depthModelMatrix = glm::mat4(1.0);
    glm::mat4 lightSpaceMatrix = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

    m_progShadow.setUnifMat4("u_ViewProj", lightSpaceMatrix);
    m_progGbuffer.setUnifMat4("u_DepthMVP", lightSpaceMatrix);

    m_progLighting.setUnifVec3("u_lightDir", lightDir);
    m_progLighting.setUnifVec3("u_CameraPos", m_player.mcr_camera.m_position);
    m_progGbuffer.setUnifVec3("u_CameraPos", m_player.mcr_camera.m_position);
}

// This function is called whenever update() is called.
// MyGL's constructor links update() to a timer that fires 60 times per second,
// so paintGL() called at a rate of 60 frames per second.
void MyGL::paintGL() {
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bindProgramUniform();

    skyPass();
    glCullFace(GL_FRONT);
    shadowPass();
    glCullFace(GL_BACK);
    renderTerrain();

    lightingPass();
    if (m_totalTime > 100) {

        if (m_player.InLavaWater(m_terrain, WATER)) {
            postprocessingPass(m_progWater);
        } else if (m_player.InLavaWater(m_terrain, LAVA)) {
            postprocessingPass(m_progLava);
        } else {
            postprocessingPass(m_progPostProcessing);
        }
    }


    // glDisable(GL_DEPTH_TEST);
    // m_progFlat.setUnifMat4("u_Model", glm::mat4());
    // m_progFlat.draw(m_worldAxes);
    // glEnable(GL_DEPTH_TEST);
}

void MyGL::skyPass() {
    glViewport(0,0,
               this->width() * this->devicePixelRatio(),
               this->height() * this->devicePixelRatio());
    glClearColor(0.f, 0.f, 0.f, 1.f);
    std::vector<GBufferOutputType> attachments = {
        GBufferOutputType::SKY,
    };
    geometryBuffer.setDrawBuffers(attachments);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_progSky.draw(quadDrawable);
}

void MyGL::shadowPass() {


    glm::vec3 player_pos = m_player.mcr_position;
    glViewport(0,0,
               4096,
               4096);

    glClear(GL_DEPTH_BUFFER_BIT); // Only clearing depth as it's a shadow pass
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    // Specify which textures from the G-buffer are to be written into during rendering
    std::vector<GBufferOutputType> attachments = {
        GBufferOutputType::DEPTH,
    };
    ShadowMapBuffer.setDrawBuffers(attachments);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // If want to render a background/procedural background, render that first
    m_terrain.drawProximity(player_pos[0], player_pos[2], 8, &m_progShadow);
    glDisable(GL_DEPTH_TEST);
}



void MyGL::renderTerrain() {
    glm::vec3 player_pos = m_player.mcr_position;
    glViewport(0,0,
               this->width() * this->devicePixelRatio(),
               this->height() * this->devicePixelRatio());


    glEnable(GL_DEPTH_TEST);
    // Specify which textures from the G-buffer are to be written into during rendering
    std::vector<GBufferOutputType> attachments = {
        GBufferOutputType::ALBEDO,
        GBufferOutputType::NORMAL,
        GBufferOutputType::POSITION_WORLD,
        GBufferOutputType::FOG,
        GBufferOutputType::SHADOW,
    };
    geometryBuffer.setDrawBuffers(attachments);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_texture.bind(TEXTURE_TEX_SLOT);
    m_progGbuffer.setUnifInt("u_Texture", TEXTURE_TEX_SLOT);

    m_textureNormal.bind(NORMAL_TEX_SLOT);
    m_progGbuffer.setUnifInt("u_NormalTexture", NORMAL_TEX_SLOT);

    ShadowMapBuffer.bindToTextureSlot(DEPTH_TEX_SLOT, GBufferOutputType::DEPTH);
    m_progGbuffer.setUnifInt("u_DepthTexture", DEPTH_TEX_SLOT);

    // If want to render a background/procedural background, render that first
    m_terrain.drawProximity(player_pos[0], player_pos[2], 8, &m_progGbuffer);
    glDisable(GL_DEPTH_TEST);
}

void MyGL::lightingPass() {
    std::vector<GBufferOutputType> attachments = {
        GBufferOutputType::SCENE
    };
    geometryBuffer.setDrawBuffers(attachments);

    glViewport(0,0,
               this->width() * this->devicePixelRatio(),
               this->height() * this->devicePixelRatio());
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    geometryBuffer.bindToTextureSlot(ALBEDO_TEX_SLOT, GBufferOutputType::ALBEDO);
    m_progLighting.setUnifInt("u_Albedo", ALBEDO_TEX_SLOT);

    geometryBuffer.bindToTextureSlot(NORMAL_TEX_SLOT, GBufferOutputType::NORMAL);
    m_progLighting.setUnifInt("u_NormalTexture", NORMAL_TEX_SLOT);

    geometryBuffer.bindToTextureSlot(GBUFFER_POSITION_WORLD_TEX_SLOT, GBufferOutputType::POSITION_WORLD);
    m_progLighting.setUnifInt("u_WorldPosTexture", GBUFFER_POSITION_WORLD_TEX_SLOT);

    geometryBuffer.bindToTextureSlot(FOG_TEX_SLOT, GBufferOutputType::FOG);
    m_progLighting.setUnifInt("u_FogTexture", FOG_TEX_SLOT);

    geometryBuffer.bindToTextureSlot(SHADOW_TEX_SLOT, GBufferOutputType::SHADOW);
    m_progLighting.setUnifInt("u_ShadowTexture", SHADOW_TEX_SLOT);

    geometryBuffer.bindToTextureSlot(SKY_TEX_SLOT, GBufferOutputType::SKY);
    m_progLighting.setUnifInt("u_SkyTexture", SKY_TEX_SLOT);

    m_progLighting.draw(quadDrawable);
}

void MyGL::postprocessingPass(ShaderProgram &s) {
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());
    glViewport(0, 0, width() * devicePixelRatio(), height() * devicePixelRatio());

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    s.useMe();
    geometryBuffer.bindToTextureSlot(SCENE_TEX_SLOT, GBufferOutputType::SCENE);
    s.setUnifInt("u_Albedo", SCENE_TEX_SLOT);

    s.draw(quadDrawable);
}


void MyGL::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        QApplication::quit();
        return;
    }

    bool shiftModifier = e->modifiers() & Qt::ShiftModifier;

    switch (e->key()) {
    case Qt::Key_W:
        m_inputs.wPressed = true;
        break;
    case Qt::Key_S:
        m_inputs.sPressed = true;
        break;
    case Qt::Key_A:
        m_inputs.aPressed = true;
        break;
    case Qt::Key_D:
        m_inputs.dPressed = true;
        break;
    case Qt::Key_Q:
        m_inputs.qPressed = true;
        break;
    case Qt::Key_E:
        m_inputs.ePressed = true;
        break;
    case Qt::Key_Space:
        m_inputs.spacePressed = true;
        break;
    case Qt::Key_F:
        m_player.flightMode = !m_player.flightMode;
        break;
    case Qt::Key_Shift:
        m_inputs.shiftPressed = shiftModifier;
        break;
    // Add other cases as needed
    default:
        QWidget::keyPressEvent(e);
    }
}

void MyGL::keyReleaseEvent(QKeyEvent *e) {
    bool shiftModifier = e->modifiers() & Qt::ShiftModifier;

    switch (e->key()) {
    case Qt::Key_W:
        m_inputs.wPressed = false;
        break;
    case Qt::Key_S:
        m_inputs.sPressed = false;
        break;
    case Qt::Key_A:
        m_inputs.aPressed = false;
        break;
    case Qt::Key_D:
        m_inputs.dPressed = false;
        break;
    case Qt::Key_Q:
        m_inputs.qPressed = false;
        break;
    case Qt::Key_E:
        m_inputs.ePressed = false;
        break;
    case Qt::Key_Space:
        m_inputs.spacePressed = false;
        break;
    case Qt::Key_Shift:
        m_inputs.shiftPressed = shiftModifier;
        break;
    // Add other cases as needed
    default:
        QWidget::keyPressEvent(e);
    }
}

void MyGL::mouseMoveEvent(QMouseEvent *e) {
    QPointF delta = e->pos() - lastMousePosition;
    m_inputs.mouseX -= delta.x();
    m_inputs.mouseY -= delta.y();

    if (!cursorCentered) {
        QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));
        lastMousePosition = QPoint(width() / 2, height() / 2);
        cursorCentered = true;
    } else {
        cursorCentered = false;
    }
}

void MyGL::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_player.removeBlock(m_terrain);
    }
    else if (e->button() == Qt::RightButton) {
        m_player.placeBlock(m_terrain);
    }
}

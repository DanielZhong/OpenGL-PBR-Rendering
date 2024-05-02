#include "mygl.h"
#include <glm_includes.h>

#include <iostream>
#include <QApplication>
#include <QKeyEvent>
#include <QDir>

#include <QFileDialog>
#include <stdexcept>
#include "texture.h"
#include <QJsonDocument>
#include <QJsonObject>

MyGL::MyGL(QWidget *parent)
    : OpenGLContext(parent),
      curr_time(QDateTime::currentMSecsSinceEpoch()),
      elapsed_time(0.f),
      m_timer(),
      m_geomSquare(this),
      m_geomMesh(this), m_textureAlbedo(this), m_textureMetallic(this),
      m_textureNormals(this), m_textureRoughness(this), m_textureAO(this),
      m_textureDisplacement(this),
      m_geomCube(this),
      m_hdrEnvMap(this),
      m_environmentCubemapFB(this, 1024, 1024, 1.f),
      m_diffuseIrradianceFB(this, 32, 32, 1.f),
      m_glossyIrradianceFB(this, 512, 512, 1.f),
      m_brdfLookupTexture(this),
      m_progPBR(this), m_progCubemapConversion(this),
      m_progCubemapDiffuseConvolution(this),
      m_progCubemapGlossyConvolution(this),
      m_progEnvMap(this),
      m_glCamera(), m_mousePosPrev(), m_albedo(1.f, 1.f, 1.f),
      m_cubemapsNotGenerated(true)
{
    setFocusPolicy(Qt::StrongFocus);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(tick()));
}

MyGL::~MyGL()
{
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    m_geomSquare.destroy();
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
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    // Set the color with which the screen is filled at the start of each render call.
    glClearColor(0.5, 0.5, 0.5, 1);

    printGLErrorLog();

    // Create a Vertex Attribute Object
    glGenVertexArrays(1, &vao);

    m_geomSquare.create();
    QString path = getCurrentPath();
    path.append("/assignment_package/objs/");
    m_geomMesh.LoadOBJ("sphere.obj", path);
    m_geomMesh.create();
    m_geomCube.create();

//    m_progPBR.create(":/glsl/pbr.vert.glsl", ":/glsl/pbr.frag.glsl");
//    m_progPBR.create(":/glsl/passthrough.vert.glsl", ":/glsl/sdf.frag.glsl");

    QString vertASCII = qTextFileRead(":/glsl/passthrough.vert.glsl");
    QString fragASCII = writeFullShaderFile();
    m_progPBR.create(vertASCII, fragASCII);

    m_progCubemapConversion.create(":/glsl/cubemap.vert.glsl", ":/glsl/cubemap_uv_conversion.frag.glsl");
    m_progCubemapDiffuseConvolution.create(":/glsl/cubemap.vert.glsl", ":/glsl/diffuseConvolution.frag.glsl");
    m_progCubemapGlossyConvolution.create(":/glsl/cubemap.vert.glsl", ":/glsl/glossyConvolution.frag.glsl");
    m_progEnvMap.create(":/glsl/envMap.vert.glsl", ":/glsl/envMap.frag.glsl");
    setupShaderHandles();

    path = getCurrentPath();
    path.append("/assignment_package/environment_maps/interior_atelier_soft_daylight.hdr");
    m_hdrEnvMap.create(path.toStdString().c_str(), false);

    m_environmentCubemapFB.create(true);
    m_diffuseIrradianceFB.create();
    m_glossyIrradianceFB.create(true);
    m_brdfLookupTexture.create(":/textures/brdfLUT.png", false);


    // We have to have a VAO bound in OpenGL 3.2 Core. But if we're not
    // using multiple VAOs, we can just bind one once.
    glBindVertexArray(vao);

    initPBRunifs();


    m_timer.start(16);
}

void MyGL::resizeGL(int w, int h)
{
    //This code sets the concatenated view and perspective projection matrices used for
    //our scene's camera view.
    m_glCamera = Camera(w, h);
    glm::mat4 viewproj = m_glCamera.getViewProj();

    // Upload the view-projection matrix to our shaders (i.e. onto the graphics card)

    m_progPBR.setUnifMat4("u_ViewProj", viewproj);
    m_progPBR.setUnifVec2("u_ScreenDims", glm::vec2(w, h));

    printGLErrorLog();
}

//This function is called by Qt any time your GL window is supposed to update
//For example, when the function update() is called, paintGL is called implicitly.
void MyGL::paintGL() {

    // Draw the environment map to the cube map frame buffer
    // if this is the very first draw cycle, or if we've loaded a new env map
    if(m_cubemapsNotGenerated) {
        m_cubemapsNotGenerated = false;
        // Convert the 2D HDR environment map texture to a cube map
        renderCubeMapToTexture();
        printGLErrorLog();
        // Generate mipmap levels for the environment map so that the
        // glossy reflection convolution has reduced fireflies
        m_environmentCubemapFB.generateMipMaps();
        printGLErrorLog();
        // Generate a cubemap of the diffuse irradiance (light reflected by the
        // Lambertian BRDF)
        renderConvolvedDiffuseCubeMapToTexture();
        printGLErrorLog();
        // Generate a cubemap of the varying levels of glossy irradiance (light
        // reflected by the Cook-Torrance
        renderConvolvedGlossyCubeMapToTexture();
        printGLErrorLog();
    }

    glViewport(0,0,this->width() * this->devicePixelRatio(),this->height() * this->devicePixelRatio());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderEnvironmentMap();

    m_progPBR.setUnifMat4("u_ViewProj", m_glCamera.getViewProj());
    m_progPBR.setUnifVec3("u_CamPos", m_glCamera.eye);
    m_progPBR.setUnifVec3("u_Forward", m_glCamera.look);
    m_progPBR.setUnifVec3("u_Right", m_glCamera.right);
    m_progPBR.setUnifVec3("u_Up", m_glCamera.up);

    //Send the geometry's transformation matrix to the shader
    m_progPBR.setUnifMat4("u_Model", glm::mat4(1.f));
    m_progPBR.setUnifMat4("u_ModelInvTr", glm::mat4(1.f));
    // Set up the diffuse irradiance map on the GPU so our surface shader can read it
    m_diffuseIrradianceFB.bindToTextureSlot(DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT);
    m_progPBR.setUnifInt("u_DiffuseIrradianceMap", DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT);
    // Set up the glossy irradiance map on the GPU so our surface shader can read it
    m_glossyIrradianceFB.bindToTextureSlot(GLOSSY_IRRADIANCE_CUBE_TEX_SLOT);
    m_progPBR.setUnifInt("u_GlossyIrradianceMap", GLOSSY_IRRADIANCE_CUBE_TEX_SLOT);
    // Also load our BRDF lookup texture for split-sum approximation
    m_brdfLookupTexture.bind(BRDF_LUT_TEX_SLOT);
    printGLErrorLog();
    m_progPBR.setUnifInt("u_BRDFLookupTexture", BRDF_LUT_TEX_SLOT);


    if(m_textureAlbedo.m_isCreated) {
        m_textureAlbedo.bind(ALBEDO_TEX_SLOT);
        m_progPBR.setUnifInt("u_AlbedoMap", ALBEDO_TEX_SLOT);
        m_progPBR.setUnifInt("u_UseAlbedoMap", 1);
    }
    else {
        m_progPBR.setUnifInt("u_UseAlbedoMap", 0);
    }
    if(m_textureMetallic.m_isCreated) {
        m_textureMetallic.bind(METALLIC_TEX_SLOT);
        m_progPBR.setUnifInt("u_MetallicMap", METALLIC_TEX_SLOT);
        m_progPBR.setUnifInt("u_UseMetallicMap", 1);
    }
    else {
        m_progPBR.setUnifInt("u_UseMetallicMap", 0);
    }
    if(m_textureRoughness.m_isCreated) {
        m_textureRoughness.bind(ROUGHNESS_TEX_SLOT);
        m_progPBR.setUnifInt("u_RoughnessMap", ROUGHNESS_TEX_SLOT);
        m_progPBR.setUnifInt("u_UseRoughnessMap", 1);
    }
    else {
        m_progPBR.setUnifInt("u_UseRoughnessMap", 0);
    }
    if(m_textureAO.m_isCreated) {
        m_textureAO.bind(AO_TEX_SLOT);
        m_progPBR.setUnifInt("u_AOMap", AO_TEX_SLOT);
        m_progPBR.setUnifInt("u_UseAOMap", 1);
    }
    else {
        m_progPBR.setUnifInt("u_UseAOMap", 0);
    }
    if(m_textureNormals.m_isCreated) {
        m_textureNormals.bind(NORMALS_TEX_SLOT);
        m_progPBR.setUnifInt("u_NormalMap", NORMALS_TEX_SLOT);
        m_progPBR.setUnifInt("u_UseNormalMap", 1);
    }
    else {
        m_progPBR.setUnifInt("u_UseNormalMap", 0);
    }

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_progPBR.draw(m_geomSquare);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);


}

void printMat(const glm::mat4 &m) {
    std::cout << glm::to_string(m[0]) << std::endl;
    std::cout << glm::to_string(m[1]) << std::endl;
    std::cout << glm::to_string(m[2]) << std::endl;
    std::cout << glm::to_string(m[3]) << std::endl;
}

void MyGL::renderCubeMapToTexture() {
    m_progCubemapConversion.useMe();
    // Set the cube map conversion shader's sampler to tex slot 0
    m_progCubemapConversion.setUnifInt("u_EquirectangularMap", ENV_MAP_FLAT_TEX_SLOT);
    // put the HDR environment map into texture slot 0
    m_hdrEnvMap.bind(ENV_MAP_FLAT_TEX_SLOT);
    // Set viewport dimensions equal to those of our cubemap faces
    glViewport(0, 0, m_environmentCubemapFB.width(), m_environmentCubemapFB.height());
    m_environmentCubemapFB.bindFrameBuffer();

    // Iterate over each face of the cube and apply the appropriate rotation
    // view matrix to the cube, then draw it w/ the HDR texture applied to it
    for(int i = 0; i < 6; ++i) {
        m_progCubemapConversion.setUnifMat4("u_ViewProj", views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               m_environmentCubemapFB.getCubemapHandle(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_progCubemapConversion.draw(m_geomCube);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());

}

void MyGL::renderConvolvedDiffuseCubeMapToTexture() {
    m_progCubemapDiffuseConvolution.useMe();
    // Set the cube map conversion shader's sampler to tex slot 0
    m_progCubemapDiffuseConvolution.setUnifInt("u_EnvironmentMap", ENV_MAP_CUBE_TEX_SLOT);
    // put the HDR environment map into texture slot 0
    m_environmentCubemapFB.bindToTextureSlot(ENV_MAP_CUBE_TEX_SLOT);
    // Set viewport dimensions equal to those of our cubemap faces
    glViewport(0, 0, m_diffuseIrradianceFB.width(), m_diffuseIrradianceFB.height());
    m_diffuseIrradianceFB.bindFrameBuffer();

    // Iterate over each face of the cube and apply the appropriate rotation
    // view matrix to the cube, then draw it w/ the HDR texture applied to it
    for(int i = 0; i < 6; ++i) {
        m_progCubemapDiffuseConvolution.setUnifMat4("u_ViewProj", views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               m_diffuseIrradianceFB.getCubemapHandle(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_progCubemapDiffuseConvolution.draw(m_geomCube);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());

}

void MyGL::renderConvolvedGlossyCubeMapToTexture() {
    m_progCubemapGlossyConvolution.useMe();
    // Set the cube map conversion shader's sampler to tex slot 0
    m_progCubemapGlossyConvolution.setUnifInt("u_EnvironmentMap", ENV_MAP_CUBE_TEX_SLOT);
    // put the HDR environment map into texture slot 0
    m_environmentCubemapFB.bindToTextureSlot(ENV_MAP_CUBE_TEX_SLOT);
    // Set viewport dimensions equal to those of our cubemap faces
    glViewport(0, 0, m_glossyIrradianceFB.width(), m_glossyIrradianceFB.height());
    m_glossyIrradianceFB.bindFrameBuffer();

    const unsigned int maxMipLevels = 5;
    for(unsigned int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
        // Resize our frame buffer according to our mip level
        unsigned int mipWidth  = static_cast<unsigned int>(m_glossyIrradianceFB.width() * std::pow(0.5, mipLevel));
        unsigned int mipHeight = static_cast<unsigned int>(m_glossyIrradianceFB.height() * std::pow(0.5, mipLevel));
        m_glossyIrradianceFB.bindRenderBuffer(mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = static_cast<float>(mipLevel) / static_cast<float>(maxMipLevels - 1);
        m_progCubemapGlossyConvolution.setUnifFloat("u_Roughness", roughness);

        // Iterate over each face of the cube and apply the appropriate rotation
        // view matrix to the cube, then draw it w/ the HDR texture applied to it
        for(int i = 0; i < 6; ++i) {
            m_progCubemapGlossyConvolution.setUnifMat4("u_ViewProj", views[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   m_glossyIrradianceFB.getCubemapHandle(), mipLevel);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_progCubemapGlossyConvolution.draw(m_geomCube);
        }
    }


    glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
}

void MyGL::renderEnvironmentMap() {
    // Bind cubemap texture to appropriate tex slot
    m_environmentCubemapFB.bindToTextureSlot(ENV_MAP_CUBE_TEX_SLOT);
//    m_diffuseIrradianceFB.bindToTextureSlot(DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT);
//    m_glossyIrradianceFB.bindToTextureSlot(GLOSSY_IRRADIANCE_CUBE_TEX_SLOT);
    // Set the environment map shader's cubemap sampler to the same tex slot
    m_progEnvMap.setUnifInt("u_EnvironmentMap", ENV_MAP_CUBE_TEX_SLOT);
    m_progEnvMap.setUnifMat4("u_ViewProj", m_glCamera.getViewProj_OrientOnly());
    m_progEnvMap.draw(m_geomCube);
}

void MyGL::setupShaderHandles() {
    m_progPBR.addUniform("u_Time");
    m_progPBR.addUniform("u_Model");
    m_progPBR.addUniform("u_ModelInvTr");
    m_progPBR.addUniform("u_ViewProj");
    m_progPBR.addUniform("u_CamPos");
    m_progPBR.addUniform("u_Forward");
    m_progPBR.addUniform("u_Right");
    m_progPBR.addUniform("u_Up");
    m_progPBR.addUniform("u_ScreenDims");
    m_progPBR.addUniform("u_Albedo");
    m_progPBR.addUniform("u_Metallic");
    m_progPBR.addUniform("u_Roughness");
    m_progPBR.addUniform("u_AmbientOcclusion");
    m_progPBR.addUniform("u_AlbedoMap");
    m_progPBR.addUniform("u_MetallicMap");
    m_progPBR.addUniform("u_RoughnessMap");
    m_progPBR.addUniform("u_AOMap");
    m_progPBR.addUniform("u_NormalMap");
    m_progPBR.addUniform("u_UseAlbedoMap");
    m_progPBR.addUniform("u_UseMetallicMap");
    m_progPBR.addUniform("u_UseRoughnessMap");
    m_progPBR.addUniform("u_UseAOMap");
    m_progPBR.addUniform("u_UseNormalMap");
    m_progPBR.addUniform("u_DiffuseIrradianceMap");
    m_progPBR.addUniform("u_GlossyIrradianceMap");
    m_progPBR.addUniform("u_BRDFLookupTexture");

    m_progCubemapConversion.addUniform("u_EquirectangularMap");
    m_progCubemapConversion.addUniform("u_ViewProj");

    m_progEnvMap.addUniform("u_EnvironmentMap");
    m_progEnvMap.addUniform("u_ViewProj");

    m_progCubemapDiffuseConvolution.addUniform("u_EnvironmentMap");
    m_progCubemapDiffuseConvolution.addUniform("u_ViewProj");

    m_progCubemapGlossyConvolution.addUniform("u_EnvironmentMap");
    m_progCubemapGlossyConvolution.addUniform("u_Roughness");
    m_progCubemapGlossyConvolution.addUniform("u_ViewProj");
}

void MyGL::initPBRunifs() {
    m_progPBR.setUnifVec3("u_Albedo", m_albedo);
    m_progPBR.setUnifFloat("u_AmbientOcclusion", 1.f);
    m_progPBR.setUnifFloat("u_Metallic", 0.5f);
    m_progPBR.setUnifFloat("u_Roughness", 0.5f);
    m_progPBR.setUnifFloat("u_DisplacementMagnitude", 0.2f);
}

void MyGL::slot_setRed(int r) {
    m_albedo.r = r / 100.f;
    m_progPBR.setUnifVec3("u_Albedo", m_albedo);
    update();
}
void MyGL::slot_setGreen(int g) {
    m_albedo.g = g / 100.f;
    m_progPBR.setUnifVec3("u_Albedo", m_albedo);
    update();
}
void MyGL::slot_setBlue(int b) {
    m_albedo.b = b / 100.f;
    m_progPBR.setUnifVec3("u_Albedo", m_albedo);
    update();
}

void MyGL::slot_setMetallic(int m) {
    m_progPBR.setUnifFloat("u_Metallic", m / 100.f);
    update();
}
void MyGL::slot_setRoughness(int r) {
    m_progPBR.setUnifFloat("u_Roughness", r / 100.f);
    update();
}
void MyGL::slot_setAO(int a) {
    m_progPBR.setUnifFloat("u_AmbientOcclusion", a / 100.f);
    update();
}
void MyGL::slot_setDisplacement(double d) {
    m_progPBR.setUnifFloat("u_DisplacementMagnitude", d);
    update();
}

void MyGL::slot_loadEnvMap() {
    QString path = getCurrentPath();
    path.append("/assignment_package/environment_maps/");
    QString filepath = QFileDialog::getOpenFileName(
                        0, QString("Load Environment Map"),
                        path, tr("*.hdr"));
    Texture2DHDR tex(this);
    try {
        tex.create(filepath.toStdString().c_str(), false);
    }
    catch(std::exception &e) {
        std::cout << "Error: Failed to load HDR image" << std::endl;
        return;
    }
    this->m_hdrEnvMap.destroy();
    this->m_hdrEnvMap = tex;
    this->m_cubemapsNotGenerated = true;
    update();
}

void MyGL::slot_loadScene() {
    QString path = getCurrentPath();
    path.append("/assignment_package/models/");
    QString filepath = QFileDialog::getOpenFileName(
                        0, QString("Load Environment Map"),
                        path, tr("*.json"));
    QFile file(filepath);
    if(file.open(QIODevice::ReadOnly)) {
        QByteArray rawData = file.readAll();
        // Parse document
        QJsonDocument doc(QJsonDocument::fromJson(rawData));
        // Get JSON object
        QJsonObject json = doc.object();

        QString obj = json["obj"].toString();
        QString albedo = json["albedo"].toString();
        QString metallic = json["metallic"].toString();
        QString normal = json["normal"].toString();
        QString roughness = json["roughness"].toString();
        QString ambientOcclusion = json["ambientOcclusion"].toString();
        QString displacement = json["displacement"].toString();

        Mesh mesh(this);
        mesh.LoadOBJ(obj, path);


        if(m_textureAlbedo.m_isCreated) {m_textureAlbedo.destroy();}
        if(m_textureMetallic.m_isCreated) {m_textureMetallic.destroy();}
        if(m_textureNormals.m_isCreated) {m_textureNormals.destroy();}
        if(m_textureRoughness.m_isCreated) {m_textureRoughness.destroy();}
        if(m_textureAO.m_isCreated) {m_textureAO.destroy();}
        if(m_textureDisplacement.m_isCreated) {m_textureDisplacement.destroy();}

        if(albedo != "") {
            m_textureAlbedo.create((path + albedo).toStdString().c_str(), true);
        }
        if(metallic != "") {
            m_textureMetallic.create((path + metallic).toStdString().c_str(), true);
        }
        if(normal != "") {
            m_textureNormals.create((path + normal).toStdString().c_str(), true);
        }
        if(roughness != "") {
            m_textureRoughness.create((path + roughness).toStdString().c_str(), true);
        }
        if(ambientOcclusion != "") {
            m_textureAO.create((path + ambientOcclusion).toStdString().c_str(), true);
        }
        if(displacement != "") {
            m_textureDisplacement.create((path + displacement).toStdString().c_str(), true);
        }


        m_geomMesh.destroy();
        m_geomMesh = mesh;
        m_geomMesh.create();
    }
    update();
}

void MyGL::slot_revertToSphere() {
    m_geomMesh.destroy();
    QString path = getCurrentPath();
    path.append("/assignment_package/objs/");
    m_geomMesh.LoadOBJ("sphere.obj", path);
    m_geomMesh.create();

    if(m_textureAlbedo.m_isCreated) {
        m_textureAlbedo.destroy();
    }
    if(m_textureMetallic.m_isCreated) {
        m_textureMetallic.destroy();
    }
    if(m_textureNormals.m_isCreated) {
        m_textureNormals.destroy();
    }
    if(m_textureRoughness.m_isCreated) {
        m_textureRoughness.destroy();
    }
    if(m_textureAO.m_isCreated) {
        m_textureAO.destroy();
    }    
    if(m_textureDisplacement.m_isCreated) {
        m_textureDisplacement.destroy();
    }
    update();
}

void MyGL::slot_loadOBJ() {
    QString path = getCurrentPath();
    path.append("/assignment_package/models/");
    QString filepath = QFileDialog::getOpenFileName(
                        0, QString("Load OBJ File"),
                        path, tr("*.obj"));
    m_geomMesh.destroy();
    m_geomMesh.LoadOBJ(filepath, "");
    m_geomMesh.create();

    if(m_textureAlbedo.m_isCreated) {
        m_textureAlbedo.destroy();
    }
    if(m_textureMetallic.m_isCreated) {
        m_textureMetallic.destroy();
    }
    if(m_textureNormals.m_isCreated) {
        m_textureNormals.destroy();
    }
    if(m_textureRoughness.m_isCreated) {
        m_textureRoughness.destroy();
    }
    if(m_textureAO.m_isCreated) {
        m_textureAO.destroy();
    }
    if(m_textureDisplacement.m_isCreated) {
        m_textureDisplacement.destroy();
    }
    update();
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

void MyGL::tick() {
    int64_t prev_time = curr_time;
    curr_time = QDateTime::currentMSecsSinceEpoch();
    elapsed_time += (curr_time - prev_time) * 0.001f;
    m_progPBR.setUnifFloat("u_Time", elapsed_time);
    update();
}

QString MyGL::writeFullShaderFile() const {
    std::vector<const char*> fragfile_sections = {
        ":/glsl/sdf.pbr.glsl",
        ":/glsl/sdf.frag.glsl"
    };

    QString qFragSource = qTextFileRead(":/glsl/sdf.defines.glsl");
    qFragSource.chop(1); // Must remove the \0 at the end of the previous section

    int lineCount = 0;
    for(auto &c : fragfile_sections) {
        QString section = qTextFileRead(c);
//        std::cout << "First line number in " << c << ": " << lineCount << std::endl;
        lineCount += section.count("\n");
        qFragSource.chop(1); // Must remove the \0 at the end of the previous section
        qFragSource = qFragSource + "\n" + section;
    }

    // Write the entire frag source to a single file so that
    // we the programmer can read & debug our code in one file
    QString path = getCurrentPath();
    QFile entireShader(path + "/assignment_package/glsl/sdf.all.glsl");
    if (entireShader.open(QIODevice::ReadWrite)) {
        entireShader.resize(0); // Clear out the old file contents
        QTextStream stream(&entireShader);
        stream << qFragSource << Qt::endl;
    }
    entireShader.close();
    return qFragSource;
}

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
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>

MyGL::MyGL(QWidget *parent)
    : QOpenGLWidget(parent),
    vao(0),
    progMeshToGBuffer(&glContext),
    progComputeScreenSpaceReflection(&glContext),
    progBlurScreenReflection(&glContext),
    progFinalOutput(&glContext),
    progCubemapConversion(&glContext),
    progCubemapDiffuseConvolution(&glContext),
    progCubemapGlossyConvolution(&glContext),
    progEnvMap(&glContext),
    cubemapsNotGenerated(true),
    meshDrawable(&glContext, (getCurrentPath() + "/models/reflection_scene.obj").toStdString()),
    quadDrawable(&glContext), cubeDrawable(&glContext),
    textureAlbedo(&glContext, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    textureMetallic(&glContext, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    textureNormals(&glContext, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    textureRoughness(&glContext, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    textureAO(&glContext, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    textureDisplacement(&glContext, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    textureHDREnvMap(&glContext, GL_RGB16F, GL_RGB, GL_FLOAT),
    textureBrdfLookup(&glContext, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE),
    environmentCubemapFB(&glContext, 1024, 1024, 1.f),
    diffuseIrradianceFB(&glContext, 32, 32, 1.f),
    glossyIrradianceFB(&glContext, 512, 512, 1.f),
    geometryBuffer(&glContext,
                   this->width(), this->height(),
                   this->devicePixelRatio()),
    gaussianKernel(&glContext, GL_R32F, GL_RED, GL_FLOAT),
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

std::vector<GLfloat> MyGL::computeGaussianKernel(int radius) {
    float sigma = glm::max(radius * 0.5f, 1.f);
    float sigma2 = sigma * sigma;
    int kernelWidth = radius * 2 + 1;
    std::vector<GLfloat> kernel =
        std::vector<GLfloat>(kernelWidth * kernelWidth, 0.f);
    // TODO: Populate the kernel vector with the values of a
    //       Gaussian kernel using the formula portrayed
    //       here: https://rastergrid.com/wp-content/uploads/2019/03/gaussian_function_2D.png
    //       Note that x and y are relative to the center pixel, and thus
    //       can be negative.

    //       To map the (x,y) of a kernel cell to a 1D index in the vector,
    //       use x + y * kernelWidth. Remember to remap (x,y) from the range
    //       [-radius, radius] to [0, 2 * radius] first.

    //       Also remember to normalize your Gaussian kernel by dividing
    //       every element by the sum of all elements.
    float sum = 0.0f;

    // Center of the kernel
    int center = radius;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            // Calculate the Gaussian weight
            float weight = exp(-(x * x + y * y) / (2 * sigma2)) / (2 * M_PI * sigma2);

            // Store the weight at the correct index
            kernel[(x + center) + (y + center) * kernelWidth] = weight;

            // Add to the sum for normalization
            sum += weight;
        }
    }

    // Normalize the kernel to make the sum of all elements equal to 1
    for (size_t i = 0; i < kernel.size(); i++) {
        kernel[i] /= sum;
    }

    return kernel;
}

void MyGL::tick() {

    ++currTime;

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

    progMeshToGBuffer.createAndCompileShaderProgram("g-buffer.vert.glsl",
                                                    "g-buffer.frag.glsl");
    printGLErrorLog();

    progComputeScreenSpaceReflection.createAndCompileShaderProgram(
        "passthrough.vert.glsl",
        "screenSpaceReflection.frag.glsl");

    // TODO: create shader program that blurs SSR
    progBlurScreenReflection.createAndCompileShaderProgram(
        "passthrough.vert.glsl",
        "blur.frag.glsl");

    progFinalOutput.createAndCompileShaderProgram("passthrough.vert.glsl",
                                                  "combine.frag.glsl");

    // in original base code, this was surface shader
    //    progPBR.createAndCompileShaderProgram(":/glsl/pbr.vert.glsl",
    //                                            ":/glsl/pbr.frag.glsl");
    progCubemapConversion.createAndCompileShaderProgram(
        "cubemap.vert.glsl",
        "cubemap_uv_conversion.frag.glsl");
    progCubemapDiffuseConvolution.createAndCompileShaderProgram(
        "cubemap.vert.glsl",
        "diffuseConvolution.frag.glsl");
    progCubemapGlossyConvolution.createAndCompileShaderProgram(
        "cubemap.vert.glsl",
        "glossyConvolution.frag.glsl");
    progEnvMap.createAndCompileShaderProgram(
        "envMap.vert.glsl",
        "envMap.frag.glsl");

    meshDrawable.initializeAndBufferGeometryData();
    printGLErrorLog();
    quadDrawable.initializeAndBufferGeometryData();
    printGLErrorLog();
    cubeDrawable.initializeAndBufferGeometryData();
    printGLErrorLog();

    setupGBuffer();

    QString localPath = getCurrentPath();
    QString texturePath = localPath + "/models/refl_scene_albedo.png";
    textureAlbedo.create(texturePath.toStdString().c_str(), false, false, false, false);
    texturePath = localPath + "/models/refl_scene_metallic.png";
    textureMetallic.create(texturePath.toStdString().c_str(), false, false, false, false);
    texturePath = localPath + "/models/refl_scene_roughness.png";
    textureRoughness.create(texturePath.toStdString().c_str(), false, false, false, false);

    QString path = getCurrentPath();
    path.append("/environment_maps/interior_atelier_soft_daylight.hdr");
    textureHDREnvMap.create(path.toStdString().c_str(), true, true, false, false);

    environmentCubemapFB.create(true);
    diffuseIrradianceFB.create();
    glossyIrradianceFB.create(true);

    path = getCurrentPath();
    path.append("/textures/brdfLUT.png");
    textureBrdfLookup.create(path.toStdString().c_str(), false, false, false, false);

    // Kernel is 41x41 (radius of 20px from center)
    std::vector<GLfloat> kernel = computeGaussianKernel(20);
    gaussianKernel.create(nullptr, false, false, false, false);
    gaussianKernel.bufferPixelData(41, 41, kernel.data());

    // Enable depth sorting in the GL pipeline
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glClearColor(0.f, 0.f, 0.f, 0.f);
}

void MyGL::setupGBuffer() {
    printGLErrorLog();
    geometryBuffer.create();
    printGLErrorLog();
    geometryBuffer.addTexture(GBufferOutputType::POSITION_WORLD);
    geometryBuffer.addTexture(GBufferOutputType::NORMAL);
    geometryBuffer.addTexture(GBufferOutputType::ALBEDO);
    geometryBuffer.addTexture(GBufferOutputType::METAL_ROUGH_MASK);
    geometryBuffer.addTexture(GBufferOutputType::PBR);
    geometryBuffer.addTexture(GBufferOutputType::SSR);
    geometryBuffer.addTexture(GBufferOutputType::SSR_BLUR0);
    geometryBuffer.addTexture(GBufferOutputType::SSR_BLUR1);
    geometryBuffer.addTexture(GBufferOutputType::SSR_BLUR2);
    geometryBuffer.addTexture(GBufferOutputType::SSR_BLUR3);
    printGLErrorLog();
}

void MyGL::resizeGL(int w, int h) {
    geometryBuffer.resize(w, h, this->devicePixelRatio());
    geometryBuffer.destroy();
    geometryBuffer.create();
    setupGBuffer();

    camera.recomputeAspectRatio(w, h);
}

void MyGL::renderSceneToGBuffer() {

    // 1. Set the geometry buffer's draw buffers
    //    so that it writes to:
    //    - World position
    //    - Surface normal
    //    - Albedo
    //    - Metallic + roughness
    //    - (Optionally) The environment-mapped PBR reflection (hw07's result)
    //    Refer to the FrameBuffer class to see
    //    what member function does this.

    // 2. Set progMeshToGBuffer's model-view-projection matrix
    //    and camera position uniforms

    // 3. Bind each of the material property texture maps
    //    (albedo, metallic, etc.) to their own texture slots
    //    and then set progMeshToGBuffer's sampler2Ds to read
    //    from the appropriate texture slots

    // 4. If choosing to output PBR reflection into G-buffer,
    //    enable the #if 0 - #endif section below

    // 5. (Already given to you) Bind the G-buffer and render to it3


    // We set up geometryBuffer from setupGBuffer()
    geometryBuffer.bindFrameBuffer();
    glViewport(0,0,
               this->width() * this->devicePixelRatio(),
               this->height() * this->devicePixelRatio());


    glEnable(GL_DEPTH_TEST);
    // Specify which textures from the G-buffer are to be written into during rendering
    std::vector<GBufferOutputType> attachments = {
        GBufferOutputType::POSITION_WORLD,
        GBufferOutputType::NORMAL,
        GBufferOutputType::ALBEDO,
        GBufferOutputType::METAL_ROUGH_MASK,
        GBufferOutputType::PBR  // Optional for PBR data
    };
    geometryBuffer.setDrawBuffers(attachments);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind and configure the shader for rendering to the G-buffer
    progMeshToGBuffer.useProgram();
    glm::mat4 projection = camera.getProj();
    glm::mat4 view = camera.getView();
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix for model transformation
    glm::mat4 mvp = projection * view * model;
    progMeshToGBuffer.setUnifMat4("u_MVP", mvp);
    progMeshToGBuffer.setUnifVec3("u_CamPos", camera.eye);

    // Bind texture maps
    textureAlbedo.bind(ALBEDO_TEX_SLOT);
    progMeshToGBuffer.setUnifInt("u_AlbedoTexture", ALBEDO_TEX_SLOT);

    textureMetallic.bind(METALLIC_TEX_SLOT);
    progMeshToGBuffer.setUnifInt("u_MetallicTexture", METALLIC_TEX_SLOT);

    textureRoughness.bind(ROUGHNESS_TEX_SLOT);
    progMeshToGBuffer.setUnifInt("u_RoughnessTexture", ROUGHNESS_TEX_SLOT);

// textureNormals.bind(GBUFFER_NORMAL_TEX_SLOT);
// progMeshToGBuffer.setUnifInt("u_NormalMap", GBUFFER_NORMAL_TEX_SLOT);

#if 1
    // Set up the diffuse irradiance map on the GPU so our surface shader can read it
    diffuseIrradianceFB.bindToTextureSlot(DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    progMeshToGBuffer.setUnifInt("u_DiffuseIrradianceMap", DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT);
    // Set up the glossy irradiance map on the GPU so our surface shader can read it
    glossyIrradianceFB.bindToTextureSlot(GLOSSY_IRRADIANCE_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    progMeshToGBuffer.setUnifInt("u_GlossyIrradianceMap", GLOSSY_IRRADIANCE_CUBE_TEX_SLOT);
    // Also load our BRDF lookup texture for split-sum approximation
    textureBrdfLookup.bind(BRDF_LUT_TEX_SLOT);
    progMeshToGBuffer.setUnifInt("u_BRDFLookupTexture", BRDF_LUT_TEX_SLOT);
#endif

    progMeshToGBuffer.draw(meshDrawable);
    glDisable(GL_DEPTH_TEST);

}

void MyGL::intermediateScreenSpaceReflToGBuffer() {
    // Don't need to bind g-buffer as it is already bound
    // from first render pass

    // Set the singular output channel of the shader to be written
    // to the frame buffer's 0th color attachment
    geometryBuffer.setDrawBuffers({GBufferOutputType::SSR});
    // Clear just the 0th color attachment to 0,0,0,0
    static const float transparent[] = {0,0,0,0};
    glContext.glClearBufferfv(GL_COLOR, 0, transparent);

    // Set the G-buffer textures to the appropriate texture slots
    geometryBuffer.bindToTextureSlot(GBUFFER_POSITION_WORLD_TEX_SLOT, GBufferOutputType::POSITION_WORLD);
    geometryBuffer.bindToTextureSlot(GBUFFER_NORMAL_TEX_SLOT, GBufferOutputType::NORMAL);
    geometryBuffer.bindToTextureSlot(GBUFFER_ALBEDO_TEX_SLOT, GBufferOutputType::ALBEDO);
    geometryBuffer.bindToTextureSlot(GBUFFER_METAL_ROUGH_MASK_TEX_SLOT, GBufferOutputType::METAL_ROUGH_MASK);
    geometryBuffer.bindToTextureSlot(GBUFFER_PBR_TEX_SLOT, GBufferOutputType::PBR);

    // Then set the various sampler2Ds in the shader to read from the
    // texture slots used above
    progComputeScreenSpaceReflection.setUnifInt("u_TexPositionWorld", GBUFFER_POSITION_WORLD_TEX_SLOT);
    progComputeScreenSpaceReflection.setUnifInt("u_TexNormal", GBUFFER_NORMAL_TEX_SLOT);
    progComputeScreenSpaceReflection.setUnifInt("u_TexAlbedo", GBUFFER_ALBEDO_TEX_SLOT);
    progComputeScreenSpaceReflection.setUnifInt("u_TexMetalRoughMask", GBUFFER_METAL_ROUGH_MASK_TEX_SLOT);
    progComputeScreenSpaceReflection.setUnifInt("u_TexPBR", GBUFFER_PBR_TEX_SLOT);

    // Set the other various uniform values the shader needs
    progComputeScreenSpaceReflection.setUnifVec3("u_CamPos", camera.eye);
    progComputeScreenSpaceReflection.setUnifVec3("u_CamForward", camera.forward);
    progComputeScreenSpaceReflection.setUnifMat4("u_View", camera.getView());
    progComputeScreenSpaceReflection.setUnifMat4("u_Proj", camera.getProj());

    // Set up the diffuse irradiance map on the GPU so our surface shader can read it
    // diffuseIrradianceFB.bindToTextureSlot(DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    // progComputeScreenSpaceReflection.setUnifInt("u_DiffuseIrradianceMap", DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT);
    // // Set up the glossy irradiance map on the GPU so our surface shader can read it
    // glossyIrradianceFB.bindToTextureSlot(GLOSSY_IRRADIANCE_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    // progComputeScreenSpaceReflection.setUnifInt("u_GlossyIrradianceMap", GLOSSY_IRRADIANCE_CUBE_TEX_SLOT);
    // // Also load our BRDF lookup texture for split-sum approximation
    // textureBrdfLookup.bind(BRDF_LUT_TEX_SLOT);
    // progComputeScreenSpaceReflection.setUnifInt("u_BRDFLookupTexture", BRDF_LUT_TEX_SLOT);


    glContext.glEnable(GL_BLEND);
    glContext.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Draw the screen-spanning quad with the shader that computes
    // the screen-space reflection of the scene
    progComputeScreenSpaceReflection.draw(quadDrawable);
    glContext.glDisable(GL_BLEND);
}

void MyGL::intermediateBlurSSR() {
    GBufferOutputType inputs[] = {
        GBufferOutputType::SSR,
        GBufferOutputType::SSR_BLUR0,
        GBufferOutputType::SSR_BLUR1,
        GBufferOutputType::SSR_BLUR2,
        GBufferOutputType::SSR_BLUR3
    };
    static const float transparent[] = {0,0,0,0};
    for(int i = 1; i < 5; ++i) {
        geometryBuffer.setDrawBuffers({inputs[i]});
        glContext.glClearBufferfv(GL_COLOR, 0, transparent);
        gaussianKernel.bind(30);
        progBlurScreenReflection.setUnifInt("u_Kernel", 30);
        progBlurScreenReflection.setUnifInt("u_KernelRadius", 20);
        geometryBuffer.bindToTextureSlot(31, inputs[i-1]);
        progBlurScreenReflection.setUnifInt("u_TextureSSR", 31);
        progBlurScreenReflection.draw(quadDrawable);
    }
}

void MyGL::combineGBufferIntoImage() {
    glContext.glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
    glViewport(0,0,this->width() * this->devicePixelRatio(),
               this->height() * this->devicePixelRatio());


    // Place the texture that stores the image of the 3D render
    // into texture slot 0
    geometryBuffer.bindToTextureSlot(GBUFFER_POSITION_WORLD_TEX_SLOT, GBufferOutputType::POSITION_WORLD);
    geometryBuffer.bindToTextureSlot(GBUFFER_NORMAL_TEX_SLOT, GBufferOutputType::NORMAL);
    geometryBuffer.bindToTextureSlot(GBUFFER_ALBEDO_TEX_SLOT, GBufferOutputType::ALBEDO);
    geometryBuffer.bindToTextureSlot(GBUFFER_METAL_ROUGH_MASK_TEX_SLOT, GBufferOutputType::METAL_ROUGH_MASK);
    geometryBuffer.bindToTextureSlot(GBUFFER_PBR_TEX_SLOT, GBufferOutputType::PBR);

    geometryBuffer.bindToTextureSlot(GBUFFER_SSR_SPECULAR_TEX_SLOT, GBufferOutputType::SSR);
    geometryBuffer.bindToTextureSlot(GBUFFER_SSR_GLOSSY1_TEX_SLOT, GBufferOutputType::SSR_BLUR0);
    geometryBuffer.bindToTextureSlot(GBUFFER_SSR_GLOSSY2_TEX_SLOT, GBufferOutputType::SSR_BLUR1);
    geometryBuffer.bindToTextureSlot(GBUFFER_SSR_GLOSSY3_TEX_SLOT, GBufferOutputType::SSR_BLUR2);
    geometryBuffer.bindToTextureSlot(GBUFFER_SSR_GLOSSY4_TEX_SLOT, GBufferOutputType::SSR_BLUR3);
    // Set the sampler2D in the post-process shader to
    // read from the texture slot that we set the
    // texture into
    progFinalOutput.setUnifInt("u_TexPositionWorld", GBUFFER_POSITION_WORLD_TEX_SLOT);
    progFinalOutput.setUnifInt("u_TexNormal", GBUFFER_NORMAL_TEX_SLOT);
    progFinalOutput.setUnifInt("u_TexAlbedo", GBUFFER_ALBEDO_TEX_SLOT);
    progFinalOutput.setUnifInt("u_TexMetalRoughMask", GBUFFER_METAL_ROUGH_MASK_TEX_SLOT);
    GLint handles[] = {GBUFFER_SSR_SPECULAR_TEX_SLOT,
                       GBUFFER_SSR_GLOSSY1_TEX_SLOT,
                       GBUFFER_SSR_GLOSSY2_TEX_SLOT,
                       GBUFFER_SSR_GLOSSY3_TEX_SLOT,
                       GBUFFER_SSR_GLOSSY4_TEX_SLOT};
    progFinalOutput.setUnifIntArray("u_TexSSR", 5, handles);

    progFinalOutput.setUnifVec3("u_CamPos", camera.eye);

    // Set up the diffuse irradiance map on the GPU so our surface shader can read it
    diffuseIrradianceFB.bindToTextureSlot(DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    progFinalOutput.setUnifInt("u_DiffuseIrradianceMap", DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT);
    // Set up the glossy irradiance map on the GPU so our surface shader can read it
    glossyIrradianceFB.bindToTextureSlot(GLOSSY_IRRADIANCE_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    progFinalOutput.setUnifInt("u_GlossyIrradianceMap", GLOSSY_IRRADIANCE_CUBE_TEX_SLOT);
    // Also load our BRDF lookup texture for split-sum approximation
    textureBrdfLookup.bind(BRDF_LUT_TEX_SLOT);
    printGLErrorLog();
    progFinalOutput.setUnifInt("u_BRDFLookupTexture", BRDF_LUT_TEX_SLOT);


    glContext.glEnable(GL_BLEND);
    glContext.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // draw quad with post shader
    progFinalOutput.draw(quadDrawable);
    glContext.glDisable(GL_BLEND);
}

void MyGL::paintGL() {
    // Draw the environment map to the cube map frame buffer
    // if this is the very first draw cycle, or if we've loaded a new env map
    if(cubemapsNotGenerated) {
        cubemapsNotGenerated = false;
        // Convert the 2D HDR environment map texture to a cube map
        renderCubeMapToTexture();
        printGLErrorLog();
        // Generate mipmap levels for the environment map so that the
        // glossy reflection convolution has reduced fireflies
        environmentCubemapFB.generateMipMaps();
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

    renderSceneToGBuffer();
    intermediateScreenSpaceReflToGBuffer();
    intermediateBlurSSR();
    combineGBufferIntoImage();
}

void MyGL::renderCubeMapToTexture() {
    progCubemapConversion.useProgram();
    // Set the cube map conversion shader's sampler to tex slot 0
    progCubemapConversion.setUnifInt("u_EquirectangularMap", ENV_MAP_FLAT_TEX_SLOT);
    // put the HDR environment map into texture slot 0
    textureHDREnvMap.bind(ENV_MAP_FLAT_TEX_SLOT);
    // Set viewport dimensions equal to those of our cubemap faces
    glViewport(0, 0, environmentCubemapFB.width(), environmentCubemapFB.height());
    environmentCubemapFB.bindFrameBuffer();

    // Iterate over each face of the cube and apply the appropriate rotation
    // view matrix to the cube, then draw it w/ the HDR texture applied to it
    for(int i = 0; i < 6; ++i) {
        progCubemapConversion.setUnifMat4("u_ViewProj", views[i]);
        glContext.glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            environmentCubemapFB.getCubemapHandle(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        progCubemapConversion.draw(cubeDrawable);
    }
    glContext.glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());

}

void MyGL::renderConvolvedDiffuseCubeMapToTexture() {
    progCubemapDiffuseConvolution.useProgram();
    // Set the cube map conversion shader's sampler to tex slot 0
    progCubemapDiffuseConvolution.setUnifInt("u_EnvironmentMap", ENV_MAP_CUBE_TEX_SLOT);
    // put the HDR environment map into texture slot 0
    environmentCubemapFB.bindToTextureSlot(ENV_MAP_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    // Set viewport dimensions equal to those of our cubemap faces
    glViewport(0, 0, diffuseIrradianceFB.width(), diffuseIrradianceFB.height());
    diffuseIrradianceFB.bindFrameBuffer();

    // Iterate over each face of the cube and apply the appropriate rotation
    // view matrix to the cube, then draw it w/ the HDR texture applied to it
    for(int i = 0; i < 6; ++i) {
        progCubemapDiffuseConvolution.setUnifMat4("u_ViewProj", views[i]);
        glContext.glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            diffuseIrradianceFB.getCubemapHandle(), 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        progCubemapDiffuseConvolution.draw(cubeDrawable);
    }
    glContext.glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());

}

void MyGL::renderConvolvedGlossyCubeMapToTexture() {
    progCubemapGlossyConvolution.useProgram();
    // Set the cube map conversion shader's sampler to tex slot 0
    progCubemapGlossyConvolution.setUnifInt("u_EnvironmentMap", ENV_MAP_CUBE_TEX_SLOT);
    // put the HDR environment map into texture slot 0
    environmentCubemapFB.bindToTextureSlot(ENV_MAP_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    // Set viewport dimensions equal to those of our cubemap faces
    glViewport(0, 0, glossyIrradianceFB.width(), glossyIrradianceFB.height());
    glossyIrradianceFB.bindFrameBuffer();

    const unsigned int maxMipLevels = 5;
    for(unsigned int mipLevel = 0; mipLevel < maxMipLevels; ++mipLevel) {
        // Resize our frame buffer according to our mip level
        unsigned int mipWidth  = static_cast<unsigned int>(glossyIrradianceFB.width() * std::pow(0.5, mipLevel));
        unsigned int mipHeight = static_cast<unsigned int>(glossyIrradianceFB.height() * std::pow(0.5, mipLevel));
        glossyIrradianceFB.bindRenderBuffer(mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = static_cast<float>(mipLevel) / static_cast<float>(maxMipLevels - 1);
        progCubemapGlossyConvolution.setUnifFloat("u_Roughness", roughness);

        // Iterate over each face of the cube and apply the appropriate rotation
        // view matrix to the cube, then draw it w/ the HDR texture applied to it
        for(int i = 0; i < 6; ++i) {
            progCubemapGlossyConvolution.setUnifMat4("u_ViewProj", views[i]);
            glContext.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                             GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                             glossyIrradianceFB.getCubemapHandle(), mipLevel);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            progCubemapGlossyConvolution.draw(cubeDrawable);
        }
    }


    glContext.glBindFramebuffer(GL_FRAMEBUFFER, this->defaultFramebufferObject());
}

void MyGL::renderEnvironmentMap() {
    // Bind cubemap texture to appropriate tex slot
    environmentCubemapFB.bindToTextureSlot(ENV_MAP_CUBE_TEX_SLOT, GBufferOutputType::NONE);
    //    m_diffuseIrradianceFB.bindToTextureSlot(DIFFUSE_IRRADIANCE_CUBE_TEX_SLOT, GBufferAttachmentType);
    //    m_glossyIrradianceFB.bindToTextureSlot(GLOSSY_IRRADIANCE_CUBE_TEX_SLOT, GBufferAttachmentType);
    // Set the environment map shader's cubemap sampler to the same tex slot
    progEnvMap.setUnifInt("u_EnvironmentMap", ENV_MAP_CUBE_TEX_SLOT);
    progEnvMap.setUnifMat4("u_ViewProj", camera.getViewProj_OrientOnly());
    progEnvMap.draw(cubeDrawable);
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
    camera.Zoom(e->angleDelta().y() * 0.001f);
}

// something about this causes Qt to stop rendering
// the entire GUI. haven't determined cause yet...
#if 0
void MyGL::loadScene() {
    QString path = getCurrentPath();
    path.append("/models/");
    QString filepath = QFileDialog::getOpenFileName(
                        0, QString("Load Scene"),
                        path, tr("*.json"));
    QFile file(filepath);

    path = filepath.left(filepath.lastIndexOf('/')) + "/";

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

//        Mesh mesh(this);
        meshDrawable.destroy();
        meshDrawable = Mesh(&glContext, (path + obj).toStdString());


        if(textureAlbedo.isCreated) {textureAlbedo.destroy();}
        if(textureMetallic.isCreated) {textureMetallic.destroy();}
        if(textureNormals.isCreated) {textureNormals.destroy();}
        if(textureRoughness.isCreated) {textureRoughness.destroy();}
        if(textureAO.isCreated) {textureAO.destroy();}
        if(textureDisplacement.isCreated) {textureDisplacement.destroy();}

        if(albedo != "") {
            textureAlbedo.create((path + albedo).toStdString().c_str(),
                                 false, true, false, false);
        }
        if(metallic != "") {
            textureMetallic.create((path + metallic).toStdString().c_str(),
                                   false, true, false, false);
        }
        if(normal != "") {
            textureNormals.create((path + normal).toStdString().c_str(),
                                  false, true, false, false);
        }
        if(roughness != "") {
            textureRoughness.create((path + roughness).toStdString().c_str(),
                                    false, true, false, false);
        }
        if(ambientOcclusion != "") {
            textureAO.create((path + ambientOcclusion).toStdString().c_str(),
                             false, true, false, false);
        }
        if(displacement != "") {
            textureDisplacement.create((path + displacement).toStdString().c_str(),
                                       false, true, false, false);
        }
        meshDrawable.initializeAndBufferGeometryData();
    }
    update();
}

void MyGL::loadEnvMap() {
    QString path = getCurrentPath();
    path.append("/environment_maps/");
    QString filepath = QFileDialog::getOpenFileName(
                        0, QString("Load Environment Map"),
                        path, tr("*.hdr"));
    try {
        textureHDREnvMap.destroy();
        textureHDREnvMap.create(filepath.toStdString().c_str(),
                                true, true, false, false);
    }
    catch(std::exception &e) {
        std::cout << "Error: Failed to load HDR image" << std::endl;
        return;
    }
    this->cubemapsNotGenerated = true;
    update();
}
#endif

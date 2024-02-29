#include "rasterizer.h"
#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>

// Helper Function
glm::vec3 BarycentricCoords(const glm::vec2& P, const glm::vec2& P1, const glm::vec2& P2, const glm::vec2& P3) {

    float alpha = ((P2.y - P3.y) * (P.x - P3.x) + (P3.x - P2.x) * (P.y - P3.y)) / ((P2.y - P3.y) * (P1.x - P3.x) + (P3.x - P2.x) * (P1.y - P3.y));
    float beta = ((P3.y - P1.y) * (P.x - P3.x) + (P1.x - P3.x) * (P.y - P3.y)) / ((P2.y - P3.y) * (P1.x - P3.x) + (P3.x - P2.x) * (P1.y - P3.y));
    float gamma = 1.0f - alpha - beta;
    return glm::vec3(alpha, beta, gamma);
}

glm::vec3 createZInv(const glm::vec3 Zvalue) {
    return glm::vec3(1.f / Zvalue[0],
                     1.f / Zvalue[1],
                     1.f / Zvalue[2]);
}

float InterpolateZ(const glm::vec3& zInv,
                   const glm::vec3& BarycentricCoords) {
    return 1.f / (glm::dot(zInv, BarycentricCoords));
}

Rasterizer::Rasterizer(const std::vector<Polygon>& polygons)
    : m_polygons(polygons)
{}

Camera& Rasterizer::GetCamera() {
    return m_camera;
}

void Rasterizer::setShadingModel(ShadingModel inShadingModel)
{
    shadingModel = inShadingModel;
}

glm::vec2 interpolateUV(const glm::vec2& UV1, const glm::vec2& UV2, const glm::vec2& UV3,
                        const glm::vec3& bcCoords,
                        const glm::vec3 zInv,
                        const float z) {
    const glm::mat3x2 uvs(UV1,
                          UV2,
                          UV3);
    return z * (uvs * (zInv * bcCoords));
}

glm::vec3 interpolateNormal(const glm::vec4& normal1, const glm::vec4& normal2, const glm::vec4& normal3,
                            const glm::vec3& bcCoords,
                            const glm::vec3 zInv,
                            const float z) {
    glm::vec3 n1 = glm::vec3(normal1);
    glm::vec3 n2 = glm::vec3(normal2);
    glm::vec3 n3 = glm::vec3(normal3);

    const glm::mat3x3 normals(n1, n2, n3);

    return z * (normals * (zInv * bcCoords));
}

QRgb ClampColor(const glm::vec3& color) {
    return qRgb(glm::clamp(color.x, 0.f, 255.f),
                glm::clamp(color.y, 0.f, 255.f),
                glm::clamp(color.z, 0.f, 255.f));
}

glm::mat3 createTangentMatrix(glm::vec3 normal) {
    glm::vec3 tangent = glm::cross(glm::vec3(0, 1, 0), normal);
    if (glm::length(tangent) < 0.0001f)
        tangent = glm::cross(glm::vec3(1, 0, 0), normal);
    tangent = glm::normalize(tangent);

    glm::vec3 bitangent = glm::cross(normal, tangent);
    bitangent = glm::normalize(bitangent);

    glm::mat3 result = {
                        tangent,
                        bitangent,
                        normal
                        };


    return result;
}

glm::vec3 getTangentNormal(glm::vec2 uv, QImage* normalMap) {
    int X = glm::min(normalMap->width() * uv.x, normalMap->width() - 1.0f);
    int Y = glm::min(normalMap->height() * (1.0f - uv.y), normalMap->height() - 1.0f);
    QColor color = normalMap->pixel(X, Y);
    glm::vec3 tangentNormal = glm::vec3(
        (color.red() / 255.0f) * 2.0f - 1.0f,
        (color.green() / 255.0f) * 2.0f - 1.0f,
        (color.blue() / 255.0f) * 2.0f - 1.0f
        );
    tangentNormal = glm::normalize(tangentNormal);

    return tangentNormal;
}

// Rasterization Main Logic
QImage Rasterizer::RenderScene() {
    int scalingResolution = 512 * scalingFactor;

    QImage result(scalingResolution, scalingResolution, QImage::Format_RGB32);
    result.fill(qRgb(0, 0, 0)); // Fill with black
    std::vector<float> zBuffer(result.width() * result.height(), std::numeric_limits<float>::max());

    glm::mat4 projectionMatrix = m_camera.GetPerspectiveMatrix();
    glm::mat4 viewMatrix = m_camera.GetViewMatrix();

    auto lightDir = -m_camera.GetForward();
    // glm::vec3 forwardInCameraSpace = -m_camera.GetForward(); // This is your camera forward vector in camera space
    // glm::mat4 inverseViewMatrix = glm::inverse(viewMatrix);

    // // Convert the forward vector to a 4D vector for matrix multiplication, assuming no translation component
    // glm::vec4 forwardInCameraSpace4D = glm::vec4(forwardInCameraSpace, 0.0f);

    // // Transform the forward vector back to world space
    // glm::vec4 forwardInWorldSpace4D = inverseViewMatrix * forwardInCameraSpace4D;

    // // Convert back to a 3D vector
    // glm::vec3 lightDir = glm::vec3(forwardInWorldSpace4D);

    auto eye = m_camera.GetPosition();

    glm::vec3 ambient(0.3f);
    glm::vec3 lightColor(1.0f);
    float shininess = 32.0f;

    for (const auto& polygon : m_polygons) {
        for (const auto& triangle : polygon.m_tris) {
            std::vector<glm::vec4> pixelSpaceVertices(3);
            glm::vec3 ZValue;
            for (int i = 0; i < 3; ++i) {
                glm::vec4 cameraSpace = viewMatrix * polygon.VertAt(triangle.m_indices[i]).m_pos;
                glm::vec4 screenSpace = projectionMatrix * cameraSpace;
                screenSpace.w = cameraSpace.z;
                glm::vec3 pixelSpace = glm::vec3(screenSpace) / screenSpace.w;
                pixelSpaceVertices[i].x = (pixelSpace.x + 1) * 0.5f * result.width();
                pixelSpaceVertices[i].y = (1 - pixelSpace.y) * 0.5f * result.height();
                ZValue[i] = screenSpace.w;
            }

            glm::vec3 zInv = createZInv(ZValue);

            // Bounding Box
            float minX = std::min({pixelSpaceVertices[0].x, pixelSpaceVertices[1].x, pixelSpaceVertices[2].x});
            float maxX = std::max({pixelSpaceVertices[0].x, pixelSpaceVertices[1].x, pixelSpaceVertices[2].x});
            float minY = std::min({pixelSpaceVertices[0].y, pixelSpaceVertices[1].y, pixelSpaceVertices[2].y});
            float maxY = std::max({pixelSpaceVertices[0].y, pixelSpaceVertices[1].y, pixelSpaceVertices[2].y});

            // Clamp to screen bounds
            minX = std::max(minX, 0.0f);
            maxX = std::min(maxX, static_cast<float>(result.width() - 1));
            minY = std::max(minY, 0.0f);
            maxY = std::min(maxY, static_cast<float>(result.height() - 1));

            // Rasterize
            for (int y = static_cast<int>(minY); y <= static_cast<int>(maxY); ++y) {
                for (int x = static_cast<int>(minX); x <= static_cast<int>(maxX); ++x) {
                    // Create pixel P
                    glm::vec2 P(x, y);
                    glm::vec3 barycentricCoords = BarycentricCoords(P, glm::vec2(pixelSpaceVertices[0]), glm::vec2(pixelSpaceVertices[1]), glm::vec2(pixelSpaceVertices[2]));

                    if (barycentricCoords.x >= 0 && barycentricCoords.y >= 0 && barycentricCoords.z >= 0) {
                        float z = InterpolateZ(zInv, barycentricCoords);
                        int zIndex = x + y * result.width();
                        if (z < zBuffer[zIndex]) {
                            zBuffer[zIndex] = z;
                            glm::vec2 uv = interpolateUV(polygon.VertAt(triangle.m_indices[0]).m_uv,
                                                         polygon.VertAt(triangle.m_indices[1]).m_uv,
                                                         polygon.VertAt(triangle.m_indices[2]).m_uv,
                                                         barycentricCoords, zInv, z);

                            glm::vec3 normal = interpolateNormal(polygon.VertAt(triangle.m_indices[0]).m_normal,
                                                              polygon.VertAt(triangle.m_indices[1]).m_normal,
                                                              polygon.VertAt(triangle.m_indices[2]).m_normal,
                                                              barycentricCoords, zInv, z);

                            glm::mat3 viewMatrix3x3 = glm::mat3(viewMatrix);

                            // Transform the normal to camera space
                            glm::vec3 normalInCameraSpace = viewMatrix3x3 * normal;

                            // Using Normal Map
                            if(polygon.mp_normalMap != nullptr) {
                                //std::cout << "using Normal Map" << std::endl;
                                glm::mat3 tangentSpaceMatrix = createTangentMatrix(normal);
                                glm::vec3 normalTangentSpace = getTangentNormal(uv, polygon.mp_normalMap);
                                normal = glm::normalize(tangentSpaceMatrix * normalTangentSpace);
                            }


                            // Lambert law
                            glm::vec3 diffuse = glm::dot(normal, lightDir) * lightColor;

                            glm::vec3 position = { x, y, z };

                            glm::vec3 viewDir = glm::normalize(eye - position);

                            // if (uv.x > 1 || uv.x < 0 || uv.y > 1 || uv.y < 0) {
                            //     std::cout << uv << " out of bound" << std::endl;
                            // }

                            //if (barycentricCoords.x + barycentricCoords.y + barycentricCoords.z > 1.0f)
                            //{
                            //    std::cout << "Wrong barycentric coords!" << std::endl;
                            //}

                            // get color from texture
                            glm::vec3 color = GetImageColor(uv, polygon.mp_texture);

                            glm::vec3 specular = glm::vec3(0.0f);

                            if (shadingModel == ShadingModel::BlinnPhong)
                            {
                                glm::vec3 halfwayDir = glm::normalize(viewDir + lightDir);

                                specular = std::max(std::pow(glm::dot(normal, halfwayDir), shininess), 0.0f) * lightColor;
                            }
                            else if (shadingModel == ShadingModel::Phong)
                            {
                                glm::vec3 reflectDir = glm::reflect(-lightDir, normal);
                                specular = std::max(std::pow(glm::dot(viewDir, reflectDir), shininess), 0.0f) * lightColor;
                            }

                            color *= (diffuse + ambient + specular);

                            //color = glm::vec3(uv.x * 255.0f, uv.y * 255.0f, 0.0f);
                            //color = glm::vec3(normal.x * 255.0f, normal.y * 255.0f, normal.z * 255.0f);

                            result.setPixel(x, y, ClampColor(color));
                        }
                    }
                }
            }
        }
    }

    QImage finalResult(512, 512, QImage::Format_RGB32);

    // Downscale for MSAA
    for (int y = 0; y < finalResult.height(); ++y) {
        for (int x = 0; x < finalResult.width(); ++x) {
            glm::vec3 avgColor(0.0f, 0.0f, 0.0f);
            for (int dy = 0; dy < scalingFactor; ++dy) {
                for (int dx = 0; dx < scalingFactor; ++dx) {
                    int highResX = x * scalingFactor + dx;
                    int highResY = y * scalingFactor + dy;
                    QRgb pixelColor = result.pixel(highResX, highResY);
                    avgColor.x += qRed(pixelColor);
                    avgColor.y += qGreen(pixelColor);
                    avgColor.z += qBlue(pixelColor);
                }
            }
            avgColor /= (scalingFactor * scalingFactor);
            finalResult.setPixel(x, y, ClampColor(avgColor));
        }
    }

    return finalResult;
}


void Rasterizer::ClearScene()
{
    m_polygons.clear();
}

#pragma once
#include <QList>
#include "camera.h"
#include "smartpointerhelp.h"
#include "scene/geometry/shape.h"
#include "scene/lights/light.h"

class Scene {
public:
    Scene();

    // Shapes
    std::vector<uPtr<Box>> boxes;
    std::vector<uPtr<Sphere>> spheres;
    std::vector<uPtr<RectangleShape>> rectangles;
    std::vector<uPtr<Mesh>> meshes;

    // Lights
    std::vector<uPtr<AreaLight>> areaLights;
    std::vector<uPtr<PointLight>> pointLights;
    std::vector<uPtr<SpotLight>> spotLights;

    // Textures
    std::vector<uPtr<Texture>> textures;

    Camera camera;

    void SetCamera(const Camera &c);

    void CreateTestScene();
    void Clear();

    // Write the definition of the initializeScene() GLSL function
    QString toGLSL() const;
};

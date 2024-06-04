#include "scene/scene.h"
#include "smartpointerhelp.h"

#include "scene/geometry/mesh.h"
#include "scene/materials/material.h"


Scene::Scene()
{}

QString Scene::toGLSL() const {
    QString result = "";

    result += "#define N_TEXTURES " + QString::number(textures.size()) + "\n";

    result += "#define N_BOXES " + QString::number(boxes.size()) + "\n";
    result += "#define N_RECTANGLES " + QString::number(rectangles.size()) + "\n";
    result += "#define N_SPHERES " + QString::number(spheres.size()) + "\n";
    result += "#define N_MESHES " + QString::number(meshes.size()) + "\n";
    int n_tris = 0;
    for(auto &m : meshes) {
        n_tris += m->numTris();
    }
    result += "#define N_TRIANGLES " + QString::number(n_tris) + "\n";

    result += "#define N_LIGHTS " + QString::number(spotLights.size() + areaLights.size() + pointLights.size()) + "\n";
    result += "#define N_AREA_LIGHTS " + QString::number(areaLights.size()) + "\n";
    result += "#define N_POINT_LIGHTS " + QString::number(pointLights.size()) + "\n";
    result += "#define N_SPOT_LIGHTS " + QString::number(spotLights.size()) + "\n";

    // Declare the set of sampler2Ds needed to read the textures
    if(textures.size() > 0) {
        result += "uniform sampler2D u_TexSamplers[N_TEXTURES];\n";
    }

    // Declare the arrays of Shapes and Lights
    int objCount = 0;
    if(boxes.size() > 0) {
        result += "const Box boxes[N_BOXES] = Box[](";
        for(unsigned int i = 0; i < boxes.size(); ++i) {
            result += boxes[i]->toGLSL(objCount++);
            result += (i+1 == boxes.size()) ? "\n" : ",\n";
        }
        result += ");\n";
    }
    if(rectangles.size() > 0) {
        result += "const Rectangle rectangles[N_RECTANGLES] = Rectangle[](";
        for(unsigned int i = 0; i < rectangles.size(); ++i) {
            result += rectangles[i]->toGLSL(objCount++);
            result += (i+1 == rectangles.size()) ? "\n" : ",\n";
        }
        result += ");\n";
    }
    if(spheres.size() > 0) {
        result += "const Sphere spheres[N_SPHERES] = Sphere[](";
        for(unsigned int i = 0; i < spheres.size(); ++i) {
            result += spheres[i]->toGLSL(objCount++);
            result += (i+1 == spheres.size()) ? "\n" : ",\n";
        }
        result += ");\n";
    }
    if(meshes.size() > 0) {
        // Make the list of meshes
        result += "const Mesh meshes[N_MESHES] = Mesh[](";
        for(unsigned int i = 0; i < meshes.size(); ++i) {
            result += meshes[i]->toGLSL(objCount++);
            result += (i+1 == meshes.size()) ? "\n" : ",\n";
        }
        result += ");\n";

        result += "uniform sampler2D[N_MESHES] u_TriangleStorageSamplers;\n";
    }
    if(areaLights.size() > 0) {
        result += "const AreaLight areaLights[N_AREA_LIGHTS] = AreaLight[](";
        for(unsigned int i = 0; i < areaLights.size(); ++i) {
            result += areaLights[i]->toGLSL(objCount++);
            result += (i+1 == areaLights.size()) ? "\n" : ",\n";
        }
        result += ");\n";
    }
    if(pointLights.size() > 0) {
        result += "const PointLight pointLights[N_POINT_LIGHTS] = PointLight[](";
        for(unsigned int i = 0; i < pointLights.size(); ++i) {
            result += pointLights[i]->toGLSL(objCount++);
            result += (i+1 == pointLights.size()) ? "\n" : ",\n";
        }
        result += ");\n";
    }
    if(spotLights.size() > 0) {
        result += "const SpotLight spotLights[N_SPOT_LIGHTS] = SpotLight[](";
        for(unsigned int i = 0; i < spotLights.size(); ++i) {
            result += spotLights[i]->toGLSL(objCount++);
            result += (i+1 == spotLights.size()) ? "\n" : ",\n";
        }
        result += ");\n";
    }
    return result;
}

void Scene::SetCamera(const Camera &c)
{
    camera = Camera(c);
}

void Scene::CreateTestScene()
{
    auto matteWhite = Material(Color3f(0.725, 0.71, 0.68), 0, -1, DIFFUSE_REFL, nullptr, nullptr, nullptr);
    auto matteRed   = Material(Color3f(0.63, 0.065, 0.05), 0, -1, DIFFUSE_REFL, nullptr, nullptr, nullptr);
    auto matteGreen = Material(Color3f(0.14, 0.45, 0.091), 0, -1, DIFFUSE_REFL, nullptr, nullptr, nullptr);

    // Floor, which is a large white plane
    uPtr<RectangleShape> floor = mkU<RectangleShape>(glm::vec3(0, -2.5, 0), glm::vec3(0, 1, 0), glm::vec2(5,5), matteWhite);
    floor->transform = Transform(Vector3f(0,0,0), Vector3f(0,0,0), Vector3f(1,1,1));

    // Left wall, which is a large red plane
    uPtr<RectangleShape> leftWall = mkU<RectangleShape>(glm::vec3(5, 2.5, 0), glm::vec3(-1, 0, 0), glm::vec2(5,5), matteRed);
    leftWall->transform = Transform(Vector3f(0,0,0), Vector3f(0,0,0), Vector3f(1,1,1));

    // Right wall, which is a large green plane
    uPtr<RectangleShape> rightWall = mkU<RectangleShape>(glm::vec3(-5, 2.5, 0), glm::vec3(1, 0, 0), glm::vec2(5,5), matteGreen);
    rightWall->transform = Transform(Vector3f(0,0,0), Vector3f(0,0,0), Vector3f(1,1,1));

    // Back wall, which is a large white plane
    uPtr<RectangleShape> backWall = mkU<RectangleShape>(glm::vec3(0, 2.5, 5), glm::vec3(0, 0, -1), glm::vec2(5,5), matteWhite);
    backWall->transform = Transform(Vector3f(0,0,0), Vector3f(0,0,0), Vector3f(1,1,1));

    // Ceiling, which is a large white plane
    uPtr<RectangleShape> ceiling = mkU<RectangleShape>(glm::vec3(0, 7.5, 0), glm::vec3(0, -1, 0), glm::vec2(5,5), matteWhite);
    backWall->transform = Transform(Vector3f(0,0,0), Vector3f(0,0,0), Vector3f(1,1,1));

    // Long cube
    uPtr<Box> longCube = mkU<Box>(glm::vec3(-0.5, -0.5, -0.5), glm::vec3(0.5, 0.5, 0.5), matteWhite);
    longCube->transform = Transform(Vector3f(2, 0, 3), Vector3f(0, 27.5, 0), Vector3f(3, 6, 3));

    // Short cube
    uPtr<Box> shortCube = mkU<Box>(glm::vec3(-3.5, -2.5, -0.75), glm::vec3(-0.5, 0.5, 2.25), matteWhite);
    shortCube->transform = Transform(Vector3f(0), Vector3f(0, -17.5, 0), Vector3f(1));

    // Light source, which is a diffuse area light with a large plane as its shape
    uPtr<AreaLight> lightSquare = mkU<AreaLight>(Color3f(40,40,40),
                                                 RECTANGLE_SHAPE,
                                                 Transform(Vector3f(0,7.45f,0),
                                                           Vector3f(90,0,0),
                                                           Vector3f(3, 3, 1)));

    rectangles.push_back(std::move(floor));
    rectangles.push_back(std::move(leftWall));
    rectangles.push_back(std::move(rightWall));
    rectangles.push_back(std::move(ceiling));
    rectangles.push_back(std::move(backWall));
    boxes.push_back(std::move(shortCube));
    boxes.push_back(std::move(longCube));
    areaLights.push_back(std::move(lightSquare));

    camera = Camera(400, 400, Point3f(0, 5.5, -30), Point3f(0,2.5,0), Vector3f(0,1,0));
    camera.near_clip = 0.1f;
    camera.far_clip = 100.0f;
    camera.fovy = 19.5f;
    camera.RecomputeAttributes();
}

void Scene::Clear()
{
    // These lists contain shared_ptrs
    // so the pointers will be freed
    // if appropriate when we clear the lists.
    rectangles.clear();
    boxes.clear();
    spheres.clear();
    areaLights.clear();
    spotLights.clear();
    pointLights.clear();
    camera = Camera();
}
